/*!
	@file lm35.cpp 
	@brief Source file for LM35 temperature sensor 
*/

#include "../../include/lm35/lm35.hpp"

/*! Constructor */
LM35_SENSOR::LM35_SENSOR(){}


/*!
	@brief Init the LM35 Sensor
	@param analogPin The ADC pin to use on RP2040 (GPIO26–29) or RP2350A(GPIO26–29) or RP2350B (GPIO40–48)
	@return 0 for success, -2 if invalid ADC chosen 
*/
int LM35_SENSOR::Init(uint8_t analogPin)
{
	_analogPin = analogPin;
	adc_init();

	if ((analogPin >= 26 && analogPin <= 29) || (analogPin >= 40 && analogPin <= 48)) {
		adc_gpio_init(analogPin);
		// Calculate ADC input number based on GPIO pin
		uint adcInput = (analogPin >= 40) ? (analogPin - 36) : (analogPin - 26);
		adc_select_input(adcInput);
	} else {
		printf("Error: Init: Invalid ADC pin. RP2040/RP2350A supports GPIO26–29. RP2350B supports GPIO40–48.\n");
		return -2;
	}

	return 0;
}

/*!
	@brief De-initialize the LM35 Sensor
*/
void LM35_SENSOR::deInit()
{
	gpio_set_function(_analogPin, GPIO_FUNC_NULL);
}

/*!
	@brief Get the temperature in Celsius
	@return Temperature in Celsius
*/
double LM35_SENSOR::getTemp()
{
	double valueADC = adc_read();
	double celsius = (double(valueADC) * (getConversionFactor() / getVolts())); 
	return celsius;
}

/*!
	@brief Get the temperature in the specified format
	@param TemperatureType The type of temperature to return (Celsius, Fahrenheit, Kelvin)
	@return Temperature in the specified format
*/
double LM35_SENSOR::getTemp(TemperatureType_e TemperatureType)
{
	switch (TemperatureType) 
	{
		case FAHRENHEIT:
			return getTemp() * 1.8 + 32.0; // °F = (°C × 1.8) + 32
		case KELVIN:
			return getTemp() + 273.15;     // K = °C + 273.15
		case CELSIUS:
			return getTemp();
		default:
			return getTemp();
	}
}

/*!
	@brief Get the conversion factor for the ADC
	@return Conversion factor for the ADC
*/
double LM35_SENSOR::getConversionFactor(void) { return conversionFactor; }

/*!
	@brief Get the voltage per degree Celsius
	@return Voltage per degree Celsius
*/
double LM35_SENSOR::getVolts(void){return VoltsPerDegreeC;}

