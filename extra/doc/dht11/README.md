  [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/whitelight976)

# DHT11_PICO

## DHT11 Sensor library

Raspberry pi Pico SDK C++ library for the DHT11 Temperature & Humidity Sensor.
This one line sensor features a temperature & humidity sensor complex 
with a calibrated digital signal output. 
Sensor can be powered by 3.3V supply and only requires one GPIO line.
Communication Format with DHT11 can be separated into four stages.

1. Request.
2. Response.
3. Data Reading 5 bytes.
4. Sum the 1st 4 bytes and check if the result is the same as CheckSum(5th byte).

[Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)

![ Pinout](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/pinout.jpg)

## Console Output

![ Output](https://github.com/gavinlyonsrepo/sensors_PICO/blob/main/extra/images/output.png)
