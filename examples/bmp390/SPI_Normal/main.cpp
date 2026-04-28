/*!
	@file examples/bmp390/SPI_Normal/main.cpp
	@brief RPI PICO SDK C++ bmp390 library test file, basic use, normal mode, SPI hardware.
	@details bmp390 is a digital pressure sensor with temperature measurement capabilities.
		In normal mode the sensor continuously cycles between measurement and standby.
		BMP390_DEBUG is set to 1 here for initial bring-up / register verification.
		Set it back to 0 once the sensor is confirmed working.
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
	printf("\n--- START BMP390 Normal SPI ---\n");

	bmp390.InitSensor();

	uint8_t chipID = bmp390.readForChipID();
	printf("Chip ID: %#x\n", chipID); // Expect 0x50 (BMP390) or 0x60 (BMP390L)

	// Report active configuration set by StartUpRoutine
	printf("Power mode   : %u\n",   static_cast<uint8_t>(bmp390.readPowerMode()));
	printf("IIR filter   : %u\n",   static_cast<uint8_t>(bmp390.readIIRFilter()));
	printf("ODR setting  : %u\n",   static_cast<uint8_t>(bmp390.readODR()));
	printf("Temp OS      : %u\n",
		static_cast<uint8_t>(bmp390.readOversampling(BMP390_Sensor::DataType_e::Temperature)));
	printf("Pressure OS  : %u\n\n",
		static_cast<uint8_t>(bmp390.readOversampling(BMP390_Sensor::DataType_e::Pressure)));

	uint16_t counter = 0;
	while (counter < 60)
	{
		printf("Test Count: %u\n", counter);

		// Test 1 Temperature
		printf("Temperature oversampling : %u\n",
			static_cast<uint8_t>(bmp390.readOversampling(BMP390_Sensor::DataType_e::Temperature)));
		printf("Temperature              : %.2f [C]\n", bmp390.readTemperature());

		// Test 2 Pressure
		printf("Pressure oversampling    : %u\n",
			static_cast<uint8_t>(bmp390.readOversampling(BMP390_Sensor::DataType_e::Pressure)));
		printf("Pressure                 : %.2f [hPa]\n",
			bmp390.readPressure(BMP390_Sensor::PressureUnit_e::hPa));

		// Test 3 Altitude
		printf("Altitude                 : %.2f [m]\n", bmp390.readAltitude(LOCAL_PRES));

		printf("\n");
		sleep_ms(3000);
		counter++;
	}

	bmp390.DeInitSensor();
	printf("--- END ---\n");
}
