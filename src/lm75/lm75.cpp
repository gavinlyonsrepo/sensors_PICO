/*
 * Project Name: Library for the LM75A temperature sensor by NXP and Texas Instruments.
 * File: LM75A.cpp
 * Description: library Source file
 * Author: Gavin Lyons.
 * IDE:  Rpi=PICo rp2040 C++
 * Created Sep 2022
 * Description: See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/RPI_PICO_projects_list
 */

#include "pico/stdlib.h"
#include "../../include/lm75/lm75.hpp"


// Constructor
// Param1 : I2C address (7-bit)
// Param2 : I2C instance of port IC20 or I2C1
// Param3 : I2C Data pin
// Param4 : I2C Clock pin
// Param5 : I2C Bus Clock speed in KHz. Typically 100-400
LIB_LM75A::LIB_LM75A(uint8_t i2cAddress, i2c_inst_t* i2c_type, uint8_t SDApin, uint8_t SCLKpin, uint16_t CLKspeed)
{
    _i2cAddress = i2cAddress;
    _SClkPin = SCLKpin;
    _SDataPin = SDApin;
    _CLKSpeed = CLKspeed;
     i2c = i2c_type; 
}

// Power management

// Func Desc : enter Shutdown mode : 4 μA (Typical) current draw.
void LIB_LM75A::shutdown()
{
    uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11111110) | 0b1);
}

// Func Desc : exit Shutdown mode for  Operating  mode: 280 μA (Typical)
void LIB_LM75A::wakeup()
{
    uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, config & 0b11111110);
}
// Fucn Desc get power mode
// Return bool
// 1 = shutdown
// 0 operating mode
bool LIB_LM75A::isShutdown()
{
    return (read8bitRegister(LM75A_REGISTER_CONFIG) & 0b1) == 1;	
}


// Temperature functions

// Get temperature Celsius
// returns float
float LIB_LM75A::getTemperature()
{
    uint16_t result;
    if (!read16bitRegister(LM75A_REGISTER_TEMP, result))
    {
        return LM75A_INVALID_TEMPERATURE;
    }
    return (float)result / 256.0f;
}

// Get temperature Farenheit(
// returns float
float LIB_LM75A::getTemperatureInFarenheit()
{
    return getTemperature() * 1.8f + 32.0f;
}

// Configuration functions

float LIB_LM75A::getHysterisisTemperature()
{
    uint16_t result;
    if (!read16bitRegister(LM75A_REGISTER_THYST, result))
    {
        return LM75A_INVALID_TEMPERATURE;
    }
    return (float)result / 256.0f;
}

FaultQueueValue LIB_LM75A::getFaultQueueValue()
{
    return (FaultQueueValue)(read8bitRegister(LM75A_REGISTER_CONFIG) & 0b00011000);
}

float LIB_LM75A::getOSTripTemperature()
{
    uint16_t result;
    if (!read16bitRegister(LM75A_REGISTER_TOS, result))
    {
        return LM75A_INVALID_TEMPERATURE;
    }
    return (float)result / 256.0f;
}

OsPolarity LIB_LM75A::getOsPolarity()
{
    return (OsPolarity)(read8bitRegister(LM75A_REGISTER_CONFIG) & 0b100);
}

DeviceMode LIB_LM75A::getDeviceMode()
{
    return (DeviceMode)(read8bitRegister(LM75A_REGISTER_CONFIG) & 0b010);
}

void LIB_LM75A::setHysterisisTemperature(float temperature)
{
    write16bitRegister(LM75A_REGISTER_THYST, temperature * 256);
}

void LIB_LM75A::setOsTripTemperature(float temperature)
{
    write16bitRegister(LM75A_REGISTER_TOS, temperature * 256);
}

void LIB_LM75A::setFaultQueueValue(FaultQueueValue value)
{
    uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11100111) | value);
}

void LIB_LM75A::setOsPolarity(OsPolarity polarity)
{
    uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11111011) | polarity);
}

void LIB_LM75A::setDeviceMode(DeviceMode deviceMode)
{
    uint8_t config = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, (config & 0b11111101) | deviceMode);
}


// Function Desc Is the Device connected
// Writes Reads and writes  the Config register
// Returns 1 for success 0 if not.
bool LIB_LM75A::isConnected()
{
    uint8_t oldValue = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, 0x0f);
    uint8_t newValue = read8bitRegister(LM75A_REGISTER_CONFIG);
    write8bitRegister(LM75A_REGISTER_CONFIG, oldValue);
    return newValue == 0x0f;
}

// Func Desc :  reads config register
// Returns Byte with values of config register
uint8_t LIB_LM75A::getConfig()
{
    return read8bitRegister(LM75A_REGISTER_CONFIG);
}

// Func Desc :  reads value of product ID register
float LIB_LM75A::getProdId()
{
    uint8_t value = read8bitRegister(LM75A_REGISTER_PRODID);
    return (float)(value >> 4) + (value & 0x0F) / 10.0f;
} 

// *************** Private function  ************************

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

void LIB_LM75A::initLM75A()
{
    gpio_set_function(_SDataPin, GPIO_FUNC_I2C);
    gpio_set_function(_SClkPin, GPIO_FUNC_I2C);
    gpio_pull_up(_SDataPin);
    gpio_pull_up(_SClkPin);
    i2c_init(i2c, _CLKSpeed * 1000);
}

void LIB_LM75A::deinitLM75A()
{
    gpio_set_function(_SDataPin, GPIO_FUNC_NULL);
    gpio_set_function(_SClkPin, GPIO_FUNC_NULL);
    i2c_deinit(i2c); 	
}