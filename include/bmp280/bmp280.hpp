/*!
	@file bmp280.hpp
	@brief library header file for bmp280 pressure sensor 
	@todo add I2C support
*/

#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include <cmath> // for pow function

#define BMP280_DEBUG 0

class BMP280_Sensor
{
public:
	BMP280_Sensor(spi_inst_t *spi, uint32_t baudRate, uint8_t cs, uint8_t mosi, uint8_t sck, uint8_t miso);
	BMP280_Sensor(i2c_inst_t *i2c, uint32_t baudrate, uint32_t timeOutDelay, uint8_t address, uint8_t sda, uint8_t scl);
	~BMP280_Sensor();

	/*! @brief Communications mode*/
	enum class CommMode_e : uint8_t
	{
		SPI = 0, /**< SPI communication mode */
		I2C = 1  /**< I2C communication mode */
	};

	/*! @brief Enumeration for BMP280 power modes */
	enum class PowerMode_e : uint8_t
	{
		Sleep = 0, /**< No measurements are performed in this mode */
		Forced = 1, /**< a single measurement is performed. When the measurement is finished, the sensor returns to sleep mode.*/
		Normal = 2  /**< Comprises an automated perpetual cycling between an active measurement period and an inactive standby period */
	};

	/*! @brief Enumeration for BMP280 data types */
	enum class DataType_e : uint8_t
	{
		Temperature = 0, /**< Temperature */
		Pressure    = 1  /**< Pressure */
	};

	/*! @brief Enumeration for BMP280 registers set*/
	enum Registers_e : uint8_t
	{
		DIG_T1_Reg = 0x88, /**< Trimming parameter Start Address*/
		Chip_ID = 0xD0,	   /**<  Chip ID register The “id” register contains the chip identification number chip*/
		Reset = 0xE0,	   /**< The “reset” register contains the soft reset word reset[7:0]*/
		Status = 0xF3,	   /**< The “status” register contains two bits which indicate the status of the device.*/
		Ctrl_Meas = 0xF4,  /**< The “ctrl_meas” register sets the data acquisition options of the device.*/
		Config = 0xF5,	   /**< The “config” register sets the rate, filter and interface options of the device.*/
		Press_MSB = 0xF7,  /**< “press” register contains the raw pressure measurement, Contains the MSB part up [19:12]*/
		Press_LSB = 0xF8,  /**< “press” register contains the raw pressure measurement, Contains the LSB part up [11:04]*/
		Press_XLSB = 0xF9, /**< “press” register contains the raw pressure measurement, Contains the XLSB part up[3:0]*/
		Temp_MSB = 0xFA,   /**< The “temp” register contains the raw temperature measurement output data, [19:12 */
		Temp_LSB = 0xFB,   /**< The “temp” register contains the raw temperature measurement output data, [11:4]*/
		Temp_XLSB = 0xFC   /**< The “temp” register contains the raw temperature measurement output data, [3:0]*/
	};

	/*! @brief Enumeration for pressure unit */
	enum class PressureUnit_e : uint8_t
	{
		Pa = 0,  /**< Pascal */
		hPa = 1, /**< Hectopascal */
	};

	/*! @brief Enumeration for sensor oversampling */
	enum class sensorSampling_e : uint8_t 
	{
		Sampling_None = 0x00, /**< No over-sampling. */
		Sampling_X1 = 0x01,   /**< 1x over-sampling. */
		Sampling_X2 = 0x02,   /**< 2x over-sampling. */
		Sampling_X4 = 0x03,   /**< 4x over-sampling. */
		Sampling_X8 = 0x04,   /**< 8x over-sampling. */
		Sampling_X16 = 0x05   /**< 16x over-sampling. */
	};
	
	/*! @brief Filtering level for sensor data */
	enum Filter_e : uint8_t {
		Filter_OFF  = 0x00, /**< No filtering. */
		Filter_X2   = 0x01, /**< 2x filtering. */
		Filter_X4   = 0x02, /**< 4x filtering. */
		Filter_X8   = 0x03, /**< 8x filtering. */
		Filter_X16  = 0x04  /**< 16x filtering. */
	};

	/*! @brief Standby duration between measurements (in milliseconds). */
	enum StandBy_e : uint8_t {
		StandBy_MS_1     = 0x00, /**< 1 ms standby. */
		StandBy_MS_63    = 0x01, /**< 62.5 ms standby. */
		StandBy_MS_125   = 0x02, /**< 125 ms standby. */
		StandBy_MS_250   = 0x03, /**< 250 ms standby. */
		StandBy_MS_500   = 0x04, /**< 500 ms standby. */
		StandBy_MS_1000  = 0x05, /**< 1000 ms standby. */
		StandBy_MS_2000  = 0x06, /**< 2000 ms standby. */
		StandBy_MS_4000  = 0x07  /**< 4000 ms standby. */
	};

	/*! @brief Encapsulates the BMP280 config register.*/
	struct config_t {
		/*! @brief Initialize to power-on-reset state. */
		config_t() : timeSb(StandBy_e::StandBy_MS_1), filter(Filter_e::Filter_OFF), none(0), spi3w_en(0) {}
		uint8_t timeSb    : 3; /**< Inactive duration (standby time) in normal mode. */
		uint8_t filter    : 3; /**< Filter settings. */
		uint8_t none      : 1; /**< Unused - don't set. */
		uint8_t spi3w_en  : 1; /**< Enables 3-wire SPI. */
		/*!
		 * @brief Retrieve the assembled config register's byte value.
		 * @return 8-bit register value combining all fields.
		 */
		uint8_t get() { return (timeSb << 5) | (filter << 2) | spi3w_en; }
	};

	void InitSensor(void);
	void DeInitSensor(void);
	int16_t CheckConnectionI2C(void);

	bool readConfig();
	bool writeConfig(StandBy_e standby, Filter_e filter, bool spi3wEn);
	
	uint8_t readForChipID();
	uint8_t getChipID() const;
	int32_t getData(Registers_e reg, bool threeRegsRead);

	bool setRegister(Registers_e reg, uint8_t config, bool check = false);
	uint8_t readRegister(Registers_e reg);

	bool setPowerMode(PowerMode_e mode, bool check = false);
	PowerMode_e readPowerMode();
	PowerMode_e getPowerMode() const;

	void reset();
	bool setOversampling(DataType_e type, sensorSampling_e oversampling, bool check = false);
	sensorSampling_e readOversampling(DataType_e type);
	sensorSampling_e getOversampling(DataType_e type) const;
	bool takeForcedMeasurement(void);

	int32_t readRawTemperature();
	int32_t getRawTemperature() const;
	double readTemperature();
	double getTemperature() const;

	uint32_t readRawPressure();
	uint32_t getRawPressure() const;
	double readPressure(PressureUnit_e unit = PressureUnit_e::Pa);
	double getPressure(PressureUnit_e unit = PressureUnit_e::Pa) const; 

	double readAltitude(double seaLevelhP);
	double seaLevelForAltitude(double altitude, double atmospheric);

private:
	i2c_inst_t *_i2cInst;   /**< I2C instance */
	uint32_t _baudrate;     /**< I2C baudrate */
	uint32_t _timeOutDelay; /**< I2C timeout delay in uS */
	uint8_t _address;       /**< I2C address */
	spi_inst_t *_spiInst;   /**< SPI instance */
	uint32_t _baudRate;     /**< SPI  or I2C baudrate in hertz */
	static constexpr uint8_t _SPI_COMM_MASK = 0x80; /**< SPI communication mask*/
	uint8_t _cs;            /**< Chip select pin , SPI Only*/
	uint8_t _mosi;          /**< Master out slave in pin for SPI, of SDA for I2C */
	uint8_t _sck;           /**< Serial clock pin , SPI and I2C */
	uint8_t _miso;          /**< Master in slave out pin , SPI only*/

	uint8_t _chipID;                  /**< Chip ID 0x56-0X58 BMP280 , 0x60 BME280*/
	sensorSampling_e _temperatureOversampling; /**< Temperature Over sampling */
	sensorSampling_e _pressureOversampling;    /**< Pressure Over sampling */
	PowerMode_e _powerMode; /**< Power mode of the sensor */
	CommMode_e _commMode;  /**< Communication mode of the sensor */
	config_t _configReg;    /**< Configuration register instance */

	int32_t _rawTemperature;  /**< Raw temperature */
	double _temperature;      /**< Temperature in celsius deg double */
	int32_t _tFine;
	uint32_t _rawPressure; /**< Raw pressure */
	double _pressure;      /**< Pressure in Pa */
	int16_t OverSamplingDelay = 100; /**< Over sampling delay in milliseconds */

	/*! @brief Calibration data structure */
	/*! @details The calibration data is stored in the chip's memory and is used to correct the raw sensor data. */
	struct 
	{
		uint16_t dig_T1; /**< dig_T1 cal register. */
		int16_t dig_T2;  /**<  dig_T2 cal register. */
		int16_t dig_T3;  /**< dig_T3 cal register. */
		uint16_t dig_P1; /**< dig_P1 cal register. */
		int16_t dig_P2;  /**< dig_P2 cal register. */
		int16_t dig_P3;  /**< dig_P3 cal register. */
		int16_t dig_P4;  /**< dig_P4 cal register. */
		int16_t dig_P5;  /**< dig_P5 cal register. */
		int16_t dig_P6;  /**< dig_P6 cal register. */
		int16_t dig_P7;  /**< dig_P7 cal register. */
		int16_t dig_P8;  /**< dig_P8 cal register. */
		int16_t dig_P9;  /**< dig_P9 cal register. */
	} calib_data_t;

	bool getTrimmingParameters(void);
	void StartUpRoutine(void);
};
