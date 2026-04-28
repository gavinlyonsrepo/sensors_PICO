/*!
	@file bmp390.hpp
	@brief library header file for bmp390 pressure sensor
*/

#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include <cmath>

#define BMP390_DEBUG 0 // Set to 1 for verbose debug output during development, 0 for normal operation

class BMP390_Sensor
{
public:
	BMP390_Sensor(spi_inst_t *spi, uint32_t baudRate, uint8_t cs, uint8_t mosi, uint8_t sck, uint8_t miso);
	BMP390_Sensor(i2c_inst_t *i2c, uint32_t baudrate, uint32_t timeOutDelay, uint8_t address, uint8_t sda, uint8_t scl);
	~BMP390_Sensor();

	/*! @brief Communications mode */
	enum class CommMode_e : uint8_t
	{
		SPI = 0, /**< SPI communication mode */
		I2C = 1  /**< I2C communication mode */
	};

	/*! @brief Enumeration for BMP390 power modes */
	enum class PowerMode_e : uint8_t
	{
		Sleep  = 0, /**< No measurements are performed */
		Forced = 1, /**< Single measurement, then returns to sleep */
		Normal = 3  /**< Perpetual cycling between measurement and standby */
	};

	/*! @brief Enumeration for BMP390 data types */
	enum class DataType_e : uint8_t
	{
		Temperature = 0, /**< Temperature */
		Pressure    = 1  /**< Pressure */
	};

	/*! @brief Enumeration for BMP390 registers (datasheet Table 25) */
	enum Registers_e : uint8_t
	{
		Chip_ID      = 0x00, /**< Chip ID register */
		ERR_REG      = 0x02, /**< Error register */
		Status       = 0x03, /**< Device status register */
		Data_0       = 0x04, /**< Pressure XLSB [7:0] */
		Data_1       = 0x05, /**< Pressure LSB  [15:8] */
		Data_2       = 0x06, /**< Pressure MSB  [23:16] */
		Data_3       = 0x07, /**< Temperature XLSB [7:0] */
		Data_4       = 0x08, /**< Temperature LSB  [15:8] */
		Data_5       = 0x09, /**< Temperature MSB  [23:16] */
		SensorTime_0 = 0x0C, /**< Sensortime bits [7:0] */
		SensorTime_1 = 0x0D, /**< Sensortime bits [15:8] */
		SensorTime_2 = 0x0E, /**< Sensortime bits [23:16] */
		Event        = 0x10, /**< Sensor event register */
		INT_Status   = 0x11, /**< Interrupt status register */
		FIFO_LEN_0   = 0x12, /**< FIFO byte counter [7:0] */
		FIFO_LEN_1   = 0x13, /**< FIFO byte counter [11:8] (9-bit: only bit 0 used) */
		FIFO_DATA    = 0x14, /**< FIFO data output (address auto-holds at this register during burst read) */
		FIFO_WTM_0   = 0x15, /**< FIFO watermark [7:0] */
		FIFO_WTM_1   = 0x16, /**< FIFO watermark bit 8 (9-bit watermark) */
		FIFO_CFG_1   = 0x17, /**< FIFO config 1: mode, stop_on_full, time_en, press_en, temp_en */
		FIFO_CFG_2   = 0x18, /**< FIFO config 2: data_select, fifo_subsampling */
		INT_CTRL     = 0x19, /**< Interrupt configuration */
		IF_CONF      = 0x1A, /**< Serial interface configuration */
		PWR_CTRL     = 0x1B, /**< Power control: mode, temp_en, press_en */
		OSR          = 0x1C, /**< Oversampling: osr_t [5:3], osr_p [2:0] */
		ODR          = 0x1D, /**< Output data rate: odr_sel [4:0] */
		Config       = 0x1F, /**< IIR filter coefficient: iir_filter [3:1] */
		CALIB_DATA   = 0x31, /**< Trimming parameters start (21 bytes, 0x31-0x45) */
		CMD          = 0x7E  /**< Command register (0xB0=fifo_flush, 0xB6=softreset) */
	};

	/*! @brief Enumeration for pressure unit */
	enum class PressureUnit_e : uint8_t
	{
		Pa  = 0, /**< Pascal */
		hPa = 1  /**< Hectopascal */
	};

	/*! @brief Enumeration for sensor oversampling (datasheet Table 43) */
	enum class sensorSampling_e : uint8_t
	{
		Sampling_None = 0x00, /**< ×1 (no oversampling) */
		Sampling_X1   = 0x01, /**< ×1  */
		Sampling_X2   = 0x02, /**< ×2  */
		Sampling_X4   = 0x03, /**< ×4  */
		Sampling_X8   = 0x04, /**< ×8  */
		Sampling_X16  = 0x05, /**< ×16 */
		Sampling_X32  = 0x06  /**< ×32 */
	};

	/*! @brief IIR filter coefficient (datasheet Table 46, CONFIG register bits [3:1]) */
	enum Filter_e : uint8_t
	{
		Filter_OFF  = 0x00, /**< Filter off (bypass) */
		Filter_X1   = 0x01, /**< Coefficient 1   */
		Filter_X3   = 0x02, /**< Coefficient 3   */
		Filter_X7   = 0x03, /**< Coefficient 7   */
		Filter_X15  = 0x04, /**< Coefficient 15  */
		Filter_X31  = 0x05, /**< Coefficient 31  */
		Filter_X63  = 0x06, /**< Coefficient 63  */
		Filter_X127 = 0x07  /**< Coefficient 127 */
	};

	/*! @brief Output data rate selection (datasheet Table 45) */
	enum ODR_e : uint8_t
	{
		ODR_200_Hz    = 0x00, /**< 200 Hz    — period   5 ms */
		ODR_100_Hz    = 0x01, /**< 100 Hz    — period  10 ms */
		ODR_50_Hz     = 0x02, /**<  50 Hz    — period  20 ms */
		ODR_25_Hz     = 0x03, /**<  25 Hz    — period  40 ms */
		ODR_12_5_Hz   = 0x04, /**<  12.5 Hz  — period  80 ms */
		ODR_6_25_Hz   = 0x05, /**<   6.25 Hz — period 160 ms */
		ODR_3_1_Hz    = 0x06, /**<   3.1 Hz  — period 320 ms */
		ODR_1_5_Hz    = 0x07, /**<   1.5 Hz  — period 640 ms */
		ODR_0_78_Hz   = 0x08, /**<  0.78 Hz  — period  1.28 s */
		ODR_0_39_Hz   = 0x09, /**<  0.39 Hz  — period  2.56 s */
		ODR_0_2_Hz    = 0x0A, /**<  0.2 Hz   — period  5.12 s */
		ODR_0_1_Hz    = 0x0B, /**<  0.1 Hz   — period 10.24 s */
		ODR_0_05_Hz   = 0x0C, /**<  0.05 Hz  — period 20.48 s */
		ODR_0_02_Hz   = 0x0D, /**<  0.02 Hz  — period 40.96 s */
		ODR_0_01_Hz   = 0x0E, /**<  0.01 Hz  — period 81.92 s */
		ODR_0_006_Hz  = 0x0F, /**< 0.006 Hz  — period 163.84 s */
		ODR_0_003_Hz  = 0x10, /**< 0.003 Hz  — period 327.68 s */
		ODR_0_0015_Hz = 0x11  /**< 0.0015 Hz — period 655.36 s */
	};

	
	// === Interrupt enumerations ===

	/*! @brief INT pin output type (INT_CTRL bit 0: int_od) */
	enum class IntOutputMode_e : uint8_t
	{
		PushPull  = 0, /**< Push-pull (default after reset) */
		OpenDrain = 1  /**< Open-drain — requires external pull-up */
	};

	/*! @brief INT pin active level (INT_CTRL bit 1: int_level) */
	enum class IntLevel_e : uint8_t
	{
		ActiveLow  = 0, /**< INT asserted when pin is LOW  */
		ActiveHigh = 1  /**< INT asserted when pin is HIGH (default after reset) */
	};

	/*! @brief INT pin and INT_STATUS latch mode (INT_CTRL bit 2: int_latch) */
	enum class IntLatch_e : uint8_t
	{
		NonLatched = 0, /**< INT de-asserts automatically when condition clears */
		Latched    = 1  /**< INT stays asserted until INT_STATUS is read */
	};

	/*! @brief Interrupt status flags read from INT_STATUS register (0x11).
	 *  @details INT_STATUS is cleared on read (datasheet section 4.3.9).
	 *    Bit 0 = fwm_int  (FIFO watermark)
	 *    Bit 1 = ffull_int (FIFO full)
	 *    Bit 3 = drdy      (data ready)
	 */
	struct IntStatus_t
	{
		bool fifoWatermark; /**< FIFO fill level ≥ watermark threshold */
		bool fifoFull;      /**< FIFO fill level ≥ 504 bytes */
		bool dataReady;     /**< Pressure and temperature conversion complete */
	};

	// === FIFO enumerations ===

	/*! @brief FIFO overflow behaviour (FIFO_CFG_1 bit 1: fifo_stop_on_full) */
	enum class FifoOverflow_e : uint8_t
	{
		Streaming  = 0, /**< Oldest frame deleted to make room for newest (circular) */
		StopOnFull = 1  /**< Newest frame discarded when FIFO is full */
	};

	/*! @brief FIFO data source (FIFO_CFG_2 bits [4:3]: data_select) */
	enum class FifoDataSelect_e : uint8_t
	{
		Unfiltered = 0x00, /**< Unfiltered ADC data stored in FIFO */
		Filtered   = 0x01  /**< IIR-filtered data stored in FIFO */
	};

	/*! @brief FIFO downsampling factor (FIFO_CFG_2 bits [2:0]: fifo_subsampling).
	 *  @details Factor = 2^value. Normal mode only. */
	enum class FifoSubsampling_e : uint8_t
	{
		Div1   = 0x00, /**< No subsampling (every measurement stored) */
		Div2   = 0x01, /**< Store every 2nd  measurement */
		Div4   = 0x02, /**< Store every 4th  measurement */
		Div8   = 0x03, /**< Store every 8th  measurement */
		Div16  = 0x04, /**< Store every 16th measurement */
		Div32  = 0x05, /**< Store every 32nd measurement */
		Div64  = 0x06, /**< Store every 64th measurement */
		Div128 = 0x07  /**< Store every 128th measurement */
	};

	/*! @brief A single parsed FIFO sensor frame.
	 *  @details Call readFifoFrames() to populate an array of these.
	 *    Fields that are not present in a given frame are left at 0 / false.
	 */
	struct FifoFrame_t
	{
		bool     hasTemperature; /**< Frame contains temperature data */
		bool     hasPressure;    /**< Frame contains pressure data */
		bool     hasSensortime;  /**< Frame is a sensortime frame */
		double   temperature;    /**< Compensated temperature in °C (if hasTemperature) */
		double   pressure;       /**< Compensated pressure in Pa   (if hasPressure)    */
		uint32_t sensortime;     /**< Raw 24-bit sensortime counter (if hasSensortime)  */
	};

	// Sensor init / deinit
	void InitSensor(void);
	void DeInitSensor(void);
	int16_t CheckConnectionI2C(void);
	// Chip ID and raw register access
	uint8_t readForChipID();
	uint8_t getChipID() const;
	bool    setRegister(Registers_e reg, uint8_t config, bool check = false);
	uint8_t readRegister(Registers_e reg);
	// power mode
	bool        setPowerMode(PowerMode_e mode, bool check = false);
	PowerMode_e readPowerMode();
	PowerMode_e getPowerMode() const;
	void        reset();
	// measurement configuration
	bool             setOversampling(DataType_e type, sensorSampling_e oversampling, bool check = false);
	sensorSampling_e readOversampling(DataType_e type);
	sensorSampling_e getOversampling(DataType_e type) const;
	
	bool     setIIRFilter(Filter_e filter, bool check = false);
	Filter_e readIIRFilter();
	Filter_e getIIRFilter() const;

	bool  setODR(ODR_e odr, bool check = false);
	ODR_e readODR();
	ODR_e getODR() const;

	bool takeForcedMeasurement(uint32_t timeoutMs = 500);
	bool readSensorData(void);

	// temperature and pressure readout
	int32_t readRawTemperature();
	int32_t getRawTemperature() const;
	double  readTemperature();
	double  getTemperature() const;

	uint32_t readRawPressure();
	uint32_t getRawPressure() const;
	double   readPressure(PressureUnit_e unit = PressureUnit_e::Pa);
	double   getPressure(PressureUnit_e unit = PressureUnit_e::Pa) const;

	double readAltitude(double seaLevelhPa);
	double seaLevelForAltitude(double altitude, double atmospheric);

	// interrupt configuration and status
	bool configureInterrupt(IntOutputMode_e outputMode = IntOutputMode_e::PushPull,
	                        IntLevel_e      level      = IntLevel_e::ActiveHigh,
	                        IntLatch_e      latch      = IntLatch_e::NonLatched,
	                        bool            drdyEn     = false,
	                        bool            ffullEn    = false,
	                        bool            fwtmEn     = false,
	                        bool            check      = false);

	IntStatus_t readIntStatus();

	// FIFO  methods 
	bool enableFifo(bool             pressEn     = true,
	                bool             tempEn      = true,
	                bool             timeEn      = false,
	                FifoOverflow_e   overflow    = FifoOverflow_e::Streaming,
	                FifoDataSelect_e dataSelect  = FifoDataSelect_e::Unfiltered,
	                FifoSubsampling_e subsampling = FifoSubsampling_e::Div1);
	bool disableFifo();
	bool setFifoWatermark(uint16_t watermarkBytes);
	uint16_t getFifoLength();
	void flushFifo();
	uint8_t readFifoFrames(FifoFrame_t *frames, uint8_t maxFrames);

private:
	i2c_inst_t *_i2cInst;   /**< I2C instance */
	uint32_t _baudrate;      /**< I2C baudrate in Hz */
	uint32_t _timeOutDelay;  /**< I2C timeout delay in uS */
	uint8_t _address;        /**< I2C address (0x76 or 0x77) */
	spi_inst_t *_spiInst;    /**< SPI instance */
	uint32_t _baudRate;      /**< SPI baudrate in Hz */
	static constexpr uint8_t _SPI_READ_MASK  = 0x80; /**< Bit 7 set for SPI read */
	static constexpr uint8_t _SPI_WRITE_MASK = 0x7F; /**< Bit 7 clear for SPI write */
	uint8_t _cs;             /**< Chip select pin, SPI only */
	uint8_t _mosi;           /**< MOSI pin (SPI) or SDA pin (I2C) */
	uint8_t _sck;            /**< Serial clock pin */
	uint8_t _miso;           /**< MISO pin, SPI only */

	uint8_t _chipID;                            /**< Chip ID: 0x60 = BMP390 / BMP390L */
	sensorSampling_e _temperatureOversampling;  /**< Temperature oversampling setting */
	sensorSampling_e _pressureOversampling;     /**< Pressure oversampling setting */
	PowerMode_e _powerMode;                     /**< Current power mode */
	CommMode_e _commMode;                       /**< Active communication mode */
	Filter_e _iirFilter;                        /**< IIR filter coefficient setting */
	ODR_e _odr;                                 /**< Output data rate setting */

	int32_t _rawTemperature;  /**< Raw (uncompensated) temperature ADC output */
	double _temperature;      /**< Compensated temperature in degrees Celsius */
	double _tLin;             /**< Linearised temperature used in pressure compensation */
	uint32_t _rawPressure;    /**< Raw (uncompensated) pressure ADC output */
	double _pressure;         /**< Compensated pressure in Pa */
	int16_t OverSamplingDelay = 100; /**< Oversampling settling delay in milliseconds */

	/*! @brief FIFO frame header byte constants (datasheet section 3.6.5) */
	static constexpr uint8_t FIFO_HDR_PRESS_TEMP = 0x94; /**< Sensor frame: press + temp (6 bytes) */
	static constexpr uint8_t FIFO_HDR_TEMP_ONLY  = 0x90; /**< Sensor frame: temp only    (3 bytes) */
	static constexpr uint8_t FIFO_HDR_PRESS_ONLY = 0x84; /**< Sensor frame: press only   (3 bytes) */
	static constexpr uint8_t FIFO_HDR_SENSORTIME = 0xA0; /**< Sensortime frame            (3 bytes) */
	static constexpr uint8_t FIFO_HDR_EMPTY      = 0x80; /**< Empty frame                 (1 byte ) */
	static constexpr uint8_t FIFO_HDR_CFG_ERR    = 0x44; /**< Control: config error       (1 opcode)*/
	static constexpr uint8_t FIFO_HDR_CFG_CHG    = 0x48; /**< Control: config change      (1 opcode)*/

	/*!
	 * @brief Trimming calibration data (NVM registers 0x31-0x45, Table 24).
	 */
	struct
	{
		uint16_t nvm_par_t1;  /**< T1 — 16-bit unsigned */
		uint16_t nvm_par_t2;  /**< T2 — 16-bit unsigned */
		int8_t   nvm_par_t3;  /**< T3 —  8-bit signed   */
		int16_t  nvm_par_p1;  /**< P1 — 16-bit signed   */
		int16_t  nvm_par_p2;  /**< P2 — 16-bit signed   */
		int8_t   nvm_par_p3;  /**< P3 —  8-bit signed   */
		int8_t   nvm_par_p4;  /**< P4 —  8-bit signed   */
		uint16_t nvm_par_p5;  /**< P5 — 16-bit unsigned */
		uint16_t nvm_par_p6;  /**< P6 — 16-bit unsigned */
		int8_t   nvm_par_p7;  /**< P7 —  8-bit signed   */
		int8_t   nvm_par_p8;  /**< P8 —  8-bit signed   */
		int16_t  nvm_par_p9;  /**< P9 — 16-bit signed   */
		int8_t   nvm_par_p10; /**< P10 —  8-bit signed  */
		int8_t   nvm_par_p11; /**< P11 —  8-bit signed  */
	} calib_data_t;

	bool   getTrimmingParameters(void);
	bool   waitCmdReady(uint32_t timeoutMs = 100);
	bool   isOSRODRValid(sensorSampling_e tempOS, sensorSampling_e pressOS, ODR_e odr);
	bool   burstReadFifo(uint8_t *buffer, uint16_t length);
	bool   burstReadRegisters(Registers_e startReg, uint8_t *buffer, uint8_t length);
	double compensateTemperature(int32_t rawTemp);
	double compensatePressure(int32_t rawPress);
	void   StartUpRoutine(void);
};

