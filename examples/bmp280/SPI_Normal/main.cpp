/*!
	@file examples/bmp280/main.cpp
	@brief RPI PICO SDK C++ bmp280 library test file, basic use, normal mode , SPI hardware
	@details bmp280 is a digital pressure sensor with temperature measurement capabilities.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp280/bmp280.hpp"

#define SPI_PORT spi0 // SPI port spi0 or spi1
#define SPI_BAUDRATE 500000 // 500kHz 
#define CS 17   // CSB GPIO pin
#define MOSI 19 // SDA GPIO pin
#define SCK 18  // SCL GPIO pin
#define MISO 16 // SDO GPIO pin

BMP280_Sensor bmp280(SPI_PORT, SPI_BAUDRATE, CS, MOSI, SCK, MISO);

int main() {
	stdio_init_all();
	sleep_ms(1000);
	printf("\n--- START Normal SPI ---\n");
	bmp280.InitSensor();

	uint8_t chipID = 0;
	chipID = bmp280.readForChipID();
	printf("Chip ID: %#x\n", chipID); // Should read 0x56 - 0x58 for BMP280 060 for BME280
	uint16_t counter = 0;
	while(counter < 60)
	{
		printf("Test Count: %u\n", counter);

		// Test 1 Temperature 
		printf("Temperature oversampling: %u\n",static_cast<uint8_t>(bmp280.readOversampling(BMP280_Sensor::DataType_e::Temperature)));
		printf("Temperature: %f[C]\n", bmp280.readTemperature());
		// Test 2 Pressure
		printf("Pressure oversampling: %u\n", static_cast<uint8_t>(bmp280.readOversampling(BMP280_Sensor::DataType_e::Pressure)));
		printf("Pressure: %f[hPa]\n", bmp280.readPressure(BMP280_Sensor::PressureUnit_e::hPa));

		printf("\n\n");
		sleep_ms(3000);
		counter++;
	}
	bmp280.DeInitSensor();
	printf("--- END ---\n");
}
