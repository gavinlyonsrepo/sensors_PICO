/*
	@file dht11.cpp
	@brief source file for DHT11 temperature and humidity sensor library for rpi Pico
*/

#include "../../include/dht11/dht11.hpp"

/*! 
	* @brief Constructor for the DHT11_SENSOR class
	* @param data Data pin for sensor
*/
DHT11_SENSOR::DHT11_SENSOR(uint8_t data,  uint32_t GPIOWaitTimeout) 
{
	_DATA_IO = data;
	_GPIO_WaitTimeout =  GPIOWaitTimeout;
}

/*! 
	 @brief init the GPIO line for sensor 
*/
void DHT11_SENSOR::SensorInit(void)
{
	 gpio_init(_DATA_IO);
}

/*! 
	 @brief de-init the GPIO line for sensor 
*/
void DHT11_SENSOR::SensorDeInit(void)
{
	gpio_set_dir(_DATA_IO, GPIO_IN);
	gpio_deinit(_DATA_IO);
}


/*! 
	 @brief print the data to console
	 @param celsius  Celsius [°C] or Fahrenheit [°F]
	 @return 
	 		 0 Success 
			-1 Sensor did not response
			-2 Check sum byte bad,  Check Sum byte data = Humdity data + temperature data
*/
int DHT11_SENSOR::PrintSensorData(bool celsius)
{
	if (SensorStatus.CheckSensor  == 0) // DHT did not respond 
	{
		printf(" Error 1:: SensorPrintData :: Sensor did not Respond \n");
		return -1;
	}else if (SensorStatus.CheckSensor  == 1) // Good respond, display data
	{
		if (CheckSumCheck())  // is the Checksum OK?
		{
			if (celsius){
				printf("Temperature :: %u 'C\n",SensorStatus.Temperature);
			}else {
				int Fahrenheit = (SensorStatus.Temperature * 9) / 5 + 32;
				printf("Temperature :: %i 'F\n",Fahrenheit);

			}
			printf("Humidity :: %u %%\n",SensorStatus.Humdity);
		}else 
		{
			printf(" Error 2:: SensorPrintData :: Bad Checksum\n");
			return -2;
		}
	}
	return 0;
}

/*! 
	 @brief read one data byte from sensor 
	 @return the databyte, 0XFF for timeout error
*/
uint8_t DHT11_SENSOR::ReadDataByte(void)
{
	uint8_t dataByte = 0;
	uint8_t bits = 0;
	uint32_t timeout =  _GPIO_WaitTimeout;

	for(bits = 0; bits < 8; bits++)
	{
		//Wait until GPIO goes HIGH (50uS)
		while(!gpio_get(_DATA_IO) && timeout--) busy_wait_us(1);
		if (timeout == 0) {
			printf(" Error 1:: ReadDataByte :: GPIO timeout exceeded %lu\n", _GPIO_WaitTimeout);	
			return 0xFF; // Error: Timeout waiting for HIGH
		}
		busy_wait_us(ReadSignal);
		if(gpio_get(_DATA_IO) == 0)
		{
			 dataByte &= ~(1 << (7 - bits));  // Its a zero Clear bit 
		}else {
			dataByte |= (1 << (7 - bits));  // Its a 1 Set bit 
			// Wait until GPIO goes LOW, with timeout
			timeout = _GPIO_WaitTimeout; //reset timeout
			while(gpio_get(_DATA_IO) && timeout--) busy_wait_us(1); //Wait until GPIO goes LOW
			if (timeout == 0) {
				printf(" Error 2:: ReadDataByte :: GPIO timeout exceeded %lu\n", _GPIO_WaitTimeout);	
				return 0xFF; // Error: Timeout waiting for LOW
			}
		}
	}
 
 return dataByte;
}

/*! 
	 @brief Send start signal to sensor, follow this with CheckResponse
*/
void DHT11_SENSOR::StartSignal(void)
{
	gpio_set_dir(_DATA_IO, GPIO_OUT); //Set GPIO as output
	gpio_put(_DATA_IO, false); //GPIO sends 0 to DHT11 (request)
	busy_wait_ms(StartSignalOne); // Wait at least 18mS
	gpio_put(_DATA_IO, true); //GPIO pull up voltage to DHT11 
	busy_wait_us(StartSignalTwo);  // wait for response 20-40uS
	gpio_set_dir(_DATA_IO, GPIO_IN); //Set GPIO as input
}

/*! 
	 @brief check sensors reponse done after Sending start signal
	 @note Sets SensorStatus.CheckSensor 1 for success , 0 for failure
*/
void DHT11_SENSOR::CheckResponse(void)
{
	SensorStatus.CheckSensor = 0;
	busy_wait_us(CheckSignalOne); 
	if (gpio_get(_DATA_IO) == 0) // DHT11 has pulled line low
	{
		busy_wait_us(CheckSignalTwo);
		if (gpio_get(_DATA_IO) == 1) // DHT has pulled line high
		{
			SensorStatus.CheckSensor = 1;  // Good response! 
		}
		busy_wait_us(CheckSignalThree);  // wait 40uS before transmission of data
	}
}

/*! 
	 @brief read all 3(5 total 2 discarded) bytes from sensor done 
	 	after checking response of sensor.
*/
void DHT11_SENSOR::ReadSensorData(void)
{
	SensorStatus.Humdity = ReadDataByte(); // intergal Humidity byte 
	ReadDataByte();   // discard decimal byte
	SensorStatus.Temperature = ReadDataByte(); //intergal temperature byte 
	ReadDataByte();  // discard decimal byte
	SensorStatus.CheckSum = ReadDataByte(); // checksum byte
}

/*! 
	 @brief Check intergity of sensor data 
	 @details Does the CheckSum data byte = Humdity data byte + Temperature data byte
	 @return true for success 
*/
bool DHT11_SENSOR::CheckSumCheck(void)
{
	if(SensorStatus.CheckSum == (SensorStatus.Humdity + SensorStatus.Temperature))
		return true;
	else	
		return false;
}