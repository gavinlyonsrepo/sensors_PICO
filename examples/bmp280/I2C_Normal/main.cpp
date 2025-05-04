/*!
	@file examples/bmp280/main.cpp
	@brief RPI PICO SDK C++ bmp280 library test file, basic use, normal mode , I2C hardware
	@details bmp280 is a digital pressure sensor with temperature measurement capabilities.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp280/bmp280.hpp"

#define I2C_PORT i2c1 // I2C port i2c0 or i2c1
#define I2C_BAUDRATE 100000 // Hertz
#define I2C_TIMEOUT 50000 // I2C timeout delay in micro seconds, uS.
#define I2C_ADDRESS 0x76 // I2C address of the sensor, try 0x76 or 0x77
#define SDA 18 // SDA GPIO pin
#define SCK 19  // SCL GPIO pin

BMP280_Sensor bmp280(I2C_PORT, I2C_BAUDRATE, I2C_TIMEOUT, I2C_ADDRESS, SDA, SCK);

int main() {
	stdio_init_all();
	sleep_ms(1000);
	printf("\n--- START Normal I2C ---\n");
	bmp280.InitSensor();

	while (bmp280.CheckConnectionI2C() < 0) {
		printf("Failed to connect to BMP280 sensor\n");
		busy_wait_ms(3000);
	}
	uint8_t chipID = 0;
	chipID = bmp280.readForChipID();
	printf("Chip ID: %#x\n", chipID); // Should read 0x56 - 0x58 for BMP280 060 for BME280
	uint16_t counter = 0;
	while(counter < 60)
	{
		printf("Test Count: %u\n", counter);

		// Test 1 Temperature 
		printf("Temperature oversampling: %i\n", bmp280.readOversampling(BMP280_Sensor::DataType_e::Temperature));
		printf("Temperature: %f[C]\n", bmp280.readTemperature());
		// Test 2 Pressure
		printf("Pressure oversampling: %i\n", bmp280.readOversampling(BMP280_Sensor::DataType_e::Pressure));
		printf("Pressure: %f[hPa]\n", bmp280.readPressure(BMP280_Sensor::PressureUnit_e::hPa));

		printf("\n\n");
		sleep_ms(3000);
		counter++;
	}
	bmp280.DeInitSensor();
	printf("--- END ---\n");
}
