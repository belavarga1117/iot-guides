/*
 * config.h — Arduino Nano Matter + IS31FL3741 LED Matrix Node
 *
 * Hardware:
 * - Arduino Nano Matter (MGM240S)
 * - Grid-EYE AMG8833 (8×8 thermal sensor) via Qwiic
 * - Modulino Light (LTR381RGB ambient light sensor) via Qwiic
 * - Adafruit IS31FL3741 QT (13×9 RGB LED matrix) via Qwiic
 * All connected via Qwiic daisy-chain
 */

#ifndef CONFIG_H
#define CONFIG_H

// --- Node Identity ---
#define NODE_NAME        "NanoMatter-1"
#define NODE_ROOM        "Living Room"
#define NODE_ID          3

// --- Grid-EYE Sensor ---
// NOTE: SparkFun Qwiic Grid-EYE has AD_SELECT=HIGH → address is 0x69 (not default 0x68)
#define GRIDEYE_ADDR     0x69  // AD_SELECT=HIGH on SparkFun Qwiic breakout

// --- IS31FL3741 LED Matrix ---
// Adafruit QT board: 13×9 RGB, I2C 0x30
// Two Qwiic ports — either one works (same I2C bus)
#define MATRIX_ADDR      0x30
#define MATRIX_WIDTH     13
#define MATRIX_HEIGHT    9

// --- Presence Detection ---
#define PRESENCE_THRESHOLD   2.5f   // °C above ambient
#define MIN_HOT_PIXELS       2

// --- Display Settings ---
#define MATRIX_BRIGHTNESS    40     // Default global current (0-255), adjusted by light sensor
#define TEMP_COLD            20.0f  // Blue end of color scale
#define TEMP_HOT             35.0f  // Red end of color scale

// --- Update Rates ---
#define DISPLAY_UPDATE_MS    200    // 5Hz heatmap refresh
#define MATTER_UPDATE_MS     1000   // 1Hz Matter reports

// --- I2C Device Addresses Summary ---
// 0x30 = IS31FL3741 LED matrix
// 0x53 = Modulino Light (LTR381RGB ambient light sensor)
// 0x69 = Grid-EYE AMG8833 thermal sensor (AD_SELECT=HIGH)
// All on same Qwiic/I2C bus, no conflicts

#endif // CONFIG_H
