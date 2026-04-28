/*!
	@file examples/bmp390/I2C_Forced/main.cpp
	@brief RPI PICO SDK C++ bmp390 library test file, basic use, forced mode, I2C hardware.
	@details bmp390 is a digital pressure sensor with temperature measurement capabilities.
		In forced mode the sensor takes a single measurement on demand and
		returns to sleep automatically. takeForcedMeasurement() blocks until
		the STATUS register confirms both results are ready.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp390/bmp390.hpp"

#define LOCAL_PRES      1024.25    // Supply local sea-level pressure in hPa
#define I2C_PORT        i2c0       // I2C port: i2c0 or i2c1
#define I2C_BAUDRATE    400000     // 400 kHz fast mode
#define I2C_TIMEOUT_US  50000      // 50 ms timeout
#define I2C_ADDRESS     0x76       // 0x76 = SDO LOW, 0x77 = SDO HIGH
#define SDA             16         // SDA GPIO pin
#define SCL             17         // SCL GPIO pin

BMP390_Sensor bmp390(I2C_PORT, I2C_BAUDRATE, I2C_TIMEOUT_US, I2C_ADDRESS, SDA, SCL);

int main()
{
	stdio_init_all();
	sleep_ms(1000);
	printf("\n--- START BMP390 Forced I2C ---\n");
	// Optional: check device is on the bus before full initialisation
	BMP390_Sensor probe(I2C_PORT, I2C_BAUDRATE, I2C_TIMEOUT_US, I2C_ADDRESS, SDA, SCL);
	bmp390.InitSensor();

	uint8_t chipID = bmp390.readForChipID();
	printf("Chip ID: %#x\n", chipID); // except 0x60 
	int16_t connCheck = bmp390.CheckConnectionI2C();
	if (connCheck < 1)
	{
		printf("BMP390 not found on I2C bus (address 0x%02X). Check wiring.\n", I2C_ADDRESS);
		return -1;
	}
	printf("BMP390 found on I2C bus.\n");
	// Switch to forced mode: InitSensor() leaves the sensor in normal mode
	if (bmp390.setPowerMode(BMP390_Sensor::PowerMode_e::Forced))
	{
		printf("Power mode set to Forced.\n");
	}else{
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
