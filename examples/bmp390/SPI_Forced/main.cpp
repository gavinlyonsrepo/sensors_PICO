/*!
	@file examples/bmp390/SPI_Forced/main.cpp
	@brief RPI PICO SDK C++ bmp390 library test file, basic use, forced mode, SPI hardware.
	@details bmp390 is a digital pressure sensor with temperature measurement capabilities.
		In forced mode the sensor takes a single measurement on demand and
		returns to sleep automatically. takeForcedMeasurement() blocks until
		the STATUS register confirms both results are ready.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp390/bmp390.hpp"

#define LOCAL_PRES   1024.25 // Supply local sea-level pressure in hPa
#define SPI_PORT     spi0    // SPI port: spi0 or spi1
#define SPI_BAUDRATE 500000  // 500 kHz
#define CS           17      // CSB  GPIO pin
#define MOSI         19      // SDA  GPIO pin
#define SCK          18      // SCL  GPIO pin
#define MISO         16      // SDO  GPIO pin

BMP390_Sensor bmp390(SPI_PORT, SPI_BAUDRATE, CS, MOSI, SCK, MISO);

int main()
{
	stdio_init_all();
	sleep_ms(1000);
	printf("\n--- START BMP390 Forced SPI ---\n");

	bmp390.InitSensor();

	uint8_t chipID = bmp390.readForChipID();
	printf("Chip ID: %#x\n", chipID); // Expect 0x50 (BMP390) or 0x60 (BMP390L)

	// Switch to forced mode: InitSensor() leaves the sensor in normal mode
	if (bmp390.setPowerMode(BMP390_Sensor::PowerMode_e::Forced))
	{
		printf("Power mode set to Forced.\n");
	}
	else
	{
		printf("Failed to set power mode to Forced.\n");
	}
	printf("Power mode   : %u\n",   static_cast<uint8_t>(bmp390.readPowerMode()));

	// Optional: increase oversampling for a one-shot measurement
	bmp390.setOversampling(BMP390_Sensor::DataType_e::Temperature, BMP390_Sensor::sensorSampling_e::Sampling_X16);
	bmp390.setOversampling(BMP390_Sensor::DataType_e::Pressure,    BMP390_Sensor::sensorSampling_e::Sampling_X8);
	bmp390.setIIRFilter(BMP390_Sensor::Filter_e::Filter_X3);

	printf("Temp OS      : %u\n",
		static_cast<uint8_t>(bmp390.readOversampling(BMP390_Sensor::DataType_e::Temperature)));
	printf("Pressure OS  : %u\n",
		static_cast<uint8_t>(bmp390.readOversampling(BMP390_Sensor::DataType_e::Pressure)));
	printf("IIR filter   : %u\n\n", static_cast<uint8_t>(bmp390.readIIRFilter()));

	uint16_t counter = 0;
	while (counter < 60)
	{
		printf("Test Count: %u\n", counter);

		// Trigger one measurement and wait for results to be ready
		if (bmp390.takeForcedMeasurement())
		{
			// Test 1 Temperature
			printf("Temperature : %.2f [C]\n",  bmp390.getTemperature());
			// Test 2 Pressure
			printf("Pressure    : %.2f [hPa]\n",
				bmp390.getPressure(BMP390_Sensor::PressureUnit_e::hPa));
			// Test 3 Altitude (supply local sea-level pressure in hPa)
			printf("Altitude    : %.2f [m]\n",   bmp390.readAltitude(LOCAL_PRES));
		}
		else
		{
			printf("takeForcedMeasurement failed - check power mode.\n");
		}
		printf("\n");
		sleep_ms(3000);
		counter++;
	}

	bmp390.DeInitSensor();
	printf("--- END ---\n");
}
