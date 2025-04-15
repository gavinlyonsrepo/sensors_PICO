/*!
	@file examples/dht11/main.cpp
	@brief RPI PICO RP2040  SDK C++ dht11 library test file
*/

// Standard
#include "pico/stdlib.h"
// Custom libraries
#include "dht11/dht11.hpp"

// test related
const uint StatusLEDPin = 25; // PICO on_board Status LED,optional.
uint8_t testCount = 5; // number of tests to carry out
uint32_t testDelay = 7000; // delay between sensor reads in mS, min 2 seconds.

// dht11 sensor related
bool CelsiusOrFahrenheit = true; // Celsius = true 
#define SENSOR_GPIO 1 // GPIO  number of sensor line
#define GPIO_WAIT_TIMEOUT 10000 // uS GPIO timeout delay in event of sensor failure, prevents lockup. min > 50uS 
DHT11_SENSOR  myDHT11(SENSOR_GPIO, GPIO_WAIT_TIMEOUT);

// === Function Prototypes ===
void Setup(void);

// === Main ====
int main()
{
	Setup();
	while(testCount--)
	{
		printf("Test count left :: %i \n", testCount); // print out test count
		gpio_put(StatusLEDPin,testCount % 2); // toggle on board LED between reads
		myDHT11.StartSignal(); //send start signal to DHT11 sensor
		myDHT11.CheckResponse(); //get response from DHT11 sensor
		if (myDHT11.SensorStatus.CheckSensor == 1) // Did DHT11 respond ? 
		{
			myDHT11.ReadSensorData(); // Read the data into SensorStatus struct
			myDHT11.PrintSensorData(CelsiusOrFahrenheit); // print data to console
		}else{
			printf(" Error 1:: main:: Sensor did not Respond \n");
		}
		busy_wait_ms(testDelay);
	}	
	myDHT11.SensorDeInit();
	printf("Test End\n");
	return 0;
}

// === End  of main ===


// === Function Space ===
void Setup(void)
{
	busy_wait_ms(100);

	// Initialize Status LED pin
	gpio_init(StatusLEDPin);
	gpio_set_dir(StatusLEDPin, GPIO_OUT);
	gpio_put(StatusLEDPin, true);

	// Init default  serial port, baud 38400
	stdio_init_all();
	busy_wait_ms(1000);

	myDHT11.SensorInit(); // Init the sensor GPIO. 
	printf("\nTest Start\n");
}

