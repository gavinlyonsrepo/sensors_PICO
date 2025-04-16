/*!
 * @file ahtxx.cpp
 * @brief library for Aosong ASAIR AHT10,
 *        AHT15 AHT20 Digital Humidity & Temperature Sensor
 *        library implementation file
 */

#include "../../include/ahtxx/ahtxx.hpp"

/*! 
	@brief Constructor for LIB_AHTXX class, init the sensor data types call before begin()
	@param address I2C address of the device (7-bit)
	@param i2c_type I2C instance of port IC20 or I2C1
	@param SDApin I2C Data pin
	@param SCLKpin I2C Clock pin
	@param CLKspeed I2C Bus Clock speed in KHz. Typically 100-400
*/
LIB_AHTXX::LIB_AHTXX(uint8_t address, i2c_inst_t* i2c_type, uint8_t  SDApin, uint8_t  SCLKpin, uint16_t CLKspeed) {
	_address = address;
	 i2c = i2c_type; 
    _SClkPin = SCLKpin;
    _SDataPin = SDApin;
    _CLKSpeed = CLKspeed;
}

/*!
	 * @brief Initialize the I2C interface for the AHT10 sensor
	 * @param sensorName The type of ASAIR I2C sensor being used
	 * @note This function must be called before using the sensor begin method
*/
void LIB_AHTXX::AHT10_InitI2C(ASAIR_I2C_SENSOR_e sensorName) {
	_sensorName = sensorName;
	//init I2C
	gpio_set_function(_SDataPin, GPIO_FUNC_I2C);
    gpio_set_function(_SClkPin, GPIO_FUNC_I2C);
	gpio_pull_up(_SDataPin);
    gpio_pull_up(_SClkPin);
	i2c_init(i2c, _CLKSpeed * 1000);
}

 /*! 
 	@brief Initialize I2C & configure the sensor, call this function before
 	@note This function must be called before using the sensor begin method
 	@return bool true init success False failure
 */

bool LIB_AHTXX::AHT10_begin()
{
	busy_wait_ms(AHT10_POWER_ON_DELAY);    //wait for sensor to initialize
	AHT10_setNormalMode();                //one measurement+sleep mode
	if (AHT10_enableFactoryCalCoeff()) //load factory calibration coeff
		isConnected = true;
	else 
		isConnected = false;
	return isConnected; 
}

/*!
 * @brief Read raw measurement data from sensor over I2C
 * @return uint8_t AHT10_ERROR for failure, true for success with data in the buffer
 */
uint8_t LIB_AHTXX::AHT10_readRawData() {

	uint8_t bufTX[3];
	bufTX[0] = AHT10_START_MEASURMENT_CMD;
	bufTX[1] = AHT10_DATA_MEASURMENT_CMD;
	bufTX[2] = AHT10_DATA_NOP;
	// 	Send measurement command
	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false, AHT10_MY_I2C_DELAY );
	if (returnValue < 1 )
		return AHT10_ERROR; //error handler, collision on I2C bus

	if (AHT10_getCalibrationBit(AHT10_FORCE_READ_DATA) != 0x01)
		return AHT10_ERROR;  // error handler, calibration coefficient turned off
	if (AHT10_getBusyBit(AHT10_USE_READ_DATA) != 0x00)
		busy_wait_ms(AHT10_MEASURMENT_DELAY); // measurement delay

	// Read 6-bytes from sensor
	returnValue = i2c_read_timeout_us(i2c, _address, _rawDataBuffer, 6 ,false, AHT10_MY_I2C_DELAY  );
	if (returnValue < 1) {
		_rawDataBuffer[0] = AHT10_ERROR;
		return AHT10_ERROR;
	}

	return true;
}

/*!
 * @brief Read temperature, °C
 * @param readI2C use last data or new
 * @return failure  AHT10_ERROR ,  success temperature as floating point
 * NOTES:
 * temperature range      -40°C..+80°C
 * temperature resolution 0.01°C
 * temperature accuracy   ±0.3°C
*/
float LIB_AHTXX::AHT10_readTemperature(bool readI2C) {
	if (readI2C == AHT10_FORCE_READ_DATA) {
		if (AHT10_readRawData() == AHT10_ERROR)
			return AHT10_ERROR; //force to read data to _rawDataBuffer & error handler
	}

	if (_rawDataBuffer[0] == AHT10_ERROR)
		return AHT10_ERROR; //error handler, collision on I2C bus

	uint32_t temperature = ((uint32_t) (_rawDataBuffer[3] & 0x0F) << 16)
			| ((uint16_t) _rawDataBuffer[4] << 8) | _rawDataBuffer[5]; //20-bit raw temperature data

	return (float)(temperature * 0.000191 - 50);
}

/*!
 * @brief Read humidity, %
 * @param readI2C use last data or new
 * @return failure  AHT10_ERROR ,  success humidity as floating point
 * NOTES:
 * relative humidity range      0%..100%
 * relative humidity resolution 0.024%
 * relative humidity accuracy   ±2%
*/
float LIB_AHTXX::AHT10_readHumidity(bool readI2C) {
	if (readI2C == AHT10_FORCE_READ_DATA) {
		if (AHT10_readRawData() == AHT10_ERROR)
			return AHT10_ERROR; //force to read data to _rawDataBuffer & error handler
	}

	if (_rawDataBuffer[0] == AHT10_ERROR)
		return AHT10_ERROR; //error handler, collision on I2C bus

	uint32_t rawData = (((uint32_t) _rawDataBuffer[1] << 16)
			| ((uint16_t) _rawDataBuffer[2] << 8) | (_rawDataBuffer[3])) >> 4; //20-bit raw humidity data

	float humidity = (float) rawData * 0.000095;

	if (humidity < 0)
		return 0;
	if (humidity > 100)
		return 100;
	return humidity;
}

/*!
 * @brief Soft reset the AHT10 sensor
 * @return bool true for success, false for failure
 * @note This function resets the sensor without power cycling it.
 *       It takes approximately 20ms to complete.
 *       All registers are restored to default values.
 */
bool LIB_AHTXX::AHT10_softReset(void) {

	uint8_t bufTX[1];
	bufTX[0]= AHT10_SOFT_RESET_CMD;

	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 1 ,false , AHT10_MY_I2C_DELAY );
	if (returnValue < 1)
		return false;

	busy_wait_ms(AHT10_SOFT_RESET_DELAY);
	AHT10_setNormalMode();                 //reinitialize sensor registers after reset
	return AHT10_enableFactoryCalCoeff();  //reinitialize sensor registers after reset
}


/*!
 * @brief Set normal measurement mode for the AHT10 sensor
 * @return bool true for success, false for failure
*/
bool LIB_AHTXX::AHT10_setNormalMode(void) {
	uint8_t bufTX[3];

	bufTX[0] = AHT10_NORMAL_CMD;
	bufTX[1] = AHT10_DATA_NOP;
	bufTX[2] = AHT10_DATA_NOP;

	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false , AHT10_MY_I2C_DELAY );
	if (returnValue < 1)
		return false; //safety check, make sure transmission complete

	busy_wait_ms(AHT10_CMD_DELAY);

	return true;
}

/*!
	@brief Set cycle measurement mode for the AHT10 sensor
	@note This function configures the sensor for continuous measurement mode.
	@return bool true for success, false for failure
*/
bool LIB_AHTXX::AHT10_setCycleMode(void) {
	uint8_t bufTX[3];

	if (_sensorName != AHT20_SENSOR)
		bufTX[0] = AHT10_INIT_CMD;
	else
		bufTX[0] = AHT20_INIT_CMD;
	bufTX[1] = AHT10_INIT_CYCLE_MODE | AHT10_INIT_CAL_ENABLE;
	bufTX[2] = AHT10_DATA_NOP;
	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false, AHT10_MY_I2C_DELAY  );

	if (returnValue < 1)
		return false; //safety check, make sure transmission complete


	return true;
}

/*! 
 * @brief Read status byte from sensor over I2C
 * @return Status byte success or AHT10_ERROR failure
 */
uint8_t LIB_AHTXX::AHT10_readStatusByte() {

	returnValue = i2c_read_timeout_us(i2c, _address, _rawDataBuffer, 1 ,false, AHT10_MY_I2C_DELAY );

	if (returnValue < 1) {
		_rawDataBuffer[0] = AHT10_ERROR;
		return AHT10_ERROR;
	}

	return _rawDataBuffer[0];

}

/*!
 * @brief Read Calibration bit from status byte
 * @param readI2C use last data or force new read
 * @return The calibration bit for success or AHT10_ERROR for error
 * @note 
 * 	- 0, factory calibration coeff disabled
 * 	- 1, factory calibration coeff loaded
 */
uint8_t LIB_AHTXX::AHT10_getCalibrationBit(bool readI2C) {
	uint8_t valueBit;
	if (readI2C == AHT10_FORCE_READ_DATA)
		_rawDataBuffer[0] = AHT10_readStatusByte(); //force to read status byte

	if (_rawDataBuffer[0] != AHT10_ERROR)
	{
		valueBit = (_rawDataBuffer[0] & 0x08);
		return (valueBit>>3); //get 3-rd bit 0001000
	}else{
		return AHT10_ERROR;
	}
}

/*!
 * @brief Enable factory calibration coefficients
 * @return bool true for success, false for failure
 * @note This function loads the factory calibration coefficients into the sensor.
*/
bool LIB_AHTXX::AHT10_enableFactoryCalCoeff() {

	uint8_t bufTX[3];
	if (_sensorName != AHT20_SENSOR)
		bufTX[0] = AHT10_INIT_CMD;
	else
		bufTX[0] = AHT20_INIT_CMD;

	bufTX[1] = AHT10_INIT_CAL_ENABLE;
	bufTX[2] = AHT10_DATA_NOP;

	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false, AHT10_MY_I2C_DELAY  );
	
	if (returnValue < 1)
	{
		return false; //safety check, make sure transmission complete
	}
	busy_wait_ms(AHT10_CMD_DELAY);

	/*check calibration enable */
	if (AHT10_getCalibrationBit(AHT10_FORCE_READ_DATA) == 0x01){
		return true;
	}else{;
		return false;
	}

}

/*!
 * @brief Read busy bit from status byte
 * @param readI2C use last data or force new read
 * @return The busy bit for success or AHT10_ERROR for error
 * @note 
 * 	- 0, sensor idle & sleeping
 * 	- 1, sensor busy & in measurement state
*/
uint8_t LIB_AHTXX::AHT10_getBusyBit(bool readI2C) {
	uint8_t valueBit;
	if (readI2C == AHT10_FORCE_READ_DATA)
		_rawDataBuffer[0] = AHT10_readStatusByte(); // Read status byte

	if (_rawDataBuffer[0] != AHT10_ERROR)
	{
		valueBit = (_rawDataBuffer[0] & 0x80);
		return (valueBit>>7);; //get 7-th bit 1000 0000  0x80
	}
	else{
		return AHT10_ERROR;
	}
}

/*!
 * @brief De-initialize the AHT10 sensor
*/
void LIB_AHTXX::AHT10_DeInit()
{
	gpio_set_function(_SDataPin, GPIO_FUNC_NULL);
    gpio_set_function(_SClkPin, GPIO_FUNC_NULL);
	i2c_deinit(i2c); 	
}

/*!
 * @brief Set the connection status of the AHT10 sensor
 * @param connected The connection status to set
*/
void LIB_AHTXX::AHT10_SetIsConnected(bool connected)
{
	isConnected = connected;
}

/*!
 * @brief Get the connection status of the AHT10 sensor
 * @return bool The connection status
*/
bool LIB_AHTXX::AHT10_GetIsConnected(void)
{
	return isConnected;
}