/*
 * Project Name: library for Aosong ASAIR AHT10, 
   AHT15 AHT20 Digital Humidity & Temperature Sensor
 * File: main.cpp
 * Description: main test file for AHT-XX library 
 * IDE:  Rpi PICO rp2040 C++
 * Created November 2022
 * Description: See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/AHTXX_PICO
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "ahtxx/ahtxx.hpp"

LIB_AHTXX myAHT10(AHT10_ADDRESS_0X38, i2c0, 16, 17, 100);

int main(void) {

	float AHT10_temperature;
	stdio_init_all();  // Initialize chosen serial port, 38400 default baudrate.
	busy_wait_ms(1000);

	printf("AHT10 Start \r\n");
	myAHT10.AHT10_InitI2C(AHT10_SENSOR);
	busy_wait_ms(500);

	// Start the sensor comms
	while (myAHT10.AHT10_begin() != true) 
	{
		printf("AHT10 not connect r fail load calib coeff \r\n");
		busy_wait_ms(5000);
	}

	while (1) 
	{

		// Display temperature
		AHT10_temperature = myAHT10.AHT10_readTemperature(true);

		if (AHT10_temperature != AHT10_ERROR) {
			AHT10_temperature *= 100;
			printf("Temperature : %u.%u C \r\n", ((unsigned int) AHT10_temperature / 100), ((unsigned int) AHT10_temperature % 100));
		} else {
			printf("Temp : AHT10 TX ERROR %f \r\n", AHT10_temperature);
		}
		
		// Display humidity
		AHT10_temperature = myAHT10.AHT10_readHumidity(true);

		if (AHT10_temperature != AHT10_ERROR) {
			AHT10_temperature *= 100;
			printf("Humidity : %u.%u %% \r\n", ((unsigned int) AHT10_temperature / 100),((unsigned int) AHT10_temperature % 100));
		} else {
			printf("Humidity : AHT10 TX ERROR %f \r\n", AHT10_temperature);
		}
		
		busy_wait_ms(8000); // Recommended polling frequency: 8sec - 30sec
	}

}


/***END OF FILE****/
