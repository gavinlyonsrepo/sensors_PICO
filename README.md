[![Website](https://img.shields.io/badge/Website-Link-blue.svg)](https://gavinlyonsrepo.github.io/)  [![Rss](https://img.shields.io/badge/Subscribe-RSS-yellow.svg)](https://gavinlyonsrepo.github.io//feed.xml)  [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/whitelight976)

# Sensor Library for Raspberry PI PICO

## Table of contents

  * [Overview](#overview)
  * [Documentation](#documentation)
    * [Supported devices](#supported-devices)

## Overview

* Name : Sensors_PICO
* Description :

C++ Library to support sensors for the Raspberry PI PICO. [URL project github link](https://github.com/gavinlyonsrepo/sensors_PICO)

* Author: Gavin Lyons

* Developed on Toolchain:
	1. Raspberry pi PICO RP2040
	2. SDK(1.4.0) C++20
	3. compiler G++ for arm-none-eabi((15:10.3-2021.07-4) 
	4. CMAKE(VERSION 3.20) , VScode(1.84.2)
	5. Linux Mint 22.1

## Documentation

### Supported devices

| Component name | Type | Interface | Readme URL link |
| -------- | ---------- | --------- | ---------- |
|LM35|Temperature   | ADC | [Readme](extra/doc/lm35/README.md)|
|LM75A|Temperature   | I2C | [Readme](extra/doc/lm75/README.md)|
|DHT11| Humidity | GPIO | [Readme](extra/doc/dht11/README.md)|
|AHT10| Humidity | I2C | [Readme](extra/doc/ahtxx/README.md)|
|BMP280 | Pressure | SPI or I2C | [Readme](extra/doc/bmp280/README.md)|
