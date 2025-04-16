/*! 
	@file lm35.hpp
	@brief LM35 Sensor library header file
*/

#pragma once

#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdint.h>
#include <cstdio> // for printf


/*! Supported Temperature types */ 
enum TemperatureType_e : uint8_t
{
	CELSIUS = 0,    /**< Celsius*/ 
	FAHRENHEIT = 1, /**< Fahrenheit*/
	KELVIN = 2      /**< Kelvin*/
};

/*!
	@brief LM35 Sensor class
	@details This class provides an interface to the LM35 temperature sensor.
*/
class LM35_SENSOR 
{
	public:
		LM35_SENSOR();

		int Init(uint8_t analogPin);
		void deInit(void);
		double getTemp(void);
		double getTemp(TemperatureType_e  temperatureType);
		double getConversionFactor(void);
		double getVolts(void);

	private:
		uint8_t _analogPin; /**< Analog pin used for sensor */
		const double conversionFactor = 3.3f / (1 << 12); /**< Volt /resolution of ADC 3.3/4096 */
		const double VoltsPerDegreeC = 0.01f; /**</ 10mV per degree Celsius */
};
