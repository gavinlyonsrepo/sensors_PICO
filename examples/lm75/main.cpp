/*
 * Project Name: Library for the LM75A temperature sensor by NXP and Texas Instruments.
 * File: main.cpp
 * Description: main test file for lm75 library ::  two modes LIB_BASIC LIB_TEST
 * Author: Gavin Lyons.
 * IDE:  Rpi=PICo rp2040 C++
 * Created Sep 2022
 * Description: See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/RPI_PICO_projects_list
 */

// *** Libraries ***
#include <stdio.h>
#include "pico/stdlib.h"
#include "lm75/lm75.hpp"

// *** Mode select ***
// mode 1 or mode 2
#define LIB_BASIC // Mode 1 :: Basic Usage
//#define LIB_TEST // Mode 2 :: Full library test example

// *** Globals ***
LIB_LM75A lm75a(LM75A_DEFAULT_ADDRESS, i2c0, 16, 17, 100);
uint8_t is_connected = 0; // test control
uint16_t test_count = 0 ;   // test control

#ifdef LIB_BASIC
bool Temp_type_Celsius = true; // true for Celsius , False  for Farenheit
bool basic_usage_or_test = true;   // true for basic , false for full library test
float temperature = 0.0;
bool is_shutdown = false;
#endif

#ifdef LIB_TEST

void checkTrueValue(const char *caption, const bool value, const bool expected);
const char *getFaultQueueValueString(const FaultQueueValue value);
const char *getOsPolarityString(const OsPolarity value);
const char *getDeviceModeString(DeviceMode value);
void checkFaultQueueValue(const char *caption, const FaultQueueValue value, const FaultQueueValue expected);
void checkOsPolarityValue(const char *caption, const OsPolarity value, const OsPolarity expected);
void checkDeviceModeValue(const char *caption, const DeviceMode value, const DeviceMode expected);
void checkTemperatureResult(const char *caption, const float value);
void checkTemperatureValue(const char *caption, const float value, const float expected);

bool testResult = true;
float oldHysterisisTemperature;
float oldOsTripTemperature;
FaultQueueValue oldFaultQueueValue;
OsPolarity oldOsPolarity;
DeviceMode oldDeviceMode;
float newHysterisisTemperature = 62.0f;
float newOsTripTemperature = 59.0f;
FaultQueueValue newFaultQueueValue = FaultQueueValue::NUMBER_OF_FAULTS_6;
OsPolarity newOsPolarity = OsPolarity::OS_POLARITY_ACTIVEHIGH;
DeviceMode newDeviceMode = DeviceMode::DEVICE_MODE_INTERRUPT;
#endif

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
    printf("LM75 : Running Test!\r\n");
    test_count++;
    printf("Test Count :: %u \n", test_count);
#ifdef LIB_BASIC
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
    // Hysterisis
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

#endif // end of LIB_Basic

#ifdef LIB_TEST
    // Test 1 : Test comms
    checkTrueValue("isConnected", lm75a.isConnected(), true);
    busy_wait_ms(1000);

    // Test 2 testing temperature getters (4*)
    checkTemperatureResult("getTemperature", lm75a.getTemperature());
    checkTemperatureResult("getTemperatureInFarenheit", lm75a.getTemperatureInFarenheit());
    checkTemperatureResult("getHysterisisTemperature", lm75a.getHysterisisTemperature());
    checkTemperatureResult("getOSTripTemperature", lm75a.getOSTripTemperature());
    // Test 3 Testing other getters
    checkFaultQueueValue("getFaultQueueValue", lm75a.getFaultQueueValue(), FaultQueueValue::NUMBER_OF_FAULTS_1);
    checkOsPolarityValue("getOsPolarity", lm75a.getOsPolarity(), OsPolarity::OS_POLARITY_ACTIVELOW);
    checkDeviceModeValue("getDeviceMode", lm75a.getDeviceMode(), DeviceMode::DEVICE_MODE_COMPARATOR);
    busy_wait_ms(2000);

    // Test 4 : Testing temperature setters
    oldHysterisisTemperature = lm75a.getHysterisisTemperature();
    oldOsTripTemperature = lm75a.getOSTripTemperature();

    lm75a.setHysterisisTemperature(newHysterisisTemperature);
    lm75a.setOsTripTemperature(newOsTripTemperature);

    checkTemperatureValue("setHysterisisTemperature", lm75a.getHysterisisTemperature(), newHysterisisTemperature);
    checkTemperatureValue("setOsTripTemperature", lm75a.getOSTripTemperature(), newOsTripTemperature);

    lm75a.setHysterisisTemperature(oldHysterisisTemperature);
    lm75a.setOsTripTemperature(oldOsTripTemperature);
    busy_wait_ms(2000);

    // Test 5 "Testing other setters");
    oldFaultQueueValue = lm75a.getFaultQueueValue();
    oldOsPolarity = lm75a.getOsPolarity();
    oldDeviceMode = lm75a.getDeviceMode();

    lm75a.setFaultQueueValue(newFaultQueueValue);
    lm75a.setOsPolarity(newOsPolarity);
    lm75a.setDeviceMode(newDeviceMode);
    checkFaultQueueValue("setFaultQueueValue: ", lm75a.getFaultQueueValue(), newFaultQueueValue);
    checkOsPolarityValue("setOsPolarity", lm75a.getOsPolarity(), newOsPolarity);
    checkDeviceModeValue("setDeviceMode", lm75a.getDeviceMode(), newDeviceMode);

    lm75a.setFaultQueueValue(oldFaultQueueValue);
    lm75a.setOsPolarity(oldOsPolarity);
    lm75a.setDeviceMode(oldDeviceMode);
    busy_wait_ms(2000);
    // Test 6
    lm75a.shutdown();
    checkTrueValue("shutdown", lm75a.isShutdown(), true);
    busy_wait_ms(2000);
    lm75a.wakeup();
    checkTrueValue("shutdown", lm75a.isShutdown(), false);
    busy_wait_ms(2000);

    printf("\nTest Result :: %u\n  ", (uint8_t)testResult);
    while (1)
    {
    }; // stay here :: test over
#endif // end of Lib TEST

  } // end of while forever
} // *** End  of main ***

// *** Function Space ***

#ifdef LIB_TEST

void checkTrueValue(const char *caption, const bool value, const bool expected)
{
  bool fail = value != expected;

  if (fail)
  {
    testResult = false;
    printf("%s : %u FAIL \r\n\n", caption, (uint8_t)value);
  }
  else
  {
    printf("%s : %u: PASS\r\n\n", caption, (uint8_t)value);
  }
}

const char *getFaultQueueValueString(const FaultQueueValue value)
{
  switch (value)
  {
  case FaultQueueValue::NUMBER_OF_FAULTS_1:
    return "NUMBER_OF_FAULTS_1";
  case FaultQueueValue::NUMBER_OF_FAULTS_2:
    return "NUMBER_OF_FAULTS_2";
  case FaultQueueValue::NUMBER_OF_FAULTS_4:
    return "NUMBER_OF_FAULTS_4";
  case FaultQueueValue::NUMBER_OF_FAULTS_6:
    return "NUMBER_OF_FAULTS_4";
  default:
    return "** ILLEGAL VALUE **";
  }
}

const char *getOsPolarityString(const OsPolarity value)
{
  switch (value)
  {
  case OsPolarity::OS_POLARITY_ACTIVEHIGH:
    return "OS_POLARITY_ACTIVEHIGH";
  case OsPolarity::OS_POLARITY_ACTIVELOW:
    return "OS_POLARITY_ACTIVELOW";
  default:
    return "** ILLEGAL VALUE **";
  }
}

const char *getDeviceModeString(DeviceMode value)
{
  switch (value)
  {
  case DeviceMode::DEVICE_MODE_COMPARATOR:
    return "DEVICE_MODE_COMPARATOR";
  case DeviceMode::DEVICE_MODE_INTERRUPT:
    return "DEVICE_MODE_INTERRUPT";
  default:
    return "** ILLEGAL VALUE **";
  }
}

void checkTemperatureValue(const char *caption, const float value, const float expected)
{
  bool fail = value != expected;

  if (fail)
  {
    testResult = false;
    printf("%s : %u: %u FAIL\r\n", caption, (uint8_t)value, (uint8_t)expected);
  }
  else
  {
    printf("%s : %u: PASS\r\n", caption, (uint8_t)value);
  }
}

void checkTemperatureResult(const char *caption, const float value)
{
  bool fail = value == LM75A_INVALID_TEMPERATURE;
  if (fail)
  {
    testResult = false;
    printf("%s : %u: FAIL LM75A_INVALID_TEMPERATURE\r\n", caption, (uint8_t)value);
  }
  else
  {
    printf("%s : %u: PASS\r\n", caption, (uint8_t)value);
  }
}

void checkOsPolarityValue(const char *caption, const OsPolarity value, const OsPolarity expected)
{
  bool fail = value != expected;
  if (fail)
  {
    testResult = false;
    printf("%s : %d: FAIL \r\n", caption, (uint8_t)expected);
  }
  else
  {
    printf("%s : %d: PASS\r\n", caption, (uint8_t)expected);
  }
}

void checkDeviceModeValue(const char *caption, const DeviceMode value, const DeviceMode expected)
{
  bool fail = value != expected;

  if (fail)
  {
    testResult = false;
    printf("%s : %d: FAIL Expected\r\n", caption, (uint8_t)expected);
  }
  else
  {
    printf("%s : %d: PASS \r\n\n", caption, (uint8_t)expected);
  }
}

void checkFaultQueueValue(const char *caption, const FaultQueueValue value, const FaultQueueValue expected)
{
  bool fail = value != expected;
  if (fail)
  {
    testResult = false;
    printf("%s : %d: FAIL Expected\r\n", caption, (uint8_t)expected);
  }
  else
  {
    printf("%s : %d: PASS \r\n", caption, (uint8_t)expected);
  }
}

#endif