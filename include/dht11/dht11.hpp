/*
	@file dht11.hpp
	@brief header file for DHT11 temperature and humidity sensor library for rpi Pico
*/
#include <cinttypes>
#include <cstdio>
#include "pico/stdlib.h"

#pragma once

/*! 
 * @brief Class for DHT11 Sensor
 */
class DHT11_SENSOR {

public:

	DHT11_SENSOR (uint8_t SignalGPIO , uint32_t GPIOWaitTimeout);

	void SensorInit(void);
	void StartSignal(void);
	void CheckResponse(void);
	void ReadSensorData(void);
	bool CheckSumCheck(void);
	int  PrintSensorData(bool centigrade);
	void SensorDeInit(void);

	/*! @brief Structure representing the Data from dht11 sensor */
	struct Sensor_Status_t 
	{
		uint8_t CheckSensor= 0;  /**< Sensor status,  0 error 1 success */
		uint8_t Temperature = 0; /**< temperature data */
		uint8_t Humdity = 0;     /**< Humidity data */
		uint8_t CheckSum = 0;   /**< Check sum of sensor data, used for data intergity checking */
	};

	Sensor_Status_t  SensorStatus; /**< A instance of the structure holding Sensor data data */

protected:

private:
	uint8_t _DATA_IO = 0; /**< Sensor Data IO pin */
	
	uint8_t ReadDataByte(void);

	// Commications delays page 6 datasheet at link in URL
	// Start Signal
	const uint16_t StartSignalOne =   18;  /**< mS at least 18mS */
	const uint16_t StartSignalTwo =   30;  /**< uS 20-40uS */
	// check response 
	const uint16_t CheckSignalOne =   40;  /**< uS All check signals combined should equal 160uS */
	const uint16_t CheckSignalTwo =   80;  /**<                     """" */
	const uint16_t CheckSignalThree = 40;  /**<                     """" */
	// read data
	uint32_t _GPIO_WaitTimeout = 10000;    /**< uS Timeout for GPIO to wait for sensor response, min  > 50uS, user defined to prevent comms lockup.*/
	const uint16_t ReadSignal  = 35;       /**< uS 26-28 is low, 70uS high : so set this 30<->68 uS  */
};
