# BMP280

![ Pinout](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/bmp280.jpg)

## Overview

* Name: BMP280
* Description: 

Library for Bosch BMP280 Digital pressure sensor hardware SPI.

* Developed on
	1. Raspberry pi PICO RP2040
	2. SDK C++ compiler,  arm-none-eabi-g++ (15:10.3)
	3. CMAKE , VScode


* Supports sensors features:

1. Read pressure
2. Read temperature
3. Tested on SPI interface(I2C not implemented yet)

* [Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf)

## Connections

The Sensor uses SPI for communication's.
Can be set up for any SPI interface and bus speed. 

| BMP280 Pin | Function | Pico GPIO | Notes        |
|------------|----------|-----------|--------------|
| CSB        | Chip Select (CS) | GPIO 17   | Active Low pick any GPIO  |
| SDA        | MOSI (Data In)   | GPIO 19   | Master Out Slave In |
| SCL        | SCK (Clock)      | GPIO 18   | SPI Clock |
| SDO        | MISO (Data Out)  | GPIO 16   | Master In Slave Out | 

## Output

Data is outputted (eg to a PC) via a USB.

 ![ op](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/bmpoutput.png)
