/*
 * Project Name: Library for the LM75A temperature sensor by NXP and Texas Instruments.
 * File: lm75.hpp
 * Description: library header file
 * Author: Gavin Lyons.
 * IDE:  Rpi=PICo rp2040 C++
 * Created Sep 2022
 * Description: See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/RPI_PICO_projects_list
 */

#ifndef LIB_LM75_h
#define LIB_LM75_h


#include "hardware/i2c.h"

#define LM75A_DEFAULT_ADDRESS		0x49		// Address is configured with pins A0-A2, 8 bit address
#define LM75A_TO_I2C_DELAY          50          // Timeout for I2C comms, mS,
#define LM75A_REGISTER_TEMP			0			// Temperature register (read-only)
#define LM75A_REGISTER_CONFIG		1			// Configuration register
#define LM75A_REGISTER_THYST		2			// Hysteresis register
#define LM75A_REGISTER_TOS			3			// OS register
#define LM75A_REGISTER_PRODID		7			// Product ID register - Only valid for Texas Instruments

#define LM75_CONF_OS_COMP_INT		1			// OS operation mode selection
#define LM75_CONF_OS_POL			2			// OS polarity selection
#define LM75_CONF_OS_F_QUE			3			// OS fault queue programming

#define LM75A_INVALID_TEMPERATURE	-1000.0f	// Just an arbritary value outside of the sensor limits

enum FaultQueueValue : uint8_t
{
	NUMBER_OF_FAULTS_1 = 0,
	NUMBER_OF_FAULTS_2 = 0b01000,
	NUMBER_OF_FAULTS_4 = 0b10000,
	NUMBER_OF_FAULTS_6 = 0b11000
};

enum OsPolarity : uint8_t
{
	OS_POLARITY_ACTIVELOW = 0,
	OS_POLARITY_ACTIVEHIGH = 0b100
};

enum DeviceMode : uint8_t
{
	DEVICE_MODE_COMPARATOR = 0,
	DEVICE_MODE_INTERRUPT = 0b10
};

class LIB_LM75A
{
private:
	// Private variables
	uint8_t _i2cAddress; 
	i2c_inst_t *i2c = i2c0;  // i2C port number, i2c1 or i2c0
    uint8_t _SDataPin;
    uint8_t _SClkPin;
    uint16_t _CLKSpeed = 100; //I2C bus speed in khz typically 100-400

	// Private functions
	uint8_t read8bitRegister(const uint8_t reg);
	bool read16bitRegister(uint8_t reg, uint16_t& response);
	bool write16bitRegister(const uint8_t reg, const uint16_t value);
	bool write8bitRegister(const uint8_t reg, const uint8_t value);

public:

	// Constructor
    LIB_LM75A(uint8_t address, i2c_inst_t* i2c_type, uint8_t SDApin, uint8_t  SCLKpin, uint16_t CLKspeed);

	//I2c init & deinit
	void initLM75A();
	void deinitLM75A();

	// Power management
	void shutdown();
	void wakeup();
	bool isShutdown();

	// Temperature functions
    float getTemperature();
	float getTemperatureInFarenheit();

	// Configuration functions
	float getHysterisisTemperature();	
	FaultQueueValue getFaultQueueValue();
	float getOSTripTemperature();
	OsPolarity getOsPolarity();
	DeviceMode getDeviceMode();
	void setHysterisisTemperature(const float temperature);
	void setOsTripTemperature(const float temperature);
	void setFaultQueueValue(const FaultQueueValue value);
	void setOsPolarity(const OsPolarity polarity);
	void setDeviceMode(const DeviceMode deviceMode);	

	// Other
	bool isConnected();
	uint8_t getConfig();
	float getProdId();

	int16_t return_value = 0; //return value, I2C routines
};

#endif
