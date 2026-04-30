/*!
	@file examples/bmp390/I2C_Normal_FIFO/main.cpp
	@brief RPI PICO SDK C++ bmp390 library test, FIFO + interrupt, I2C hardware.
	@details Demonstrates the BMP390 FIFO and INT pin in Normal mode.
	  The sensor collects pressure + temperature frames into its 512-byte FIFO.
	  The FIFO watermark interrupt fires when the fill level reaches the threshold.
	  The Pico polls the INT pin (GPIO) and reads all available frames when it
	  goes high, then prints them. This avoids polling the sensor every loop
	  iteration and is the recommended low-power pattern.

	  Wiring (in addition to I2C lines):
		BMP390 INT pin → Pico GPIO 22
		No pull-up needed: INT is push-pull, active-high by default.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp390/bmp390.hpp"

// ---- I2C pins ----
#define I2C_PORT        i2c0       // I2C port: i2c0 or i2c1
#define I2C_BAUDRATE    400000     // 400 kHz fast mode
#define I2C_TIMEOUT_US  50000      // 50 ms timeout
#define I2C_ADDRESS     0x76       // 0x76 = SDO LOW, 0x77 = SDO HIGH
#define SDA             16         // SDA GPIO pin
#define SCL             17         // SCL GPIO pin

// ---- INT pin ----
#define INT_PIN      22       // Connect BMP390 INT pin here

// ---- Application settings ----
#define LOCAL_PRESSURE  1025.25   // Replace with today's QNH from forecast [hPa]
// ---- FIFO configuration ----
// Frame size breakdown (pressure + temperature frame, datasheet Table 15):
//   1 header byte + 3 temperature bytes + 3 pressure bytes = 7 bytes/frame
//
// ODR_1_5_Hz = odr_sel 0x07 = 25/16 Hz = 1.5625 Hz → one frame every 640 ms
//
// Watermark calculation:
//   FIFO_WATERMARK (35 bytes) / 7 bytes per frame = 5 frames
//   Interrupt fires every: 5 frames × 640 ms = 3,200 ms (~3.2 seconds)
//
// FIFO capacity check:
//   512 bytes / 7 bytes per frame = 73 frames maximum before FIFO is full
//   FIFO full threshold (datasheet section 3.7.5.2) = 504 bytes = ~72 frames
//   Watermark (35 bytes) is well within capacity — no overflow risk
#define FIFO_WATERMARK  35
// MAX_FRAMES must be >= (FIFO_WATERMARK / 7) to avoid discarding frames.
// Set to 80 to safely cover the full FIFO (73 frames max) with margin.
#define MAX_FRAMES   80

BMP390_Sensor bmp390(I2C_PORT, I2C_BAUDRATE, I2C_TIMEOUT_US, I2C_ADDRESS, SDA, SCL);

void SetupInterruptPin(void);
void DeinitInterruptPin(void);

int main()
{
	stdio_init_all();
	sleep_ms(1000);
	printf("\n--- START BMP390 I2C FIFO + Interrupt ---\n");

	// ---- Initialise INT GPIO input ----
	SetupInterruptPin();

	// ---- Initialise sensor ----
	// Optional: check device is on the bus before full initialisation
	BMP390_Sensor probe(I2C_PORT, I2C_BAUDRATE, I2C_TIMEOUT_US, I2C_ADDRESS, SDA, SCL);
	bmp390.InitSensor();

	int16_t connCheck = bmp390.CheckConnectionI2C();
	if (connCheck < 1)
	{
		printf("BMP390 not found on I2C bus (address 0x%02X). Check wiring.\n", I2C_ADDRESS);
		return -1;
	}
	printf("BMP390 found on I2C bus.\n");

	uint8_t chipID = bmp390.readForChipID();
	printf("Chip ID      : 0x%02X\n", chipID);
	printf("Power mode   : %u\n", static_cast<uint8_t>(bmp390.readPowerMode()));

	// ---- Configure interrupt pin ----
	// Push-pull, active-high, non-latched, FIFO watermark source enabled.
	// drdy and ffull are left disabled for this example.
	bmp390.configureInterrupt(
		BMP390_Sensor::IntOutputMode_e::PushPull,
		BMP390_Sensor::IntLevel_e::ActiveHigh,
		BMP390_Sensor::IntLatch_e::NonLatched,
		false,   // drdy_en  — not used here
		false,   // ffull_en — not used here
		true     // fwtm_en  — fire when watermark is reached
	);
	printf("INT configured: push-pull, active-high, watermark source\n");
	// ---- ODR and watermark -----------------------------------------------
	// ODR_1_5_Hz: odr_sel=0x07, period=640ms (datasheet Table 45: 25/16 Hz)
	// At this rate the sensor produces ~1.5 frames/sec into the FIFO.
	// With FIFO_WATERMARK=35 bytes (5 frames), the interrupt fires every ~3.2s.
	// Changing FIFO_WATERMARK changes the interrupt rate:
	//   7  bytes ( 1 frame) → ~0.64s    14 bytes ( 2 frames) → ~1.3s
	//   35 bytes ( 5 frames) → ~3.2s    70 bytes (10 frames) → ~6.4s
	//   350 bytes (50 frames) → ~32s    490 bytes (70 frames) → ~45s
	bmp390.setODR(BMP390_Sensor::ODR_e::ODR_1_5_Hz);
	bmp390.setFifoWatermark(FIFO_WATERMARK);  // fires ~every 5 seconds   
	printf("FIFO watermark : %u bytes (%u frames, fires every ~%.1f s)\n",
		FIFO_WATERMARK,
		FIFO_WATERMARK / 7,
		(FIFO_WATERMARK / 7) * 0.64f);
	// ---- Enable FIFO (press + temp, unfiltered, no subsampling) -----
	bmp390.enableFifo(
		true,                                       // pressEn
		true,                                       // tempEn
		false,                                      // timeEn  — skip sensortime frames
		BMP390_Sensor::FifoOverflow_e::Streaming,   // overwrite oldest on full
		BMP390_Sensor::FifoDataSelect_e::Unfiltered,
		BMP390_Sensor::FifoSubsampling_e::Div1
	);
	printf("FIFO enabled: press+temp, unfiltered, no subsampling\n\n");

	// ---- Main loop ----
	BMP390_Sensor::FifoFrame_t frames[MAX_FRAMES];
	uint32_t batchCount = 0;

	while (batchCount < 20) // collect 20 watermark events then stop
	{
		// Poll INT pin — no RTOS or IRQ handler needed for this example.
		// In a real application you would use gpio_set_irq_enabled_with_callback().
		if (!gpio_get(INT_PIN))
		{
			tight_loop_contents(); // yield — no busy work while waiting
			continue;
		}
		// INT is high: watermark (or full) condition met.
		uint16_t fillLevel = bmp390.getFifoLength();
		printf("=== Batch %lu — FIFO fill: %u bytes ===\n", batchCount, fillLevel);
		// Read INT_STATUS to clear the interrupt and de-assert INT pin.
		BMP390_Sensor::IntStatus_t status = bmp390.readIntStatus();
		printf("INT_STATUS: watermark=%u  full=%u  drdy=%u\n",
			status.fifoWatermark, status.fifoFull, status.dataReady);
		// Parse all available frames from the FIFO.
		uint8_t count = bmp390.readFifoFrames(frames, MAX_FRAMES);
		printf("Frames parsed: %u\n", count);
		for (uint8_t f = 0; f < count; f++)
		{
			if (frames[f].hasTemperature && frames[f].hasPressure)
			{
				double altM = bmp390.readAltitude(LOCAL_PRESSURE);
				printf("  [%2u] T=%.2f C  P=%.2f hPa  Alt=%.1f m\n",
					f,
					frames[f].temperature,
					frames[f].pressure / 100.0, // Pa → hPa
					altM);
			}
			else if (frames[f].hasTemperature)
			{
				printf("  [%2u] T=%.2f C  (no pressure)\n", f, frames[f].temperature);
			}
			else if (frames[f].hasPressure)
			{
				printf("  [%2u] P=%.2f hPa  (no temperature)\n",
					f, frames[f].pressure / 100.0);
			}
			else if (frames[f].hasSensortime)
			{
				printf("  [%2u] Sensortime=%lu\n", f, (unsigned long)frames[f].sensortime);
			}
		}
		printf("\n");
		batchCount++;
	} // while batchs loop

	// --- Clean up ---
	bmp390.disableFifo();
	bmp390.flushFifo();
	bmp390.DeInitSensor();
	DeinitInterruptPin();
	printf("--- END ---\n");
}

void SetupInterruptPin()
{
	gpio_init(INT_PIN);
	gpio_set_dir(INT_PIN, GPIO_IN);
	gpio_pull_down(INT_PIN); // keep defined when INT is de-asserted (active-high mode)
}

void DeinitInterruptPin()
{
	gpio_set_function(INT_PIN, GPIO_FUNC_NULL);
	gpio_deinit(INT_PIN);
}