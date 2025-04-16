/*!
 * @file   lm75.cpp
 * @brief  Library for the LM75A temperature sensor by NXP and Texas Instruments.
 * 		   library source file
 */

#include "pico/stdlib.h"
#include "../../include/lm75/lm75.hpp"

/*!
 * @brief Constructor for LIB_LM75A class
 * @param i2cAddress I2C address of the device (7-bit)
 * @param i2c_type I2C instance of port IC20 or I2C1
 * @param SDApin I2C Data pin
 * @param SCLKpin I2C Clock pin
 * @param CLKspeed I2C Bus Clock speed in KHz. Typically 100-400
 */
LIB_LM75A::LIB_LM75A(uint8_t i2cAddress, i2c_inst_t *i2c_type, uint8_t SDApin, uint8_t SCLKpin, uint16_t CLKspeed)
{
	_i2cAddress = i2cAddress;
	_SClkPin = SCLKpin;
	_SDataPin = SDApin;
	_CLKSpeed = CLKspeed;
	i2c = i2c_type;
}

/*! @brief Initialize the LM75A sensor */
void LIB_LM75A::initLM75A()
{
	gpio_set_function(_SDataPin, GPIO_FUNC_I2C);
	gpio_set_function(_SClkPin, GPIO_FUNC_I2C);
	gpio_pull_up(_SDataPin);
	gpio_pull_up(_SClkPin);
	i2c_init(i2c, _CLKSpeed * 1000);
}

/*! @brief De-initialize the LM75A sensor */
void LIB_LM75A::deinitLM75A()
{
	gpio_set_function(_SDataPin, GPIO_FUNC_NULL);
	gpio_set_function(_SClkPin, GPIO_FUNC_NULL);
	i2c_deinit(i2c); 	
}

// *** Power management ***

/*! @brief enter Shutdown mode : 4 μA (Typical) current draw.*/
void LIB_LM75A::shutdown()
{
	uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11111110) | 0b1);
}

/*! @brief exit Shutdown mode for Operating mode: 280 μA (Typical) current draw.*/
void LIB_LM75A::wakeup()
{
	uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, config & 0b11111110);
}

/*!
	@brief Check if the device is in shutdown mode
	@return 1 if in shutdown mode, 0 operating mode
*/
bool LIB_LM75A::isShutdown()
{
	return (read8bitRegister(LM75A_REGISTER_CONFIG) & 0b1) == 1;
}

// *** Temperature functions ***

/*!
	@brief Get temperature in Celsius
	@return Temperature in Celsius as float
*/
float LIB_LM75A::getTemperature()
{
	uint16_t result;
	if (!read16bitRegister(LM75A_REGISTER_TEMP, result))
	{
		return LM75A_INVALID_TEMPERATURE;
	}
	return (float)result / 256.0f;
}

/*!
	@brief Get temperature in Fahrenheit
	@return Temperature in Fahrenheit as float
	@note 1.8 * C + 32 = F
*/
float LIB_LM75A::getTemperatureInFarenheit()
{
	return getTemperature() * 1.8f + 32.0f;
}

// *** Configuration functions ***


/*!
	@brief Get the hysteresis temperature
	@return Hysteresis temperature in Celsius as float
*/
float LIB_LM75A::getHysterisisTemperature()
{
	uint16_t result;
	if (!read16bitRegister(LM75A_REGISTER_THYST, result))
	{
		return LM75A_INVALID_TEMPERATURE;
	}
	return (float)result / 256.0f;
}

/*!
	@brief Get the fault queue value
	@return Fault queue value as FaultQueueValue enum
*/
FaultQueueValue LIB_LM75A::getFaultQueueValue()
{
	return (FaultQueueValue)(read8bitRegister(LM75A_REGISTER_CONFIG) & 0b00011000);
}

/*!
	@brief Get the OS trip temperature
	@return OS trip temperature in Celsius as float
*/
float LIB_LM75A::getOSTripTemperature()
{
	uint16_t result;
	if (!read16bitRegister(LM75A_REGISTER_TOS, result))
	{
		return LM75A_INVALID_TEMPERATURE;
	}
	return (float)result / 256.0f;
}

/*!
	@brief Get the OS polarity
	@return OS polarity as OsPolarity enum
*/
OsPolarity LIB_LM75A::getOsPolarity()
{
	return (OsPolarity)(read8bitRegister(LM75A_REGISTER_CONFIG) & 0b100);
}

/*!
	@brief Get the device mode
	@return Device mode as DeviceMode enum
*/
DeviceMode LIB_LM75A::getDeviceMode()
{
	return (DeviceMode)(read8bitRegister(LM75A_REGISTER_CONFIG) & 0b010);
}

/*!
	@brief Set the hysteresis temperature
	@param temperature Hysteresis temperature in Celsius as float
*/
void LIB_LM75A::setHysterisisTemperature(float temperature)
{
	write16bitRegister(LM75A_REGISTER_THYST, temperature * 256);
}

/*!
	@brief Get the OS trip temperature
	@return OS trip temperature in Celsius as float
*/
void LIB_LM75A::setOsTripTemperature(float temperature)
{
	write16bitRegister(LM75A_REGISTER_TOS, temperature * 256);
}

/*!
	@brief Set the fault queue value
	@param value Fault queue value as FaultQueueValue enum
*/
void LIB_LM75A::setFaultQueueValue(FaultQueueValue value)
{
	uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11100111) | value);
}

/*!
	@brief Set the OS polarity
	@param polarity OS polarity as OsPolarity enum
*/
void LIB_LM75A::setOsPolarity(OsPolarity polarity)
{
	uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11111011) | polarity);
}

/*!
	@brief Set the device mode
	@param deviceMode Device mode as DeviceMode enum
*/
void LIB_LM75A::setDeviceMode(DeviceMode deviceMode)
{
	uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11111101) | deviceMode);
}

/*! 
	@brief Checks if the device is connected
	@details This function checks if the device is connected 
	by reading the configuration register and checking if it returns 0x0F.
	@return true if connected, false otherwise
*/
bool LIB_LM75A::isConnected()
{
	uint8_t oldValue = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, 0x0f);
	uint8_t newValue = read8bitRegister(LM75A_REGISTER_CONFIG);
	write8bitRegister(LM75A_REGISTER_CONFIG, oldValue);
	return newValue == 0x0f;
}

/*! 
 *  @brief  reads config register
 *  @returns Byte with values of config register
 */
uint8_t LIB_LM75A::getConfig()
{
	return read8bitRegister(LM75A_REGISTER_CONFIG);
}

/*! 
 *  @brief  reads value of product ID register
 *  @return Product ID as float
 */
float LIB_LM75A::getProdId()
{
	uint8_t value = read8bitRegister(LM75A_REGISTER_PRODID);
	return (float)(value >> 4) + (value & 0x0F) / 10.0f;
} 

// *** Private functions  ***

uint8_t LIB_LM75A::read8bitRegister(const uint8_t reg)
{
	uint8_t result;
	uint8_t BufTx[1];
	BufTx[0] = reg;

	return_value = i2c_write_blocking(i2c, _i2cAddress, BufTx, 1 , true);
	if (return_value < 1)
	{
		return 0xFF;
	}

	return_value =  i2c_read_blocking(i2c, _i2cAddress, &result, 1, false); 	
	if (return_value < 1)
	{
		return 0xFF;
	}
	return result;
}

bool LIB_LM75A::read16bitRegister(const uint8_t reg, uint16_t& response)
{
	uint8_t bufRX[3];
	uint8_t bufTX[1];
	bufTX[0] = reg;

	return_value = i2c_write_blocking(i2c, _i2cAddress, bufTX, 1 , true);
	if (return_value < 1)
	{
		return false;
	}

	return_value =  i2c_read_blocking(i2c, _i2cAddress, bufRX, 2, false); 
	if (return_value < 1)
	{
		return false;
	}
	
	response = bufRX[0] << 8 | bufRX[1];
	return true;
}

bool LIB_LM75A::write16bitRegister(const uint8_t reg, const uint16_t value)
{
	uint8_t bufTX[3];
	bufTX[0] = reg;
	bufTX[1] = value >> 8;
	bufTX[2] = value;
		
	return_value = i2c_write_blocking(i2c, _i2cAddress, bufTX, 3 , false);
	if (return_value < 1)
	{
		return false;
	}

	return true;
}

bool LIB_LM75A::write8bitRegister(const uint8_t reg, const uint8_t value)
{
	uint8_t bufTX[2];
	bufTX[0] = reg;
	bufTX[1] = value;

	return_value = i2c_write_blocking(i2c, _i2cAddress, bufTX, 2 ,false);
	if (return_value < 1)
	{
		return false;
	}
	return true;
}