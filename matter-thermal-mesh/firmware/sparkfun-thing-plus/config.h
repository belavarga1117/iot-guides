/*
 * config.h — SparkFun Thing Plus Matter Node Configuration
 *
 * Change these per-node before flashing.
 */

#ifndef CONFIG_H
#define CONFIG_H

// --- Node Identity ---
#define NODE_NAME        "SparkFun-1"
#define NODE_ROOM        "Hallway"
#define NODE_ID          1

// --- Grid-EYE Sensors ---
// SparkFun Thing Plus: 1 Grid-EYE per board (Qwiic)
// NOTE: SparkFun Qwiic Grid-EYE has AD_SELECT=HIGH → address is 0x69 (not default 0x68)
#define GRIDEYE_COUNT    1
#define GRIDEYE_ADDR_1   0x69   // AD_SELECT=HIGH on SparkFun Qwiic breakout
// Uncomment if 2 Grid-EYEs daisy-chained (requires one board with AD_SELECT=LOW):
// #define GRIDEYE_COUNT  2
// #define GRIDEYE_ADDR_2 0x68

// --- Presence Detection ---
#define PRESENCE_THRESHOLD_ABS    28.0f  // °C absolute
#define PRESENCE_THRESHOLD_DELTA  2.5f   // °C above ambient
#define MIN_BLOB_PIXELS           2      // Minimum pixels for a "person"

// --- Matter Update Rate ---
#define MATTER_UPDATE_INTERVAL_MS 1000   // 1Hz updates to Matter attributes

// --- LED Configuration ---
// SparkFun Thing Plus Matter has a blue LED on pin LED_BUILTIN
#define STATUS_LED       LED_BUILTIN
#define LED_BLINK_ON_MS  100
#define LED_BLINK_OFF_MS 900

#endif // CONFIG_H
