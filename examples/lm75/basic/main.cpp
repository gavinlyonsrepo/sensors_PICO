/*!
 * @file main.cpp
 * @brief demo file for lm75 library : basic usage
 * @details This example test file demonstrates the use of the LM75A temperature sensor library.
 *          This example is for the Raspberry Pi Pico  microcontroller.
 * @author Gavin Lyons.
 * @link  https://github.com/gavinlyonsrepo/RPI_PICO_projects_list
 */

// *** Libraries ***
#include <stdio.h>
#include "pico/stdlib.h"
#include "lm75/lm75.hpp"


// *** Globals ***
// Default I2C address(0x48) for LM75A, can be changed with A0-A2 pins
uint8_t i2c_address = LM75A_DEFAULT_ADDRESS; 
LIB_LM75A lm75a(i2c_address, i2c0, 16, 17, 100);
uint8_t is_connected = 0; // test control
uint16_t test_count = 0 ; // test control

bool Temp_type_Celsius = true; // true for Celsius , False for Fahrenheit
float temperature = 0.0;
bool is_shutdown = false;

// *** Main ***
int main()
{
  stdio_init_all(); // Initialize chosen serial port
  busy_wait_ms(1000);
  printf("LM75 : Start!\r\n");
  lm75a.initLM75A(); 
  
  // Check for connection
  is_connected = lm75a.isConnected();
  if(is_connected == 0)
  {
    while (is_connected != 1) // Connection error
    {
     printf("LM75 : Connection error :: %i \n",lm75a.return_value);
     printf("LM75 : Is connected? :: %u \n", is_connected);
     busy_wait_ms(1500);
     is_connected = lm75a.isConnected();
    }
  }else{
     printf("LM75 : Is connected? :: %u \n", is_connected);
  }

  while (true)
  {
    printf("LM75 : Running Basic Test!\r\n");
    test_count++;
    printf("Test Count :: %u \n", test_count);
    // Celsius
    if (Temp_type_Celsius == true)
    {
      temperature = lm75a.getTemperature();
      temperature *= 100;
      printf("Temperature : %u.%u *C \n", (unsigned int)temperature / 100, (unsigned int)temperature % 100);
    }
    else
    {
      // Farenheit
      temperature = lm75a.getTemperatureInFarenheit();
      temperature *= 1000;
      printf("Temperature : %u.%u *F \n", (unsigned int)temperature / 1000,
             (unsigned int)temperature % 100);
    }
    // Hysteresis
    temperature = lm75a.getHysterisisTemperature();
    temperature *= 100;
    printf("Hysteris Temperature : %u.%u *C \n", (unsigned int)temperature / 100, (unsigned int)temperature % 100);
    // OS trip temperature
    temperature = lm75a.getOSTripTemperature();
    temperature *= 100;
    printf("OS trip  Temperature : %u.%u *C \n", (unsigned int)temperature / 100, (unsigned int)temperature % 100);
    // Shutdown
    printf("Shutting Down \n");
    lm75a.shutdown();
    is_shutdown = lm75a.isShutdown();
    printf("is shutdown? %u \n", ((unsigned int)is_shutdown));
    busy_wait_ms(5000);
    /// Wake up
    printf("Waking up \n");
    lm75a.wakeup();
    is_shutdown = lm75a.isShutdown();
    printf("is shutdown? %u \n", ((unsigned int)is_shutdown));
    busy_wait_ms(5000);
  } // end of while forever
} // *** End  of main ***


