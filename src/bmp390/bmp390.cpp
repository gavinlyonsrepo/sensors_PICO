/*!
	@file bmp390.cpp
	@brief source file for bmp390 pressure sensor
*/

#include "../../include/bmp390/bmp390.hpp"

/*!
	@brief Constructor for SPI mode.
	@param spi Instance of SPI.
	@param baudRate SPI speed in Hertz.
	@param cs Chip-select pin.
	@param mosi Master out slave in pin.
	@param sck Serial clock pin.
	@param miso Master in slave out pin.
*/
BMP390_Sensor::BMP390_Sensor(spi_inst_t *spi, uint32_t baudRate, uint8_t cs, uint8_t mosi, uint8_t sck, uint8_t miso)
{
	_spiInst = spi;
	_baudRate = baudRate;
	_cs = cs;
	_mosi = mosi;
	_sck = sck;
	_miso = miso;
	_commMode = CommMode_e::SPI;
}

/*!
	@brief Constructor for I2C mode.
	@param i2c Instance of I2C.
	@param baudrate I2C speed in Hertz.
	@param timeOutDelay I2C timeout delay in micro seconds, uS.
	@param address I2C address of the sensor. 0x76 or 0x77.
	@param sda Serial data pin.
	@param scl Serial clock pin.
*/
BMP390_Sensor::BMP390_Sensor(i2c_inst_t *i2c, uint32_t baudrate, uint32_t timeOutDelay, uint8_t address, uint8_t sda, uint8_t scl)
{
	_i2cInst = i2c;
	_baudrate = baudrate;
	_timeOutDelay = timeOutDelay;
	_address = address;
	_mosi = sda;
	_sck = scl;
	_commMode = CommMode_e::I2C;
}

/*!
	@brief Default destructor.
*/
BMP390_Sensor::~BMP390_Sensor()
{
	// Destructor body
}

/*!
	@brief Initialise hardware, configure GPIO and communication peripheral.
*/
void BMP390_Sensor::InitSensor(void)
{
	if (_commMode == CommMode_e::I2C)
	{
		gpio_set_function(_sck, GPIO_FUNC_I2C);
		gpio_set_function(_mosi, GPIO_FUNC_I2C);
		gpio_pull_up(_sck);
		gpio_pull_up(_mosi);
		i2c_init(_i2cInst, _baudrate);
	}
	else
	{
		gpio_init(_mosi);
		gpio_init(_sck);
		gpio_init(_miso);
		gpio_init(_cs);
		gpio_set_dir(_cs, true);
		gpio_put(_cs, true);

		spi_init(_spiInst, _baudRate);

		spi_set_format(_spiInst,
						8,
						SPI_CPOL_0,
						SPI_CPHA_0,
						SPI_MSB_FIRST);

		gpio_set_function(_mosi, GPIO_FUNC_SPI);
		gpio_set_function(_miso, GPIO_FUNC_SPI);
		gpio_set_function(_sck,  GPIO_FUNC_SPI);
	}
	busy_wait_ms(100);
	StartUpRoutine();
	busy_wait_ms(100);
}

/*!
	@brief De-initialise hardware.
*/
void BMP390_Sensor::DeInitSensor(void)
{
	if (_commMode == CommMode_e::I2C)
	{
		gpio_set_function(_sck,  GPIO_FUNC_NULL);
		gpio_set_function(_mosi, GPIO_FUNC_NULL);
		gpio_deinit(_sck);
		gpio_deinit(_mosi);
		i2c_deinit(_i2cInst);
	}
	else
	{
		gpio_set_function(_cs,   GPIO_FUNC_NULL);
		gpio_set_dir(_cs, false);
		gpio_put(_cs, false);
		gpio_set_function(_mosi, GPIO_FUNC_NULL);
		gpio_set_function(_sck,  GPIO_FUNC_NULL);
		gpio_set_function(_miso, GPIO_FUNC_NULL);
		spi_deinit(_spiInst);
		gpio_deinit(_cs);
		gpio_deinit(_mosi);
		gpio_deinit(_sck);
		gpio_deinit(_miso);
	}
}

/*!
	@brief Poll STATUS register until cmd_rdy (bit 4) is set or timeout expires.
	@details The BMP390 must report cmd_rdy = 1 before accepting a new register write
	@param timeoutMs Maximum time to wait in milliseconds (default 100).
	@return True if cmd_rdy was observed within the timeout, false otherwise.
*/
bool BMP390_Sensor::waitCmdReady(uint32_t timeoutMs)
{
	uint32_t elapsed = 0;
	while (elapsed < timeoutMs)
	{
		uint8_t status = readRegister(Status);
		if (status & 0x10) // bit 4 = cmd_rdy
		{
			return true;
		}
		busy_wait_ms(1);
		elapsed++;
	}
	printf("BMP390::waitCmdReady timed out after %lu ms\n", timeoutMs);
	return false;
}

/*!
	@brief Start-up routine: reset sensor, load calibration, configure, enter normal mode.
	@details Sequence:
	  1. Soft-reset to guarantee clean register state.
	  2. Wait for power-on time and cmd_rdy.
	  3. Clear ERR_REG latch.
	  4. Load NVM trimming parameters.
	  5. Configure OSR, IIR, ODR in Sleep mode using safe conservative defaults.
	  6. Clear ERR_REG latch again before mode transition.
	  7. Enter Normal mode and verify.
*/
void BMP390_Sensor::StartUpRoutine(void)
{
	// Step 1: Soft reset — puts all registers back to power-on defaults.
	setRegister(CMD, 0xB6);
	busy_wait_ms(10); // BMP390 NVM load + startup time after reset (~2 ms min, 10 ms safe)

	// Step 2: Wait for device to signal readiness.
	if (!waitCmdReady(100))
	{
		printf("BMP390::StartUpRoutine: device not ready after reset\n");
		return;
	}

	// Step 3: Clear ERR_REG latch. This register is sticky — read clears all bits.
	//         Must be clear before config writes and before the Normal mode transition.
	(void)readRegister(ERR_REG);

	// Step 4: Load NVM trimming parameters used by the compensation formulas.
	getTrimmingParameters();

	// Step 5: Write configuration registers while sensor is in Sleep mode.
	setIIRFilter(Filter_e::Filter_OFF);
	setODR(ODR_e::ODR_50_Hz); // 50 Hz = 20 ms period
	uint8_t osrVal = (static_cast<uint8_t>(sensorSampling_e::Sampling_X1) << 3)
	               |  static_cast<uint8_t>(sensorSampling_e::Sampling_X1);
	setRegister(OSR, osrVal);
	_temperatureOversampling = sensorSampling_e::Sampling_X1;
	_pressureOversampling    = sensorSampling_e::Sampling_X1;
	busy_wait_ms(5);

	// Step 6: Enable sensors in Sleep mode first.
	//   0x03 = 0000 0011: mode[5:4]=00 (Sleep), temp_en(bit1)=1, press_en(bit0)=1
	setRegister(PWR_CTRL, 0x03);
	busy_wait_ms(5);

	// Step 7: Clear ERR_REG once more immediately before the mode transition.
	(void)readRegister(ERR_REG);
	waitCmdReady(100);

	// Step 8: Enter Normal mode.
	setRegister(PWR_CTRL, 0x33);
	busy_wait_ms(50); // wait at least one full ODR period (50 Hz = 20 ms); 50 ms is safe

	// Step 9: Verify mode bits were accepted. Also capture ERR_REG before it clears.
	uint8_t pwrCtrl = readRegister(PWR_CTRL);
	uint8_t errReg  = readRegister(ERR_REG); // read here so it's not cleared by debug prints
	uint8_t modeBits = (pwrCtrl >> 4) & 0x03;
	_powerMode = (modeBits == 3) ? PowerMode_e::Normal :
				 (modeBits == 0) ? PowerMode_e::Sleep   : PowerMode_e::Forced;

#if BMP390_DEBUG
	printf("BMP390 StartUpRoutine register dump:\n");
	printf("  PWR_CTRL  (0x1B) = 0x%02X  (mode bits[5:4]=%u)\n", pwrCtrl, modeBits);
	printf("  ERR_REG   (0x02) = 0x%02X  (0=OK, 4=conf_err, 2=cmd_err, 1=fatal)\n", errReg);
	printf("  OSR       (0x1C) = 0x%02X  (expect 0x09: temp=X1 bits[5:3]=001, press=X1 bits[2:0]=001)\n", readRegister(OSR));
	printf("  ODR       (0x1D) = 0x%02X  (expect 0x02: 50 Hz)\n",              readRegister(ODR));
	printf("  CONFIG    (0x1F) = 0x%02X  (expect 0x00: IIR off)\n",            readRegister(Config));
	printf("  STATUS    (0x03) = 0x%02X  (bit4=cmd_rdy, bit6=drdy_temp, bit5=drdy_press)\n",
		readRegister(Status));
#endif

	if (modeBits != 3)
	{
		printf("BMP390 ERROR: Normal mode not accepted. PWR_CTRL=0x%02X ERR_REG=0x%02X\n",
			pwrCtrl, errReg);
		printf("  Check: conf_err(bit2)=%u  cmd_err(bit1)=%u  fatal(bit0)=%u\n",
			(errReg >> 2) & 1, (errReg >> 1) & 1, errReg & 1);
	}
}

/*!
	@brief Read chip ID register from sensor.
	@return Chip ID value: 0x50 for BMP390, 0x60 for BMP390L.
*/
uint8_t BMP390_Sensor::readForChipID()
{
	uint8_t result = readRegister(Chip_ID);
	_chipID = result;
	return result;
}

/*!
	@brief Get cached chip ID value.
	@return Chip ID value last read via readForChipID().
*/
uint8_t BMP390_Sensor::getChipID() const
{
	return _chipID;
}

/*!
	@brief Write a value to a sensor register.
	@details Optionally reads back and verifies the written value.
	@param reg  Register to write.
	@param config Value to write.
	@param check If true, read back register and confirm value matches.
	@return True if write (and optional verify) succeeded, otherwise false.
*/
bool BMP390_Sensor::setRegister(Registers_e reg, uint8_t config, bool check)
{
	uint8_t regVal = static_cast<uint8_t>(reg);
	uint8_t data[2] = {regVal, config};

	if (_commMode == CommMode_e::I2C)
	{
		int16_t returnValue = i2c_write_timeout_us(_i2cInst, _address, data, 2, false, _timeOutDelay);
		if (returnValue < 1)
		{
			printf("BMP390 I2C write error: setRegister %d\n", returnValue);
			return false;
		}
	}
	else
	{
		// BMP390 SPI write: bit 7 of address must be 0
		data[0] = regVal & _SPI_WRITE_MASK;
		gpio_put(_cs, false);
		spi_write_blocking(_spiInst, data, 2);
		gpio_put(_cs, true);
	}

	if (check == false)
	{
		return true;
	}
	uint8_t value = readRegister(reg);
	return (value == config);
}

/*!
	@brief Read a single byte from a sensor register.
	@param reg Register to read.
	@return Register value, or 0xFF if a communication error occurs.
	@note BMP390 SPI reads require a dummy byte after the address byte.
*/
uint8_t BMP390_Sensor::readRegister(Registers_e reg)
{
	uint8_t buffer[1] = {0};
	uint8_t data[1] = {0};

	if (_commMode == CommMode_e::I2C)
	{
		data[0] = static_cast<uint8_t>(reg);
		int16_t returnValue = i2c_write_timeout_us(_i2cInst, _address, data, 1, true, _timeOutDelay);
		if (returnValue < 1)
		{
			printf("BMP390 I2C write error: readRegister %d\n", returnValue);
			return 0xFF;
		}
		returnValue = i2c_read_timeout_us(_i2cInst, _address, buffer, 1, false, _timeOutDelay);
		if (returnValue < 1)
		{
			printf("BMP390 I2C read error: readRegister %d\n", returnValue);
			return 0xFF;
		}
	}
	else
	{
		// BMP390 SPI: set bit 7 for read, then discard dummy byte before data
		uint8_t dummy = 0;
		data[0] = static_cast<uint8_t>(reg) | _SPI_READ_MASK;
		gpio_put(_cs, false);
		spi_write_blocking(_spiInst, data, 1);
		spi_read_blocking(_spiInst, 0, &dummy,     1); // dummy byte (per BMP390 datasheet)
		spi_read_blocking(_spiInst, 0, &buffer[0], 1);
		gpio_put(_cs, true);
	}
	return buffer[0];
}

/*!
	@brief Burst-read a block of consecutive registers.
	@details Reads length bytes starting at startReg into buffer.
		Used internally to fetch all 6 data bytes and calibration data.
	@param startReg First register address to read from.
	@param buffer   Destination byte array (must be at least length bytes).
	@param length   Number of bytes to read.
	@return True if successful, false if a communication error occurs.
	@note BMP390 SPI reads require a dummy byte after the address byte.
*/
bool BMP390_Sensor::burstReadRegisters(Registers_e startReg, uint8_t *buffer, uint8_t length)
{
	uint8_t reg[1] = {static_cast<uint8_t>(startReg)};

	if (_commMode == CommMode_e::I2C)
	{
		int16_t returnValue = i2c_write_timeout_us(_i2cInst, _address, reg, 1, true, _timeOutDelay);
		if (returnValue < 1)
		{
			printf("BMP390 I2C write error: burstReadRegisters %d\n", returnValue);
			return false;
		}
		returnValue = i2c_read_timeout_us(_i2cInst, _address, buffer, length, false, _timeOutDelay);
		if (returnValue < 1)
		{
			printf("BMP390 I2C read error: burstReadRegisters %d\n", returnValue);
			return false;
		}
	}
	else
	{
		uint8_t dummy = 0;
		reg[0] = static_cast<uint8_t>(startReg) | _SPI_READ_MASK;
		gpio_put(_cs, false);
		spi_write_blocking(_spiInst, reg, 1);
		spi_read_blocking(_spiInst, 0, &dummy,  1); // dummy byte (per BMP390 datasheet)
		spi_read_blocking(_spiInst, 0, buffer, length);
		gpio_put(_cs, true);
	}
	return true;
}

/*!
	@brief Set power mode of BMP390 sensor.
	@details In Normal and Forced modes, both pressure and temperature
		measurements are enabled automatically.
	@param mode PowerMode_e value to apply.
	@param check If true, reads back PWR_CTRL and confirms mode bits.
	@note The BMP390 does not allow a direct Normal → Forced transition.
			The write is silently ignored. Must go through Sleep first.
	@return True if successful, otherwise false.
*/
bool BMP390_Sensor::setPowerMode(PowerMode_e mode, bool check)
{
	if (mode == PowerMode_e::Forced && _powerMode == PowerMode_e::Normal)
	{
		setRegister(PWR_CTRL, 0x00); // Sleep: mode=00, sensors off
		busy_wait_ms(5);
	}
	uint8_t config = 0;
	switch (mode)
	{
		case PowerMode_e::Sleep:
			config = 0x00;
			break;
		case PowerMode_e::Forced:
			config = 0x13;
			break;
		case PowerMode_e::Normal:
			config = 0x33;
			break;
	}
	bool result = setRegister(PWR_CTRL, config, check);
	if (result)
	{
		_powerMode = mode;
	}
	return result;
}

/*!
	@brief Read current power mode from sensor PWR_CTRL register.
	@details Mode bits are [5:4] per BMP390 datasheet Table 23
	@return Current PowerMode_e value.
*/
BMP390_Sensor::PowerMode_e BMP390_Sensor::readPowerMode()
{
	uint8_t data = readRegister(PWR_CTRL);
	uint8_t modeBits = (data >> 4) & 0x03;
	PowerMode_e mode;
	switch (modeBits)
	{
		case 0:  mode = PowerMode_e::Sleep;  break;
		case 1:
		case 2:  mode = PowerMode_e::Forced; break;
		default: mode = PowerMode_e::Normal; break;
	}
	_powerMode = mode;
	return mode;
}

/*!
	@brief Get cached power mode value.
	@return Last known PowerMode_e value.
*/
BMP390_Sensor::PowerMode_e BMP390_Sensor::getPowerMode() const
{
	return _powerMode;
}

/*!
	@brief Software reset of BMP390 via CMD register.
	@details Writing 0xB6 triggers a complete power-on-reset sequence.
*/
void BMP390_Sensor::reset()
{
	setRegister(CMD, 0xB6);
	busy_wait_ms(10);
}

/*!
	@brief Validate that the measurement time fits within the ODR period.
	@details The BMP390 sets conf_err and refuses Normal mode if the total
		measurement time exceeds the ODR period. This guard catches that
		condition before writing to hardware, giving a clear error message
		rather than a silent sensor lockup.
		Measurement time formula (BMP390 datasheet section 3.9.2):
		  T_meas[µs] = 234 + (N_temp + N_press) × 2020
		where N = 2^(osr-1) for osr >= 1, and N = 0 if osr = 0 (skipped).
		ODR period starts at 5000µs (200 Hz) and doubles each step.
	@param tempOS  Temperature oversampling setting to validate.
	@param pressOS Pressure oversampling setting to validate.
	@param odr     ODR setting to validate against.
	@return True if the combination is safe to write, false if it would cause conf_err.
*/
bool BMP390_Sensor::isOSRODRValid(sensorSampling_e tempOS, sensorSampling_e pressOS, ODR_e odr)
{
	// ODR constraint only applies in Normal mode.
	// In Forced and Sleep modes measurements are on-demand with no period constraint.
	if (_powerMode != PowerMode_e::Normal)
	{
		return true;
	}

	// Derive oversampling repetition counts from enum values.
	auto osrToN = [](sensorSampling_e os) -> uint32_t {
		uint8_t val = static_cast<uint8_t>(os);
		return (val == 0) ? 1u : (1u << (val - 1));
	};
	uint32_t n_temp  = osrToN(tempOS);
	uint32_t n_press = osrToN(pressOS);
	// Formula from datasheet section 3.9.2 (both sensors enabled):
	// T_conv = 234µs + press_en*(392µs + 2^osr_p * 2020µs)
	//                + temp_en *(163µs + 2^osr_t * 2020µs)
	// With both sensors enabled and simplifying:
	// T_conv = 234 + 392 + 163 + (N_press + N_temp) * 2020
	//        = 789 + (N_press + N_temp) * 2020  [µs]
	uint32_t t_meas_us = 789u + (n_temp + n_press) * 2020u;
	// ODR period: 5000µs at 200 Hz (odr_sel=0), doubles each step.
	// Period[µs] = 5000 * 2^odr_sel  (Table 45, datasheet)
	uint32_t t_odr_us = 5000u << static_cast<uint8_t>(odr);

	if (t_meas_us >= t_odr_us)
	{
		// Walk up from requested ODR until period exceeds T_meas.
		uint8_t safeOdrVal = static_cast<uint8_t>(odr);
		while (safeOdrVal <= 17u)
		{
			if ((5000u << safeOdrVal) > t_meas_us){break;}
			safeOdrVal++;
		}
		printf("BMP390 ERROR: OSR/ODR combination invalid (Normal mode only).\n");
		printf("  Measurement time : %lu us\n", t_meas_us);
		printf("  ODR period       : %lu us\n", t_odr_us);
		printf("  T_meas must be < T_odr.\n");
		if (safeOdrVal <= 17u)
		{
			printf("  Minimum safe ODR : ODR enum value %u (period = %u us).\n",
				safeOdrVal, 5000u << safeOdrVal);
		}else{
			printf("  No valid ODR exists for this OSR combination. Reduce oversampling.\n");
		}
		return false;
	}
	return true;
}

/*!
	@brief Set oversampling for temperature or pressure.
	@details OSR register layout: bits [5:3] = osr_t, bits [2:0] = osr_p.
		Validates the new OSR value against the current ODR setting before
		writing. If the combination would cause conf_err the write is refused
		and false is returned with a diagnostic message.
	@param type Temperature or Pressure.
	@param oversampling Oversampling value (Sampling_None through Sampling_X32).
	@param check If true, reads back OSR register to verify.
	@return True if successful, false if invalid combination or write failed.
*/
bool BMP390_Sensor::setOversampling(DataType_e type, sensorSampling_e oversampling, bool check)
{
	static constexpr uint8_t OSR_MASK_T = 0xC7; // Preserve bits [7:6],[2:0], clear [5:3]
	static constexpr uint8_t OSR_MASK_P = 0xF8; // Preserve bits [7:3], clear [2:0]

	uint8_t osVal = static_cast<uint8_t>(oversampling);
	if (osVal > static_cast<uint8_t>(sensorSampling_e::Sampling_X32))
	{
		printf("BMP390 ERROR: setOversampling: invalid oversampling value %u\n", osVal);
		return false;
	}

	// Validate the new OSR against the current ODR before touching hardware.
	// Use the prospective new value for the axis being changed, keep cached for the other.
	sensorSampling_e prospectiveTemp  = (type == DataType_e::Temperature) ? oversampling : _temperatureOversampling;
	sensorSampling_e prospectivePress = (type == DataType_e::Pressure)    ? oversampling : _pressureOversampling;
	if (!isOSRODRValid(prospectiveTemp, prospectivePress, _odr))
	{
		return false;
	}
	uint8_t registerValue = readRegister(OSR);
	switch (type)
	{
		case DataType_e::Temperature:
			registerValue = (registerValue & OSR_MASK_T) | (osVal << 3);
			break;
		case DataType_e::Pressure:
			registerValue = (registerValue & OSR_MASK_P) | osVal;
			break;
	}
	bool result = setRegister(OSR, registerValue, check);
	if (result)
	{
		switch (type)
		{
			case DataType_e::Temperature:
				_temperatureOversampling = oversampling;
				break;
			case DataType_e::Pressure:
				_pressureOversampling = oversampling;
				break;
		}
	}
	return result;
}

/*!
	@brief Read oversampling setting from OSR register.
	@param type DataType_e::Temperature or DataType_e::Pressure.
	@return sensorSampling_e value currently applied.
*/
BMP390_Sensor::sensorSampling_e BMP390_Sensor::readOversampling(DataType_e type)
{
	uint8_t reg = readRegister(OSR);
	sensorSampling_e result = sensorSampling_e::Sampling_None;
	uint8_t bits = 0;

	switch (type)
	{
		case DataType_e::Temperature:
			bits = (reg >> 3) & 0x07;
			break;
		case DataType_e::Pressure:
			bits = reg & 0x07;
			break;
		default:
			printf("BMP390_Sensor::readOversampling: Invalid type\n");
			return sensorSampling_e::Sampling_None;
	}
	switch (bits)
	{
		case 0:  result = sensorSampling_e::Sampling_None; break;
		case 1:  result = sensorSampling_e::Sampling_X1;   break;
		case 2:  result = sensorSampling_e::Sampling_X2;   break;
		case 3:  result = sensorSampling_e::Sampling_X4;   break;
		case 4:  result = sensorSampling_e::Sampling_X8;   break;
		case 5:  result = sensorSampling_e::Sampling_X16;  break;
		default: result = sensorSampling_e::Sampling_X32;  break;
	}
	if (type == DataType_e::Temperature)
		_temperatureOversampling = result;
	else
		_pressureOversampling = result;

	return result;
}

/*!
	@brief Get cached oversampling value.
	@param type DataType_e::Temperature or DataType_e::Pressure.
	@return Cached sensorSampling_e value.
*/
BMP390_Sensor::sensorSampling_e BMP390_Sensor::getOversampling(DataType_e type) const
{
	switch (type)
	{
		case DataType_e::Temperature:
			return _temperatureOversampling;
		case DataType_e::Pressure:
			return _pressureOversampling;
		default:
			printf("BMP390_Sensor::getOversampling: Invalid type\n");
			return sensorSampling_e::Sampling_None;
	}
}

/*!
	@brief Set IIR filter coefficient via CONFIG register.
	@details CONFIG register layout: bits [3:1] = iir_filter[2:0].
	@param filter Filter_e coefficient value.
	@param check If true, reads back CONFIG register to verify.
	@return True if successful, otherwise false.
*/
bool BMP390_Sensor::setIIRFilter(Filter_e filter, bool check)
{
	static constexpr uint8_t CONFIG_MASK_FILTER = 0xF1; // Preserve bits [7:4],[0], clear [3:1]
	uint8_t registerValue = readRegister(Config);
	registerValue = (registerValue & CONFIG_MASK_FILTER) | (static_cast<uint8_t>(filter) << 1);

	bool result = setRegister(Config, registerValue, check);
	if (result)
	{
		_iirFilter = filter;
	}
	return result;
}

/*!
	@brief Read IIR filter coefficient from CONFIG register.
	@return Filter_e value currently set.
*/
BMP390_Sensor::Filter_e BMP390_Sensor::readIIRFilter()
{
	uint8_t reg = readRegister(Config);
	uint8_t bits = (reg >> 1) & 0x07;
	Filter_e result;
	switch (bits)
	{
		case 0:  result = Filter_e::Filter_OFF; break;
		case 1:  result = Filter_e::Filter_X1;  break;
		case 2:  result = Filter_e::Filter_X3;  break;
		case 3:  result = Filter_e::Filter_X7;  break;
		case 4:  result = Filter_e::Filter_X15; break;
		case 5:  result = Filter_e::Filter_X31; break;
		case 6:  result = Filter_e::Filter_X63; break;
		default: result = Filter_e::Filter_X127; break;
	}
	_iirFilter = result;
	return result;
}

/*!
	@brief Get cached IIR filter value.
	@return Cached Filter_e value.
*/
BMP390_Sensor::Filter_e BMP390_Sensor::getIIRFilter() const
{
	return _iirFilter;
}

/*!
	@brief Set output data rate via ODR register.
	@details Validates the requested ODR against the current OSR settings before
		writing. If the ODR period would be shorter than the measurement time
		the write is refused and false is returned with a diagnostic message.
	@param odr ODR_e value to apply.
	@param check If true, reads back ODR register to verify.
	@return True if successful, false if invalid combination or write failed.
*/
bool BMP390_Sensor::setODR(ODR_e odr, bool check)
{
	// Validate new ODR against current OSR settings before writing.
	if (!isOSRODRValid(_temperatureOversampling, _pressureOversampling, odr))
	{
		return false;
	}
	static constexpr uint8_t ODR_MASK = 0xE0; // Preserve bits [7:5], clear [4:0]
	uint8_t registerValue = readRegister(ODR);
	registerValue = (registerValue & ODR_MASK) | static_cast<uint8_t>(odr);
	bool result = setRegister(ODR, registerValue, check);
	if (result)
	{
		_odr = odr;
	}
	return result;
}

/*!
	@brief Read output data rate setting from ODR register.
	@return ODR_e value currently set.
*/
BMP390_Sensor::ODR_e BMP390_Sensor::readODR()
{
	uint8_t bits = readRegister(ODR) & 0x1F;
	ODR_e result = (bits <= static_cast<uint8_t>(ODR_e::ODR_0_0015_Hz))
					? static_cast<ODR_e>(bits)
					: ODR_e::ODR_0_0015_Hz;
	_odr = result;
	return result;
}

/*!
	@brief Get cached ODR value.
	@return Cached ODR_e value.
*/
BMP390_Sensor::ODR_e BMP390_Sensor::getODR() const
{
	return _odr;
}

/*!
	@brief Trigger a single forced measurement and block until data is ready.
	@details Each call to this function writes the Forced mode command to PWR_CTRL,
		which triggers exactly one measurement. The sensor then returns automatically
		to Sleep. This function polls STATUS register bits drdy_temp (bit 6) and
		drdy_press (bit 5) and blocks until both are set, or until timeout.
		After this call, use getTemperature() and getPressure() to read results
		without issuing another bus transaction.
	@note Can be called regardless of current power mode. If the sensor is in
		Normal mode it is first put to Sleep before the Forced command is issued,
		as a direct Normal → Forced transition is not supported by the BMP390.
	@param timeoutMs Maximum time to wait for data ready in milliseconds.
	@return True if measurement completed successfully, false on timeout.
*/
bool BMP390_Sensor::takeForcedMeasurement(uint32_t timeoutMs)
{
	// If in Normal mode, must go through Sleep first (BMP390 hardware requirement).
	if (_powerMode == PowerMode_e::Normal)
	{
		setRegister(PWR_CTRL, 0x00);
		busy_wait_ms(5);
	}

	// Trigger one measurement: mode=01 (Forced), temp_en=1, press_en=1
	// Sensor performs measurement then returns to Sleep automatically.
	setRegister(PWR_CTRL, 0x13);
	_powerMode = PowerMode_e::Forced;
	// Poll STATUS register until drdy_press (bit 5) and drdy_temp (bit 6) both set.
	uint32_t elapsed = 0;
	while (elapsed < timeoutMs)
	{
		busy_wait_ms(1);
		elapsed++;
		uint8_t status = readRegister(Status);
		if ((status & 0x60) == 0x60)
		{
			// Data is ready — read and compensate both values now.
			readSensorData();
			_temperature = compensateTemperature(_rawTemperature);
			_pressure    = compensatePressure(_rawPressure);
			_powerMode   = PowerMode_e::Sleep; // sensor returned to sleep
			return true;
		}
	}
	printf("BMP390::takeForcedMeasurement timed out after %lu ms\n", timeoutMs);
	return false;
}

/*!
	@brief Burst-read all 6 data registers and update raw temperature and pressure.
	@details Reads DATA_0 through DATA_5 in a single transaction, assembles
		the two 24-bit ADC values, and stores them in _rawTemperature and _rawPressure.
	@return True if successful, false if a communication error occurs.
*/
bool BMP390_Sensor::readSensorData(void)
{
	uint8_t buffer[6] = {0};
	if (!burstReadRegisters(Data_0, buffer, 6))
	{
		return false;
	}
	_rawPressure    = ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[1] << 8) | (uint32_t)buffer[0];
	_rawTemperature = ((int32_t)buffer[5] << 16) | ((int32_t)buffer[4] << 8) | (int32_t)buffer[3];
	return true;
}

/*!
	@brief Apply BMP390 temperature compensation formula to a raw ADC value.
	@details Uses the double-precision floating-point formula from the BMP390
		datasheet section 8.6.1. Also updates _tLin which is needed for
		pressure compensation.
	@param rawTemp Raw 24-bit temperature ADC output.
	@return Compensated temperature in degrees Celsius.
*/
double BMP390_Sensor::compensateTemperature(int32_t rawTemp)
{
	// Calibration coefficients scaled as per datasheet Table 2
	double par_t1 = (double)calib_data_t.nvm_par_t1 * 256.0;           // / 2^-8
	double par_t2 = (double)calib_data_t.nvm_par_t2 / 1073741824.0;    // / 2^30
	double par_t3 = (double)calib_data_t.nvm_par_t3 / 281474976710656.0; // / 2^48
	double pd1 = (double)rawTemp - par_t1;
	double pd2 = pd1 * par_t2;
	_tLin = pd2 + (pd1 * pd1) * par_t3;
	return _tLin;
}

/*!
	@brief Apply BMP390 pressure compensation formula to a raw ADC value.
	@details Uses the double-precision floating-point formula from the BMP390
		datasheet section 8.6.2. Requires compensateTemperature() to have been
		called first so that _tLin is valid.
	@param rawPress Raw 24-bit pressure ADC output.
	@return Compensated pressure in Pascals.
*/
double BMP390_Sensor::compensatePressure(int32_t rawPress)
{
	// Calibration coefficients scaled as per datasheet Table 2
	double par_p1  = ((double)calib_data_t.nvm_par_p1  - 16384.0) / 1048576.0;
	double par_p2  = ((double)calib_data_t.nvm_par_p2  - 16384.0) / 536870912.0;
	double par_p3  = (double)calib_data_t.nvm_par_p3  / 4294967296.0;
	double par_p4  = (double)calib_data_t.nvm_par_p4  / 137438953472.0;
	double par_p5  = (double)calib_data_t.nvm_par_p5  * 8.0;            // / 2^-3
	double par_p6  = (double)calib_data_t.nvm_par_p6  / 64.0;
	double par_p7  = (double)calib_data_t.nvm_par_p7  / 256.0;
	double par_p8  = (double)calib_data_t.nvm_par_p8  / 32768.0;
	double par_p9  = (double)calib_data_t.nvm_par_p9  / 281474976710656.0;
	double par_p10 = (double)calib_data_t.nvm_par_p10 / 281474976710656.0;
	double par_p11 = (double)calib_data_t.nvm_par_p11 / 36893488147419103232.0; // / 2^65

	// First part: temperature-dependent offset
	double pd1 = par_p6 * _tLin;
	double pd2 = par_p7 * (_tLin * _tLin);
	double pd3 = par_p8 * (_tLin * _tLin * _tLin);
	double po1 = par_p5 + pd1 + pd2 + pd3;

	// Second part: pressure x temperature cross-term
	pd1 = par_p2 * _tLin;
	pd2 = par_p3 * (_tLin * _tLin);
	pd3 = par_p4 * (_tLin * _tLin * _tLin);
	double po2 = (double)rawPress * (par_p1 + pd1 + pd2 + pd3);

	// Third part: higher-order pressure terms
	pd1 = (double)rawPress * (double)rawPress;
	pd2 = par_p9 + par_p10 * _tLin;
	pd3 = pd1 * pd2;
	double pd4 = pd3 + par_p11 * (double)rawPress * (double)rawPress * (double)rawPress;

	return po1 + po2 + pd4; // Pascals
}

/*!
	@brief Read and compensate temperature. Updates _rawTemperature and _temperature.
	@return Compensated temperature in degrees Celsius.
*/
int32_t BMP390_Sensor::readRawTemperature()
{
	readSensorData();
	_temperature = compensateTemperature(_rawTemperature);
	return _rawTemperature;
}

/*!
	@brief Get cached raw (uncompensated) temperature ADC value.
	@return Raw temperature value last read.
*/
int32_t BMP390_Sensor::getRawTemperature() const
{
	return _rawTemperature;
}

/*!
	@brief Read and return compensated temperature.
	@details Performs a full sensor read, compensates temperature and pressure.
	@return Compensated temperature in degrees Celsius.
*/
double BMP390_Sensor::readTemperature()
{
	readSensorData();
	_temperature = compensateTemperature(_rawTemperature);
	// Also compensate pressure to keep _pressure in sync
	_pressure = compensatePressure(_rawPressure);
	return _temperature;
}

/*!
	@brief Get cached compensated temperature value.
	@return Temperature in degrees Celsius.
*/
double BMP390_Sensor::getTemperature() const
{
	return _temperature;
}

/*!
	@brief Read and compensate pressure. Updates _rawPressure and _pressure.
	@details Temperature must be compensated first (compensateTemperature is
		called internally) as pressure compensation depends on _tLin.
	@return Raw (uncompensated) 24-bit pressure ADC value.
*/
uint32_t BMP390_Sensor::readRawPressure()
{
	readSensorData();
	// Temperature must be compensated first to update _tLin
	compensateTemperature(_rawTemperature);
	_pressure = compensatePressure(_rawPressure);
	return _rawPressure;
}

/*!
	@brief Get cached raw (uncompensated) pressure ADC value.
	@return Raw pressure value last read.
*/
uint32_t BMP390_Sensor::getRawPressure() const
{
	return _rawPressure;
}

/*!
	@brief Read and return compensated pressure.
	@param unit Pa (default) or hPa.
	@return Compensated pressure in the requested unit.
*/
double BMP390_Sensor::readPressure(PressureUnit_e unit)
{
	readSensorData();
	_temperature = compensateTemperature(_rawTemperature);
	_pressure    = compensatePressure(_rawPressure);
	return (unit == PressureUnit_e::Pa ? _pressure : (_pressure / 100.0));
}

/*!
	@brief Get cached compensated pressure value.
	@param unit Pa (default) or hPa.
	@return Pressure in the requested unit.
*/
double BMP390_Sensor::getPressure(PressureUnit_e unit) const
{
	return (unit == PressureUnit_e::Pa ? _pressure : (_pressure / 100.0));
}

/*!
	@brief Calculate approximate altitude above sea level.
	@param seaLevelhPa Current sea-level pressure in hPa (e.g. 1013.25).
	@return Altitude in metres.
*/
double BMP390_Sensor::readAltitude(double seaLevelhPa)
{
	double pressure = readPressure() / 100.0; // Convert Pa to hPa
	return 44330.0 * (1.0 - pow(pressure / seaLevelhPa, 0.1903));
}

/*!
	@brief Calculate sea-level pressure (QNH) from known altitude and local pressure.
	@param altitude     Known altitude in metres.
	@param atmospheric  Measured atmospheric pressure in hPa.
	@return Estimated sea-level pressure in hPa.
*/
double BMP390_Sensor::seaLevelForAltitude(double altitude, double atmospheric)
{
	return atmospheric / pow(1.0 - (altitude / 44330.0), 5.255);
}

/*!
	@brief Read all 21 bytes of NVM trimming parameters from sensor.
	@details Parameters span registers 0x31-0x45 and are stored in
		calib_data_t for use in temperature and pressure compensation.
	@return True if read succeeded, false if a communication error occurs.
*/
bool BMP390_Sensor::getTrimmingParameters(void)
{
	uint8_t buffer[21] = {};
	if (!burstReadRegisters(CALIB_DATA, buffer, 21))
	{
		return false;
	}

	calib_data_t.nvm_par_t1  = (uint16_t)((buffer[1]  << 8) | buffer[0]);
	calib_data_t.nvm_par_t2  = (uint16_t)((buffer[3]  << 8) | buffer[2]);
	calib_data_t.nvm_par_t3  = (int8_t)   buffer[4];
	calib_data_t.nvm_par_p1  = (int16_t) ((buffer[6]  << 8) | buffer[5]);
	calib_data_t.nvm_par_p2  = (int16_t) ((buffer[8]  << 8) | buffer[7]);
	calib_data_t.nvm_par_p3  = (int8_t)   buffer[9];
	calib_data_t.nvm_par_p4  = (int8_t)   buffer[10];
	calib_data_t.nvm_par_p5  = (uint16_t)((buffer[12] << 8) | buffer[11]);
	calib_data_t.nvm_par_p6  = (uint16_t)((buffer[14] << 8) | buffer[13]);
	calib_data_t.nvm_par_p7  = (int8_t)   buffer[15];
	calib_data_t.nvm_par_p8  = (int8_t)   buffer[16];
	calib_data_t.nvm_par_p9  = (int16_t) ((buffer[18] << 8) | buffer[17]);
	calib_data_t.nvm_par_p10 = (int8_t)   buffer[19];
	calib_data_t.nvm_par_p11 = (int8_t)   buffer[20];

#if BMP390_DEBUG
	printf("BMP390 Trimming parameters loaded:\n");
	printf("  T1=%u T2=%u T3=%d\n",
		calib_data_t.nvm_par_t1, calib_data_t.nvm_par_t2, calib_data_t.nvm_par_t3);
	printf("  P1=%d P2=%d P3=%d P4=%d P5=%u P6=%u\n",
		calib_data_t.nvm_par_p1, calib_data_t.nvm_par_p2,
		calib_data_t.nvm_par_p3, calib_data_t.nvm_par_p4,
		calib_data_t.nvm_par_p5, calib_data_t.nvm_par_p6);
	printf("  P7=%d P8=%d P9=%d P10=%d P11=%d\n",
		calib_data_t.nvm_par_p7, calib_data_t.nvm_par_p8, calib_data_t.nvm_par_p9,
		calib_data_t.nvm_par_p10, calib_data_t.nvm_par_p11);
#endif
	return true;
}

/*!
	@brief Check if device is present on the I2C bus.
	@return Number of bytes received (>= 1 = connected, < 1 = error/not found).
*/
int16_t BMP390_Sensor::CheckConnectionI2C(void)
{
	uint8_t rxData = 0;
	int16_t returnValue = i2c_read_timeout_us(_i2cInst, _address, &rxData, 1, false, _timeOutDelay);
#if BMP390_DEBUG
	printf("BMP390::CheckConnectionI2C\n");
	printf("I2C Return value = %d, RxData = %u\n", returnValue, rxData);
	printf(returnValue >= 1 ? "Connected.\n" : "Not Connected.\n");
#endif
	return returnValue;
}

// === Interrupt methods ===

/*!
	@brief Configure the INT pin and enable interrupt sources.
	@details Writes INT_CTRL register (0x19). Bit layout per datasheet Table 40:
	  bit 0 = int_od    (0=push-pull,  1=open-drain)
	  bit 1 = int_level (0=active-low, 1=active-high)
	  bit 2 = int_latch (0=non-latched, 1=latched)
	  bit 3 = fwtm_en   (FIFO watermark interrupt enable)
	  bit 4 = ffull_en  (FIFO full interrupt enable)
	  bit 5 = int_ds    (driven to 0 — reserved, keep low)
	  bit 6 = drdy_en   (data ready interrupt enable)
	@param outputMode Push-pull or open-drain.
	@param level      Active-high or active-low.
	@param latch      Non-latched or latched.
	@param drdyEn     Enable data-ready interrupt on INT pin.
	@param ffullEn    Enable FIFO-full interrupt on INT pin.
	@param fwtmEn     Enable FIFO-watermark interrupt on INT pin.
	@param check      If true, read back INT_CTRL to verify.
	@return True if write (and optional verify) succeeded.
*/
bool BMP390_Sensor::configureInterrupt(IntOutputMode_e outputMode,
                                       IntLevel_e      level,
                                       IntLatch_e      latch,
                                       bool            drdyEn,
                                       bool            ffullEn,
                                       bool            fwtmEn,
                                       bool            check)
{
	uint8_t reg = 0;
	reg |= (static_cast<uint8_t>(outputMode) & 0x01);       // bit 0: int_od
	reg |= (static_cast<uint8_t>(level)      & 0x01) << 1;  // bit 1: int_level
	reg |= (static_cast<uint8_t>(latch)      & 0x01) << 2;  // bit 2: int_latch
	reg |= (fwtmEn  ? 0x08u : 0x00u);                       // bit 3: fwtm_en
	reg |= (ffullEn ? 0x10u : 0x00u);                       // bit 4: ffull_en
	// bit 5 (int_ds) stays 0 — test mode bit, must not be set
	reg |= (drdyEn  ? 0x40u : 0x00u);                       // bit 6: drdy_en

#if BMP390_DEBUG
	printf("BMP390::configureInterrupt INT_CTRL = 0x%02X\n", reg);
#endif

	return setRegister(INT_CTRL, reg, check);
}

/*!
	@brief Read and clear the INT_STATUS register (0x11).
	@details INT_STATUS is clear-on-read (datasheet section 4.3.9).
	  In non-latched mode reading this also de-asserts the INT pin.
	  Bit layout:
	    bit 0 = fwm_int   (FIFO watermark)
	    bit 1 = ffull_int (FIFO full)
	    bit 3 = drdy      (data ready)
	@return IntStatus_t struct with individual flags.
*/
BMP390_Sensor::IntStatus_t BMP390_Sensor::readIntStatus()
{
	uint8_t reg = readRegister(INT_Status);
	IntStatus_t status;
	status.fifoWatermark = (reg & 0x01) != 0; // bit 0
	status.fifoFull      = (reg & 0x02) != 0; // bit 1
	status.dataReady     = (reg & 0x08) != 0; // bit 3
#if BMP390_DEBUG
	printf("BMP390::readIntStatus = 0x%02X  fwm=%u ffull=%u drdy=%u\n",
		reg, status.fifoWatermark, status.fifoFull, status.dataReady);
#endif
	return status;
}

// === FIFO configuration methods ===

/*!
	@brief Enable the FIFO and configure its behaviour.
	@details Writes FIFO_CFG_1 (0x17) and FIFO_CFG_2 (0x18).
	  FIFO_CFG_1 bit layout (Table 38):
	    bit 0 = fifo_mode          (1 = enable)
	    bit 1 = fifo_stop_on_full  (0 = streaming, 1 = stop-on-full)
	    bit 2 = fifo_time_en       (1 = append sensortime frame to burst read)
	    bit 3 = fifo_press_en      (1 = store pressure frames)
	    bit 4 = fifo_temp_en       (1 = store temperature frames)
	  FIFO_CFG_2 bit layout (Table 39):
	    bits [2:0] = fifo_subsampling  (factor = 2^value)
	    bits [4:3] = data_select       (00=unfiltered, 01=filtered)
	@param pressEn      Store pressure frames.
	@param tempEn       Store temperature frames.
	@param timeEn       Append sensortime frame.
	@param overflow     Streaming or stop-on-full.
	@param dataSelect   Unfiltered or filtered.
	@param subsampling  Down-sampling factor.
	@return True if both writes succeeded.
*/
bool BMP390_Sensor::enableFifo(bool              pressEn,
                                bool              tempEn,
                                bool              timeEn,
                                FifoOverflow_e    overflow,
                                FifoDataSelect_e  dataSelect,
                                FifoSubsampling_e subsampling)
{
	uint8_t cfg1 = 0;
	cfg1 |= 0x01u;                                            // bit 0
	cfg1 |= (static_cast<uint8_t>(overflow) & 0x01) << 1;     // bit 1
	cfg1 |= (timeEn  ? 0x04u : 0x00u);                        // bit 2
	cfg1 |= (pressEn ? 0x08u : 0x00u);                        // bit 3
	cfg1 |= (tempEn  ? 0x10u : 0x00u);                        // bit 4
	uint8_t cfg2 = 0;
	cfg2 |= (static_cast<uint8_t>(subsampling) & 0x07);       // bits [2:0]
	cfg2 |= (static_cast<uint8_t>(dataSelect)  & 0x03) << 3;  // bits [4:3]
#if BMP390_DEBUG
	printf("BMP390::enableFifo CFG1=0x%02X CFG2=0x%02X\n", cfg1, cfg2);
#endif
	bool ok = setRegister(FIFO_CFG_1, cfg1);
	ok     &= setRegister(FIFO_CFG_2, cfg2);
	return ok;
}

/*!
	@brief Disable the FIFO by clearing fifo_mode (bit 0 of FIFO_CFG_1).
	@details Data already in the FIFO remains readable.
	@return True if write succeeded.
*/
bool BMP390_Sensor::disableFifo()
{
	uint8_t cfg1 = readRegister(FIFO_CFG_1);
	cfg1 &= ~0x01u; // clear fifo_mode bit
	return setRegister(FIFO_CFG_1, cfg1);
}

/*!
	@brief Set the FIFO watermark level (9-bit value, 0–511 bytes).
	@details Both FIFO_WTM_0 (0x15) and FIFO_WTM_1 (0x16) must be written
	  in a single burst transaction as required by the datasheet section 3.7.5.1.
	  FIFO_WTM_0 holds bits [7:0].
	  FIFO_WTM_1 bit 0 holds bit 8 (the MSB of the 9-bit value).
	  A watermark of 0 disables the interrupt condition.
	@param watermarkBytes Threshold in bytes (max 511).
	@return True if write succeeded.
*/
bool BMP390_Sensor::setFifoWatermark(uint16_t watermarkBytes)
{
	if (watermarkBytes > 511u)
	{
		watermarkBytes = 511u;
	}
	uint8_t wtm0 = static_cast<uint8_t>(watermarkBytes & 0xFF);
	uint8_t wtm1 = static_cast<uint8_t>((watermarkBytes >> 8) & 0x01);

	if (_commMode == CommMode_e::I2C)
	{
		// I2C multi-byte write: address, then data pairs
		uint8_t data[4] = {
			static_cast<uint8_t>(FIFO_WTM_0), wtm0,
			static_cast<uint8_t>(FIFO_WTM_1), wtm1
		};
		int16_t ret = i2c_write_timeout_us(_i2cInst, _address, data, 4, false, _timeOutDelay);
		return (ret == 4);
	}
	else
	{
		// SPI multi-byte write: [addr0, val0, addr1, val1]
		uint8_t data[4] = {
			static_cast<uint8_t>(FIFO_WTM_0 & _SPI_WRITE_MASK), wtm0,
			static_cast<uint8_t>(FIFO_WTM_1 & _SPI_WRITE_MASK), wtm1
		};
		gpio_put(_cs, false);
		spi_write_blocking(_spiInst, data, 4);
		gpio_put(_cs, true);
		return true;
	}
}

/*!
	@brief Read current FIFO fill level in bytes (9-bit, 0–512).
	@details FIFO_LEN_0 (0x12) = bits [7:0].
	  FIFO_LEN_1 (0x13) bit 0 = bit 8 (MSB).
	@return Fill level in bytes.
*/
uint16_t BMP390_Sensor::getFifoLength()
{
	uint8_t buf[2] = {0};
	burstReadRegisters(FIFO_LEN_0, buf, 2);
	uint16_t len = (uint16_t)buf[0] | ((uint16_t)(buf[1] & 0x01) << 8);
	return len;
}

/*!
	@brief Flush all data from the FIFO without changing FIFO_CONFIG registers.
	@details Issues CMD = 0xB0 (fifo_flush, datasheet Table 48).
*/
void BMP390_Sensor::flushFifo()
{
	setRegister(CMD, 0xB0);
	busy_wait_ms(2);
}

/*!
	@brief Burst-read exactly length bytes from the FIFO_DATA register (0x14).
	@details During a burst read the address counter stops incrementing when
	  the FIFO_DATA address is reached (datasheet section 3.6.3), so repeated
	  reads from the same start address stream all FIFO content automatically.
	@param buffer Destination byte array.
	@param length Number of bytes to read.
	@return True if successful.
*/
bool BMP390_Sensor::burstReadFifo(uint8_t *buffer, uint16_t length)
{
	if (length == 0) return true;

	uint8_t reg[1] = {static_cast<uint8_t>(FIFO_DATA)};

	if (_commMode == CommMode_e::I2C)
	{
		int16_t ret = i2c_write_timeout_us(_i2cInst, _address, reg, 1, true, _timeOutDelay);
		if (ret < 1) return false;
		ret = i2c_read_timeout_us(_i2cInst, _address, buffer, length, false, _timeOutDelay);
		return (ret == (int16_t)length);
	}
	else
	{
		uint8_t dummy = 0;
		reg[0] = static_cast<uint8_t>(FIFO_DATA) | _SPI_READ_MASK;
		gpio_put(_cs, false);
		spi_write_blocking(_spiInst, reg, 1);
		spi_read_blocking(_spiInst, 0, &dummy, 1); // mandatory dummy byte (datasheet section 5.3.2)
		spi_read_blocking(_spiInst, 0, buffer, length);
		gpio_put(_cs, true);
		return true;
	}
}

/*!
	@brief Read and parse all available sensor frames from the FIFO.
	@details Frame header byte encoding (datasheet section 3.6.5, Tables 12-18):
	  Header byte bit layout for sensor frames (fh_mode[7:6] = 10):
	    bit 7 = 1, bit 6 = 0 (sensor frame marker)
	    bit 5 = s  (sensortime flag)
	    bit 4 = t  (temperature flag)
	    bit 3 = 0  (reserved)
	    bit 2 = p  (pressure flag)
	    bits [1:0] = 00
	  Known header byte values:
	    0x94  (10 0 1 0 1 00) = pressure + temperature frame (7 bytes: 1 hdr + 3 temp + 3 press)
	    0x90  (10 0 1 0 0 00) = temperature-only frame       (4 bytes: 1 hdr + 3 temp)
	    0x84  (10 0 0 0 1 00) = pressure-only frame          (4 bytes: 1 hdr + 3 press)
	    0xA0  (10 1 0 0 0 00) = sensortime frame             (4 bytes: 1 hdr + 3 time)
	    0x80  (10 0 0 0 0 00) = empty frame                  (2 bytes: 1 hdr + 1 zero)
	    0x44  (01 0 0 0 1 00) = control: config error        (2 bytes: 1 hdr + 1 opcode)
	    0x48  (01 0 0 1 0 00) = control: config change       (2 bytes: 1 hdr + 1 opcode)
	@param frames    Caller-allocated array for results.
	@param maxFrames Capacity of the array.
	@return Number of frames written (≤ maxFrames).
*/
uint8_t BMP390_Sensor::readFifoFrames(FifoFrame_t *frames, uint8_t maxFrames)
{
	if (frames == nullptr || maxFrames == 0) return 0;

	// Read current fill level
	uint16_t fifoLen = getFifoLength();
	if (fifoLen == 0) return 0;
	// Cap at 512 bytes (maximum FIFO size)
	if (fifoLen > 512u) fifoLen = 512u;
	// Burst-read all available FIFO bytes into a local buffer
	uint8_t buf[512] = {0};
	if (!burstReadFifo(buf, fifoLen)) return 0;
	uint8_t  frameCount = 0;
	uint16_t i          = 0u;
	while (i < fifoLen && frameCount < maxFrames)
	{
		uint8_t hdr = buf[i];
		i++;

		if (hdr == FIFO_HDR_PRESS_TEMP)
		{
			// Pressure + temperature frame: 3 bytes temp then 3 bytes press (6 bytes total)
			if (i + 6u > fifoLen) break;
			int32_t rawT = ((int32_t)buf[i+2] << 16) | ((int32_t)buf[i+1] << 8) | buf[i];
			i += 3u;
			int32_t rawP = ((int32_t)buf[i+2] << 16) | ((int32_t)buf[i+1] << 8) | buf[i];
			i += 3u;
			frames[frameCount].hasTemperature = true;
			frames[frameCount].hasPressure    = true;
			frames[frameCount].hasSensortime  = false;
			frames[frameCount].temperature    = compensateTemperature(rawT);
			frames[frameCount].pressure       = compensatePressure(rawP);
			frames[frameCount].sensortime     = 0u;
			frameCount++;
		}
		else if (hdr == FIFO_HDR_TEMP_ONLY)
		{
			// Temperature-only frame: 3 data bytes
			if (i + 3u > fifoLen) break;

			int32_t rawT = ((int32_t)buf[i+2] << 16) | ((int32_t)buf[i+1] << 8) | buf[i];
			i += 3u;
			frames[frameCount].hasTemperature = true;
			frames[frameCount].hasPressure    = false;
			frames[frameCount].hasSensortime  = false;
			frames[frameCount].temperature    = compensateTemperature(rawT);
			frames[frameCount].pressure       = 0.0;
			frames[frameCount].sensortime     = 0u;
			frameCount++;
		}
		else if (hdr == FIFO_HDR_PRESS_ONLY)
		{
			// Pressure-only frame: 3 data bytes.
			// Temperature must have been compensated previously for _tLin to be valid.
			if (i + 3u > fifoLen) break;
			int32_t rawP = ((int32_t)buf[i+2] << 16) | ((int32_t)buf[i+1] << 8) | buf[i];
			i += 3u;
			frames[frameCount].hasTemperature = false;
			frames[frameCount].hasPressure    = true;
			frames[frameCount].hasSensortime  = false;
			frames[frameCount].temperature    = 0.0;
			frames[frameCount].pressure       = compensatePressure(rawP);
			frames[frameCount].sensortime     = 0u;
			frameCount++;
		}
		else if (hdr == FIFO_HDR_SENSORTIME)
		{
			// Sensortime frame: 3 data bytes (LSB first)
			if (i + 3u > fifoLen) break;
			uint32_t st = ((uint32_t)buf[i+2] << 16) | ((uint32_t)buf[i+1] << 8) | buf[i];
			i += 3u;
			frames[frameCount].hasTemperature = false;
			frames[frameCount].hasPressure    = false;
			frames[frameCount].hasSensortime  = true;
			frames[frameCount].temperature    = 0.0;
			frames[frameCount].pressure       = 0.0;
			frames[frameCount].sensortime     = st;
			frameCount++;
		}
		else if (hdr == FIFO_HDR_EMPTY)
		{
			// Empty frame: 1 data byte (all zeros) — skip it
			if (i + 1u > fifoLen) break;
			i++;
		}
		else if (hdr == FIFO_HDR_CFG_ERR || hdr == FIFO_HDR_CFG_CHG)
		{
			// Control frame: 1 opcode byte — skip silently
			if (i + 1u > fifoLen) break;
			i++;
		}
		else
		{
			// Unknown header — cannot safely continue parsing
#if BMP390_DEBUG
			printf("BMP390::readFifoFrames: unknown header 0x%02X at offset %u\n", hdr, i - 1u);
#endif
			break;
		}
	}

	return frameCount;
}
// === End of file ===
