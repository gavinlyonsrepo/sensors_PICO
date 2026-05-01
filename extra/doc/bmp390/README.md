# BMP390

![ Pinout](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/bmp390.png)

## Overview

* Name: BMP390
* Description:

Library for Bosch BMP390 Digital pressure sensor hardware SPI or I2C.

* Developed on

1. Raspberry pi PICO RP2040
2. SDK C++ compiler,  arm-none-eabi-g++ (15:10.3)
3. CMAKE , VScode

* Supports sensors features:

1. Read pressure data
2. Read temperature data
3. Tested on SPI interface and I2C interface, Interface is selected by user constructor overload(see examples)
4. Oversampling settings, filter settings and can be set thru API.
5. Normal mode, sleep mode and forced mode supported.
6. Chip ID read: 0x60
7. I2C address is 0x76 or 0x77 depending on SDO pin connection.
8. Supports FIFO and interrupt pin.

* [Data sheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp390-ds002.pdf)

## Default settings

These are the default settings for the BMP390 sensor when it is initialized. These settings can be changed using the API functions provided in the library.

| BMP390 Setting | Default Enumeration |
| ------------ | ---------- |
| Power mode | Normal (3) |
| IIR Filter | filter coefficient (0) bypass-mode |
| Output data rate (ODR) | ODR_100ODR 100Hz 10ms (2) |
| Oversampling Temperature | no oversampling (1) |
| Oversampling Pressure | no oversampling (1) |

## Examples

Select example you want by modifying the CMakeList.txt file(add_executable(${PROJECT_NAME} section).

| File name | Function |
| ------------ | ---------- |
| SPI_Normal/main.cpp | SPI normal mode |
| SPI_Forced/main.cpp | SPI forced mode |
| SPI_Normal_FIFO/main.cpp | SPI normal mode with FIFO buffer & interupt, 560mS |
| I2C_Normal/main.cpp | I2C normal mode |
| I2C_Forced/main.cpp | I2C forced mode |
| I2C_Normal_FIFO/main.cpp | I2C normal mode with FIFO buffer & interupt, 3.2 S |

## Features

### Power modes

BMP390 can be operated in three power modes. In sleep mode, no measurements are performed. Normal mode comprises an automated perpetual cycling between an active
measurement period and an inactive standby period. In forced mode, a single measurement is performed. When the measurement is finished, the sensor returns to sleep mode.

### Filter settings

The environmental pressure is subject to many short-term changes, caused e.g. by slamming of a door or window, or wind
blowing into the sensor. To suppress these disturbances in the output data without causing additional interface traffic and
processor work load, the BMP390 features an internal IIR filter. The filter coefficient can be set to 0 (bypass mode), 1, 3, 7, 15, 31, 63 or 127. The higher the filter coefficient, the stronger the filtering of the output data. The filter coefficient is set to 0 (bypass mode) by default.

### Oversampling settings

Noise depends on the oversampling and filter settings selected. The stated values were determined in a controlled pressure
environment and are based on the average standard deviation of 32 consecutive measurement points taken at highest
sampling rate. This is required to exclude long term drifts from the noise measurement.

### Output data rate (ODR)

The output data rate (ODR) is the rate at which the sensor updates its output data. Control settings for ODR are in odr_sel, data sheet 4.3.20. . The default ODR setting is 50Hz or 20mS. ODR values are in header file under ODR_e enumeration.

### Interrupt

The BMP390 has an interrupt pin that can be configured to trigger on various events, such as new data available, FIFO watermark reached, or FIFO full. The interrupt pin can be connected to any GPIO pin on the Raspberry Pi Pico and can be used to wake up the Pico from sleep mode or to signal the Pico to read data from the sensor.

### FIFO

The BMP390 has a FIFO buffer that can store up to 512 bytes of data. The FIFO can be configured to store pressure and temperature data, and can be set to trigger an interrupt when a certain number of bytes are stored in the FIFO (watermark). This allows for efficient data collection without the need for constant polling of the sensor. See the SPI_Normal_FIFO and I2C_Normal_FIFO examples for how to use the FIFO feature.

The BMP390 has a FIFO buffer that can store up to 512 bytes of data. The FIFO can be configured to store pressure and temperature data, and can be set to trigger an interrupt when a certain number of bytes are stored in the FIFO (watermark). This allows for efficient data collection without the need for constant polling of the sensor. See the SPI_Normal_FIFO and I2C_Normal_FIFO examples for how to use the FIFO feature.

The calculation for the interrupt time is:

1. Size of FIFO Buffer = 512 bytes
2. Bytes per frame = 7 bytes
3. Frame-watermark is defined by user in example file
4. ODR period is defined by the ODR set, the available values are in datasheet or ODR-e enum. default 20mS
5. Max frames = Size Of FIFO/bytes per frame
6. Frame watermark/bytes per frame = frames per_read
7. Interrupt time = frames per_read * ODR period.

| Example file | ODR period | Frame_watermark | interrupt time |
| ---- | --- | --- | --- |
| I2C_Normal_FIFO | ODR_50_Hz   20 mS | 200 | 560 mS |
| SPI_Normal_FIFO | ODR_1_5_Hz 640 mS | 35 | 3.2 S |

## Connections

The Sensor uses SPI or I2C for communication with the pico.
The BMP390 is a 3.3V device and the Pico is also a 3.3V device.
Some BMP390 breakout modules can be powered off 5 Volts. Check your module specifications before connecting to 5V.

### SPI Connections

The BMP390 can be connected to the Raspberry Pi Pico using SPI. The following table shows the pin connections between the BMP390 and the Raspberry Pi Pico spi0.
The BMP390 has a CS pin which can be connected to any GPIO pin on the Pico. Can be set up for any SPI interface and bus speed.

| BMP390 Pin | Function | Pico GPIO | Notes |
| ---- | ---- | --- | ---- |
| INT | interrupt pin | GPIO 22 | Optional, pick any GPIO |
| CS | Chip Select (CS) | GPIO 17 | Active Low pick any GPIO |
| SDO | MISO (Data Out) | GPIO 16 | Master In Slave Out |
| SDI | MOSI (Data In) | GPIO 19 | Master Out Slave In |
| SCK | SCK (Clock) | GPIO 18 | SPI Clock |
| VCC | Power | VCC | - |
| GND | Ground | GND | - |

### I2C Connections

The BMP390 can be connected to the Raspberry Pi Pico using I2C. The following table shows the pin connections between the BMP390 and the Raspberry Pi Pico I2c1.
I2C error timeout and baud rate can be adjusted. The BMP390 has a CSB pin , it should be pulled high. The I2C address of the BMP390 is 0x76 or 0x77 depending on the SDO pin connection.

| BMP390 Pin | Function | Pico GPIO | Notes |
| ---- | ---- | ---- | ---- |
| INT | interrupt pin | GPIO 22 | Optional, pick any GPIO |
| CS | Chip Select | n/a | set high |
| SDO | select I2C address | n/a | high: address=0x77. low: address=0x76 |
| SDI | Data | GPIO 18 | I2C Data |
| SCK | Clock | GPIO 19 | I2C Clock |
| VCC | Power | VCC | - |
| GND | Ground | GND | - |

## Debug Mode

Setting BMP390_DEBUG to 1 in the header file will enable verbose debug output during development. This can help with troubleshooting and understanding the sensor's behavior. When BMP390_DEBUG is set to 0, the library will operate in normal mode with less verbose output.

## Output

In example files data is outputted (eg to a PC) via a USB, 38400 Uart baud rate connection.

 ![ op](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/bmp390output.png)
