
/*
   Matter Thermal Sensor — Full Firmware (SparkFun Thing Plus Matter)

   Board: SparkFun Thing Plus Matter (MGM240P)
   Protocol: Matter over Thread

   Sensors (all via Qwiic daisy-chain — zero soldering):
   - Grid-EYE AMG8833: 8x8 thermal camera (I2C 0x69, AD_SELECT=HIGH)
   - SCD40: CO2, temperature, humidity (I2C 0x62)
   - SparkFun Micro OLED: 64x48 monochrome display (I2C 0x3D)

   Matter endpoints (visible in Home Assistant):
   - MatterTemperature: max pixel temperature from Grid-EYE
   - MatterOccupancy: binary presence detection (hot pixel count)
   - MatterHumidity: relative humidity from SCD40
   - MatterAirQuality: CO2 level mapped to GOOD/FAIR/MODERATE/POOR

   How presence detection works:
   The Grid-EYE reads 64 temperature pixels. If enough pixels are
   hotter than ambient + threshold (e.g. body heat), we flag the
   room as "occupied". This works through walls, in darkness, and
   without cameras — it's purely thermal.

   How the OLED heatmap works:
   Since the display is monochrome (white/black only), we represent
   temperature intensity using different fill levels:
   - HOT: fully filled white square
   - WARM: small filled square
   - COOL: single pixel dot
   - COLD: empty (black)
*/

// --- Libraries ---
#include <Wire.h>                   // I2C bus (shared by all Qwiic sensors)
#include <Matter.h>                 // Silicon Labs Matter protocol
#include <MatterTemperature.h>      // Matter temperature endpoint
#include <MatterOccupancy.h>        // Matter occupancy endpoint
#include <MatterHumidity.h>         // Matter humidity endpoint
#include <MatterAirQuality.h>       // Matter air quality endpoint (CO2 → enum)
#include <SparkFun_Qwiic_OLED.h>    // SparkFun Micro OLED display
#include <SensirionI2cScd4x.h>      // Sensirion SCD40 CO2 + humidity sensor

// --- Matter endpoints ---
// Each endpoint appears as a separate entity in Home Assistant
MatterTemperature matter_temp;       // Reports max Grid-EYE pixel temp
MatterOccupancy matter_occupancy;    // Reports binary occupied/empty
MatterHumidity matter_humidity;      // Reports relative humidity %
MatterAirQuality matter_air;         // Reports air quality enum (from CO2 ppm)

// --- Hardware objects ---
QwiicMicroOLED oled;                 // 64x48 monochrome OLED
SensirionI2cScd4x scd40;            // CO2 + temp + humidity sensor
bool oledOK = false;                 // Flag: was OLED found during init?
bool scd40OK = false;                // Flag: was SCD40 found during init?

// --- Grid-EYE I2C constants ---
#define GRIDEYE_ADDR         0x69   // AD_SELECT=HIGH on SparkFun Qwiic Grid-EYE breakout
#define GRIDEYE_PIXEL_START  0x80   // First pixel register
#define GRIDEYE_PIXEL_COUNT  64     // 8x8 = 64 pixels
#define GRIDEYE_THERMISTOR   0x0E   // On-board thermistor register

// --- Presence detection ---
#define PRESENCE_THRESHOLD   1.2f   // °C above ambient = person (lowered for 2m+ detection)
#define MIN_HOT_PIXELS       1      // At least 1 hot pixel = occupied (lowered for 2m+ detection)

// --- Grid-EYE sensor data ---
float pixels[GRIDEYE_PIXEL_COUNT];   // 64 temperature readings (one per pixel)
float maxTemp, minTemp, avgTemp;     // Statistics from current frame
float ambientTemp;                   // On-board thermistor (room baseline)
bool personDetected = false;         // True if hot blob detected
int hotPixelCount = 0;               // How many pixels exceed threshold

// --- SCD40 sensor data ---
uint16_t co2ppm = 0;                // CO2 concentration in parts per million
float scdTemperature = 0.0f;        // Temperature from SCD40 (secondary reading)
float scdHumidity = 0.0f;           // Relative humidity percentage

// --- OLED heatmap layout ---
// SparkFun Micro OLED: 64x48 pixels, monochrome (white on black)
// Layout: 8x8 heatmap grid (5px per cell = 40x40) + 8px status bar at bottom
// Heatmap is centered: x offset = (64 - 40) / 2 = 12
#define OLED_GRID_X      12    // Horizontal offset to center 40px grid in 64px display
#define OLED_GRID_Y      0     // Start from top
#define OLED_CELL_SIZE   5     // Each Grid-EYE pixel = 5x5 OLED pixels
#define OLED_STATUS_Y    41    // Status text row (below 40px grid + 1px gap)

// Temperature range for heatmap visualization
// Below TEMP_COLD = empty, above TEMP_HOT = fully filled
#define TEMP_COLD        20.0f
#define TEMP_HOT         35.0f

// --- Timing ---
unsigned long lastRead = 0;
unsigned long lastGridPrint = 0;
unsigned long lastDisplayUpdate = 0;

// ============================================================
// Grid-EYE I2C functions
// ============================================================

bool gridEyeInit() {
  // Check if Grid-EYE responds on I2C bus.
  // NOTE: SiLabs Wire library bug — address-only endTransmission() always returns 0.
  // We MUST write data for endTransmission() to return a correct ACK/NACK.
  // Writing 0x00 to register 0x00 = set normal mode (harmless if device exists).
  Wire.beginTransmission(GRIDEYE_ADDR);
  Wire.write(0x00);  // register address: power control
  Wire.write(0x00);  // value: normal mode
  if (Wire.endTransmission() != 0) return false;  // NACK = no device here

  // Set frame rate to 10fps (register 0x02)
  // 0x00 = 10fps, 0x01 = 1fps
  Wire.beginTransmission(GRIDEYE_ADDR);
  Wire.write(0x02);
  Wire.write(0x00);
  Wire.endTransmission();

  return true;
}

float gridEyeReadThermistor() {
  Wire.beginTransmission(GRIDEYE_ADDR);
  Wire.write(GRIDEYE_THERMISTOR);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)GRIDEYE_ADDR, (uint8_t)2);
  uint8_t low = Wire.read();
  uint8_t high = Wire.read();
  uint16_t raw = ((uint16_t)high << 8) | low;
  raw &= 0x0FFF;
  if (raw & 0x0800) {
    return -((float)((~raw & 0x07FF) + 1)) * 0.0625f;
  }
  return (float)raw * 0.0625f;
}

void gridEyeReadPixels() {
  maxTemp = -999.0f;
  minTemp = 999.0f;
  float sum = 0.0f;

  for (uint8_t i = 0; i < GRIDEYE_PIXEL_COUNT; i++) {
    uint8_t reg = GRIDEYE_PIXEL_START + (i * 2);
    Wire.beginTransmission(GRIDEYE_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)GRIDEYE_ADDR, (uint8_t)2);
    uint8_t low = Wire.read();
    uint8_t high = Wire.read();
    uint16_t raw = ((uint16_t)high << 8) | low;
    raw &= 0x0FFF;

    float temp;
    if (raw & 0x0800) {
      temp = -((float)((~raw & 0x07FF) + 1)) * 0.25f;
    } else {
      temp = (float)raw * 0.25f;
    }

    pixels[i] = temp;
    sum += temp;
    if (temp > maxTemp) maxTemp = temp;
    if (temp < minTemp) minTemp = temp;
  }

  avgTemp = sum / GRIDEYE_PIXEL_COUNT;
  ambientTemp = gridEyeReadThermistor();
}

// ============================================================
// Presence detection
// Compares each pixel against ambient + threshold.
// If enough pixels are "hot" (e.g. a person's body heat),
// we flag the room as occupied.
// ============================================================

void detectPresence() {
  // Threshold = ambient room temp + offset (e.g. 23°C + 2.5°C = 25.5°C)
  // Anything above this is likely a warm body, not background
  float threshold = ambientTemp + PRESENCE_THRESHOLD;
  hotPixelCount = 0;

  for (uint8_t i = 0; i < GRIDEYE_PIXEL_COUNT; i++) {
    if (pixels[i] >= threshold) {
      hotPixelCount++;
    }
  }

  // Require at least MIN_HOT_PIXELS to avoid false positives
  // (a single hot pixel could be a lamp or sunlight reflection)
  personDetected = (hotPixelCount >= MIN_HOT_PIXELS);
}

// ============================================================
// SCD40 CO2 Sensor
// The SCD40 measures every 5 seconds internally.
// We check if new data is ready before reading.
// ============================================================

void readSCD40() {
  if (!scd40OK) return;

  // Check if the sensor has a new measurement ready
  // (SCD40 measures internally every ~5 seconds)
  bool dataReady = false;
  int16_t err = scd40.getDataReadyStatus(dataReady);
  if (err != 0 || !dataReady) return;

  // Read CO2 (ppm), temperature (°C), and humidity (%)
  err = scd40.readMeasurement(co2ppm, scdTemperature, scdHumidity);
  if (err != 0) {
    Serial.println("SCD40 read error!");
    return;
  }
}

// ============================================================
// CO2 ppm → Matter Air Quality enum mapping
// Based on common indoor air quality guidelines:
//   < 800 ppm  → GOOD (fresh outdoor air is ~420 ppm)
//   800-1000   → FAIR (acceptable, well-ventilated room)
//   1000-1500  → MODERATE (could use some ventilation)
//   1500-2000  → POOR (stuffy, should open a window)
//   2000-5000  → VERY_POOR (unhealthy for prolonged exposure)
//   > 5000     → EXTREMELY_POOR (dangerous)
// ============================================================

MatterAirQuality::AirQuality_t co2ToAirQuality(uint16_t ppm) {
  if (ppm < 800)  return MatterAirQuality::AirQuality_t::GOOD;
  if (ppm < 1000) return MatterAirQuality::AirQuality_t::FAIR;
  if (ppm < 1500) return MatterAirQuality::AirQuality_t::MODERATE;
  if (ppm < 2000) return MatterAirQuality::AirQuality_t::POOR;
  if (ppm < 5000) return MatterAirQuality::AirQuality_t::VERY_POOR;
  return MatterAirQuality::AirQuality_t::EXTREMELY_POOR;
}

// ============================================================
// OLED Heatmap Display
// Draws an 8x8 thermal grid on the 64x48 Micro OLED.
// Since the OLED is monochrome (white/black only), we represent
// temperature intensity using different fill levels:
//   - HOT (>75%): fully filled 5x5 white square
//   - WARM (>50%): 3x3 filled square centered in 5x5
//   - COOL (>25%): single center pixel
//   - COLD (<25%): empty (black)
// A status bar at the bottom shows temperature + occupancy.
// ============================================================

void drawOledHeatmap() {
  if (!oledOK) return;  // Skip if OLED not found during init

  oled.erase();  // Clear the display buffer

  // Draw 8x8 heatmap grid
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      float temp = pixels[y * 8 + x];

      // Normalize temperature to 0.0 - 1.0 range
      // TEMP_COLD (20°C) = 0.0, TEMP_HOT (35°C) = 1.0
      float t = (temp - TEMP_COLD) / (TEMP_HOT - TEMP_COLD);
      if (t < 0.0f) t = 0.0f;
      if (t > 1.0f) t = 1.0f;

      // Calculate pixel position on OLED
      uint8_t px = OLED_GRID_X + (x * OLED_CELL_SIZE);
      uint8_t py = OLED_GRID_Y + (y * OLED_CELL_SIZE);

      // Draw fill level based on temperature intensity
      if (t > 0.75f) {
        // HOT: fully filled 5x5 white square
        oled.rectangleFill(px, py, OLED_CELL_SIZE, OLED_CELL_SIZE);
      } else if (t > 0.50f) {
        // WARM: 3x3 filled square centered in 5x5 cell
        oled.rectangleFill(px + 1, py + 1, 3, 3);
      } else if (t > 0.25f) {
        // COOL: single center pixel
        oled.pixel(px + 2, py + 2);
      }
      // COLD: leave empty (black) — nothing to draw
    }
  }

  // Status bar at bottom (y = 41)
  // Shows temperature, CO2 level, and occupancy indicator
  // Format: "25C 450p OCC" or "25C 450p    "
  char statusLine[20];
  if (scd40OK && co2ppm > 0) {
    snprintf(statusLine, sizeof(statusLine), "%.0fC %dp %s",
      maxTemp, co2ppm, personDetected ? "OCC" : "");
  } else {
    snprintf(statusLine, sizeof(statusLine), "%.0fC %s",
      maxTemp, personDetected ? "OCC" : "");
  }
  oled.text(0, OLED_STATUS_Y, statusLine);

  // Push buffer to the physical display
  oled.display();
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=== Matter Thermal Sensor — SparkFun Thing Plus Matter ===");
  Serial.println("Grid-EYE + SCD40 + OLED → Matter over Thread");
  Serial.println();

  // Initialize I2C bus (shared by all Qwiic sensors)
  Wire.begin();

  // Initialize Grid-EYE thermal camera (I2C address 0x69, AD_SELECT=HIGH)
  Serial.print("Grid-EYE (0x69)... ");
  if (gridEyeInit()) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED - check Qwiic cable!");
  }

  // Initialize SparkFun Micro OLED (64x48, I2C address 0x3D)
  // The OLED is optional — firmware works fine without it
  Serial.print("Micro OLED (0x3D)... ");
  oledOK = oled.begin(Wire, 0x3D);
  if (oledOK) {
    Serial.println("OK");
    oled.erase();
    oled.text(0, 0, "Matter");
    oled.text(0, 12, "Thermal");
    oled.text(0, 24, "Sensor");
    oled.display();
  } else {
    Serial.println("not found (optional)");
  }

  // Initialize SCD40 CO2 sensor (I2C address 0x62)
  // The SCD40 is optional — firmware works without it
  Serial.print("SCD40 CO2 (0x62)... ");
  scd40.begin(Wire, SCD40_I2C_ADDR_62);
  // Stop any previous measurement, then restart fresh
  scd40.stopPeriodicMeasurement();
  delay(500);
  int16_t scdErr = scd40.startPeriodicMeasurement();
  if (scdErr == 0) {
    scd40OK = true;
    Serial.println("OK (first reading in ~5s)");
  } else {
    Serial.println("not found (optional)");
  }

  // Initialize Matter protocol and all endpoints
  // Each begin() registers an endpoint that Home Assistant will discover
  Matter.begin();
  matter_temp.begin();
  matter_occupancy.begin();
  matter_humidity.begin();
  matter_air.begin();

  // Show pairing info if not yet commissioned
  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Device is NOT commissioned yet.");
    Serial.println("Use this code to pair with Home Assistant:");
    Serial.print("  Pairing code: ");
    Serial.println(Matter.getManualPairingCode());
    Serial.print("  QR URL: ");
    Serial.println(Matter.getOnboardingQRCodeUrl());
  }

  // Wait for commissioning
  Serial.println("Waiting for commissioning...");
  while (!Matter.isDeviceCommissioned()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(500);
  }
  Serial.println("Commissioned!");

  // Wait for Thread network
  Serial.println("Connecting to Thread...");
  while (!Matter.isDeviceThreadConnected()) {
    delay(200);
  }
  Serial.println("Thread connected!");

  // Wait for Matter discovery (all endpoints must be online)
  while (!matter_temp.is_online() || !matter_occupancy.is_online() ||
         !matter_humidity.is_online() || !matter_air.is_online()) {
    delay(200);
  }
  Serial.println("Matter device ONLINE!");
  Serial.println();
}

// ============================================================
// Loop
// ============================================================

void loop() {
  unsigned long now = millis();

  // --- Sensor reading at 4Hz (250ms) ---
  // Math: person walks ~1.4m/s, crosses 2m/60° FOV in ~1.5s.
  // At 1Hz we risk missing short crossings. 4Hz catches entry within 250ms.
  // Matter only reports on state CHANGE so Thread traffic stays low.
  if ((now - lastRead) >= 250) {
    lastRead = now;

    // Read all 64 thermal pixels from Grid-EYE
    gridEyeReadPixels();

    // Run presence detection algorithm
    detectPresence();

    // Read SCD40 CO2 + humidity (if available)
    readSCD40();

    // Send all data to Matter endpoints (visible in Home Assistant)
    matter_temp.set_measured_value_celsius(maxTemp);
    matter_occupancy.set_occupancy(personDetected);
    if (scd40OK && co2ppm > 0) {
      matter_humidity.set_measured_value(scdHumidity);
      matter_air.set_air_quality(co2ToAirQuality(co2ppm));
    }

    // Serial output for debugging (visible in Arduino Serial Monitor)
    if (scd40OK && co2ppm > 0) {
      Serial.printf("%s | hot:%d | max:%.1f°C | CO2:%dppm | RH:%.0f%% | amb:%.1f°C\n",
        personDetected ? "OCCUPIED" : "empty   ",
        hotPixelCount, maxTemp, co2ppm, scdHumidity, ambientTemp);
    } else {
      Serial.printf("%s | hot:%d | max:%.1f°C | ambient:%.1f°C\n",
        personDetected ? "OCCUPIED" : "empty   ",
        hotPixelCount, maxTemp, ambientTemp);
    }

    // Print full 8x8 grid every 10 seconds (useful for debugging sensor)
    if ((now - lastGridPrint) >= 10000) {
      lastGridPrint = now;
      Serial.println("--- 8x8 Thermal Grid ---");
      for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
          Serial.printf("%5.1f ", pixels[y * 8 + x]);
        }
        Serial.println();
      }
      Serial.println();
    }
  }

  // --- OLED heatmap update at 5Hz ---
  // Faster refresh than Matter updates so the visual display feels smooth.
  // The OLED draws from the same pixel[] data as the last sensor read.
  if ((now - lastDisplayUpdate) >= 200) {
    lastDisplayUpdate = now;
    drawOledHeatmap();
  }
}
