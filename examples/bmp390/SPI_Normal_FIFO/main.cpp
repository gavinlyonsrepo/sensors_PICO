/*!
	@file examples/bmp390/SPI_Normal_FIFO/main.cpp
	@brief RPI PICO SDK C++ bmp390 library test, FIFO + interrupt, SPI hardware.
	@details Demonstrates the BMP390 FIFO and INT pin in Normal mode.
	  The sensor collects pressure + temperature frames into its 512-byte FIFO.
	  The FIFO watermark interrupt fires when the fill level reaches the threshold.
	  The Pico polls the INT pin (GPIO) and reads all available frames when it
	  goes high, then prints them. This avoids polling the sensor every loop
	  iteration and is the recommended low-power pattern.

	  Wiring (in addition to SPI lines):
		BMP390 INT pin → Pico GPIO 22
		No pull-up needed: INT is push-pull, active-high by default.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp390/bmp390.hpp"

// ---- SPI pins ----
#define SPI_PORT     spi0
#define SPI_BAUDRATE 500000   // 500 kHz
#define CS           17
#define MOSI         19
#define SCK          18
#define MISO         16

// ---- INT pin ----
#define INT_PIN      22       // Connect BMP390 INT pin here

// ---- Application settings ----
#define LOCAL_PRESSURE  1025.25   // Replace with today's QNH from forecast [hPa]
#define FIFO_WATERMARK  200       // Fire interrupt when FIFO reaches 200 bytes
								  // Each pressure+temperature frame = 7 bytes
								  // → ~28 frames before interrupt fires every 570mS
								  // Default ODR 20 mS (50Hz) (FIFO_WATERMARK / 7) * 0.02f);


// Maximum frames to parse per FIFO read.
// 512 bytes FIFO / 7 bytes per P+T frame ≈ 73 frames maximum.
#define MAX_FRAMES   80

BMP390_Sensor bmp390(SPI_PORT, SPI_BAUDRATE, CS, MOSI, SCK, MISO);

int main()
{
	stdio_init_all();
	sleep_ms(1000);
	printf("\n--- START BMP390 SPI FIFO + Interrupt ---\n");

	// ---- Initialise INT GPIO input ----
	gpio_init(INT_PIN);
	gpio_set_dir(INT_PIN, GPIO_IN);
	gpio_pull_down(INT_PIN); // keep defined when INT is de-asserted (active-high mode)

	// ---- Initialise sensor ----
	bmp390.InitSensor();

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
	// ---- Set FIFO watermark ----
	bmp390.setFifoWatermark(FIFO_WATERMARK);
		printf("FIFO watermark : %u bytes (%u frames, fires every ~%.1f s)\n",
		FIFO_WATERMARK,
		FIFO_WATERMARK / 7,
		(FIFO_WATERMARK / 7) * 0.02f);
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
	printf("--- END ---\n");
}
