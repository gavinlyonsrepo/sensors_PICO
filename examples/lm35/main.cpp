/*! 
	@file main.cpp
	@brief LM35 Sensor library example test file
	@details This example test file demonstrates the use of the LM35 temperature sensor library.
			This example is for the Raspberry Pi Pico and RP2040 microcontroller.
			It initializes two LM35 sensors on ADC pins 26 and 27, reads the temperature values
			in Celsius, Fahrenheit, and Kelvin, and prints the results to the serial console.
			The test runs 10 times, with a 2-second delay between each reading.
			The LM35 sensors are de-initialized at the end of the test.
*/

#include "pico/stdlib.h"
#include "lm35/lm35.hpp"
#include <cstdio> // for printf

uint8_t ADC_PIN_1 = 26;   // ADC pin for LM35 sensor no 1
uint8_t ADC_PIN_2 = 27;   // ADC pin for LM35 sensor no 2
uint8_t testCount = 0;  // Test count for the number of readings

LM35_SENSOR sensor1;
LM35_SENSOR sensor2; 

// *** Main ***
int main()
{
	stdio_init_all(); // Initialize chosen serial port 38400 default baudrate.
	busy_wait_ms(1000);
	printf("LM35 : Start!\r\n");

	if(sensor1.Init(ADC_PIN_1) != 0) //Initialize the LM35 sensor 
	{
		printf("LM35 : Error initializing sensor 1!\r\n");
		return -1;
	}else{
		printf("LM35 : Sensor 1 initialized successfully!\r\n");
	}
	if(sensor2.Init(ADC_PIN_2) != 0) //Initialize the LM35 sensor 2
	{
		printf("LM35 : Error initializing sensor 2!\r\n");
		return -1;
	}else{
		printf("LM35 : Sensor 2 initialized successfully!\r\n");
	}

	busy_wait_ms(1000); // Wait for 2 seconds before taking the next reading

	while(testCount < 10) // Run the test 10 times
	{
		testCount++;
		printf("Test Count : %u \n", testCount);
		// Read temperature values
		printf("LM35 : Running Test!\r\n");
		// Get temperature in Celsius
		double temperatureC = sensor1.getTemp();
		printf("Temperature 1 : %.2f *C \n", temperatureC);
		temperatureC = sensor2.getTemp();
		printf("Temperature 2 : %.2f *C \n", temperatureC);
		busy_wait_ms(2000);
		// Get temperature in Fahrenheit
		double temperatureF = sensor1.getTemp(FAHRENHEIT);
		printf("Temperature 1 : %.2f *F \n", temperatureF);
		temperatureF = sensor2.getTemp(FAHRENHEIT);
		printf("Temperature 2 : %.2f *F \n", temperatureF);
		busy_wait_ms(2000);
		// Get temperature in Kelvin
		double temperatureK = sensor1.getTemp(KELVIN);
		printf("Temperature 1 : %.2f K \n", temperatureK);
		temperatureK = sensor2.getTemp(KELVIN);
		printf("Temperature 2 : %.2f K \n", temperatureK);
		busy_wait_ms(2000);
	}

	printf("LM35 : End!\r\n");
	sensor1.deInit(); // De-initialize the LM35 sensor 1
	sensor2.deInit(); // De-initialize the LM35 sensor 2

}


