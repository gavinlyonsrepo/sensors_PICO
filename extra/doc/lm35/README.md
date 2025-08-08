[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/whitelight976)

## Overview

* Name: LM35
* Description: 
Library for LM35 temperature sensor.
* Author: Gavin Lyons
* Developed on
	1. Raspberry pi PICO RP2040
	2. SDK C++ compiler G++ for arm-none-eabi
	3. CMAKE , VScode

## Connections

The Sensor uses ADC for communication's.
Connect Vcc to 5 volts not 3.3 volts. Connect sensor analog out to an ADC 
(ADC 0:2) (GPIO 26:29) for rp2040. Example file connects 2 sensors to GPIO 26 & 27. 
note on PICO GPIO 29 is not broken out on PCB.

 ![ Pinout](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/lm35.jpg)

## Files

The example file main.cpp contains tests showing library functions.
There is also the library files(lm35.cpp and lm35.hpp),

## Output

The example file outputs data to the PC USB, 38400 baud. 

![ Output](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/outputlm35.png)

## Datasheet

LM35 data sheet.
 - [Texas Instruments data sheet](https://www.ti.com/product/LM35)
