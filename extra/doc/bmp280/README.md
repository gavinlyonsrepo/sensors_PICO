# BMP280

![ Pinout](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/bmp280.jpg)

## Overview

* Name: BMP280
* Description:

Library for Bosch BMP280 Digital pressure sensor hardware SPI or I2C.

* Developed on
	1. Raspberry pi PICO RP2040
	2. SDK C++ compiler,  arm-none-eabi-g++ (15:10.3)
	3. CMAKE , VScode


* Supports sensors features:

1. Read pressure data
2. Read temperature data
3. Tested on SPI interface and I2C interface, Interface is selected by user constructor overload(see examples)
4. Oversampling settings , standby times, Filter settings and can be set thru API.
5. Normal mode, sleep mode and forced mode supported.
6. This supports the BMP280 sensor fully only. The humidity functionality of the BME280 is not tested or supported.
7. 3 examples:main.cpp files 2 for SPI and 1 for I2C, pick the one you want in CMakeLists.txt, add_executable section.
8. Chip ID should be 0x56-0X58 for BMP280, 0x60 BME280. 

* [Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf)

## Default settings


| BMP280 Setting | Default Enumeration |
|------------|----------|
| Oversampling Temperature | Sampling_X16 (5)|
| Oversampling Pressure    | Sampling X2 (2)  |
| Standby Duration         | StandBy_MS_1 (0) |
| Filter                   | Filter_Off (0)|
| Power mode               | Normal (2) |

## Connections

The Sensor uses SPI or I2C for communication's.

### SPI Connections

The BMP280 can be connected to the Raspberry Pi Pico using SPI. The following table shows the pin connections between the BMP280 and the Raspberry Pi Pico spi0.
The BMP280 is a 3.3V device and the Pico is also a 3.3V device. The BMP280 has a CSB pin which can be connected to any GPIO pin on the Pico. The CSB pin is active low, so it should be pulled high when not in use. Can be set up for any SPI interface and bus speed.

| BMP280 Pin | Function | Pico GPIO | Notes        |
|------------|----------|-----------|--------------|
| CSB        | Chip Select (CS) | GPIO 17   | Active Low pick any GPIO  |
| SDA        | MOSI (Data In)   | GPIO 19   | Master Out Slave In |
| SCL        | SCK (Clock)      | GPIO 18   | SPI Clock |
| SDO        | MISO (Data Out)  | GPIO 16   | Master In Slave Out |

### I2C Connections

The BMP280 can be connected to the Raspberry Pi Pico using I2C. The following table shows the pin connections between the BMP280 and the Raspberry Pi Pico I2c1.
The BMP280 is a 3.3V device and the Pico is also a 3.3V device.
I2C error timeout and baudrate can be adjusted. The BMP280 has a CSB pin which can be connected to any GPIO pin on the Pico. The CSB pin is active low, so it should be pulled high when not in use. The I2C address of the BMP280 is 0x76 or 0x77 depending on the SDO pin connection.

| BMP280 Pin | Function | Pico GPIO | Notes        |
|------------|----------|-----------|--------------|
| SDA        | Data     | GPIO 18   | I2C DATA |
| SCL        | Clock   | GPIO 19   | I2C Clock |
| CSB        | Chip Select  | n/a  | set high |
| SDO        | MISO   | n/a  | used to select I2C address, If SDO is high then the I2C address is 0x77. If SDO is low the I2C address is 0x76. |

## Output

Data is outputted (eg to a PC) via a USB.

 ![ op](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/bmpoutput.png)
