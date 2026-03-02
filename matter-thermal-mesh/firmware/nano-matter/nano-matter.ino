
/*
   Matter Thermal Sensor — Full Firmware (Arduino Nano Matter)

   Board: Arduino Nano Matter (MGM240S)
   Protocol: Matter over Thread

   Sensors (all via Qwiic daisy-chain — zero soldering):
   - Grid-EYE AMG8833: 8x8 thermal camera (I2C 0x69, AD_SELECT=HIGH)
   - Modulino Light (LTR381RGB): ambient light sensor (I2C 0x53)
   - Adafruit IS31FL3741 QT: 13x9 RGB LED matrix (I2C 0x30)

   Matter endpoints (visible in Home Assistant):
   - MatterTemperature: max pixel temperature from Grid-EYE
   - MatterOccupancy: binary presence detection (thermal)

   How the ambient light sensor enhances the display:
   The Modulino Light reads ambient brightness (lux) and adjusts the
   LED matrix brightness automatically:
   - Dark room: dim display (easy on the eyes, saves power)
   - Bright room: brighter display (still visible in daylight)
   This makes the heatmap display practical in any lighting condition.

   How the RGB heatmap works:
   The 13x9 LED matrix shows a LIVE thermal heatmap from the Grid-EYE:
   - Center 8x8 area: one LED per Grid-EYE pixel
   - Color scale: Blue(cold) → Cyan → Green → Yellow → Red(hot)
   - Edge columns (left 2, right 3): pulsing red when person detected
   - Bottom row: status bar showing relative heat level

   LED matrix animations during boot:
   - Commissioning: sweeping blue wave (searching for hub)
   - Thread connecting: expanding green ring
   - Online: rainbow celebration flash

   How presence detection works:
   Same algorithm as SparkFun node — counts pixels hotter than
   ambient + threshold. If enough "hot pixels" → room is occupied.
*/

// --- Libraries ---
#include <Wire.h>                   // I2C bus (shared by all Qwiic sensors)
#include <Matter.h>                 // Silicon Labs Matter protocol
#include <MatterTemperature.h>      // Matter temperature endpoint
#include <MatterOccupancy.h>        // Matter occupancy endpoint
#include <Adafruit_IS31FL3741.h>    // Adafruit IS31FL3741 RGB LED matrix
#include <Modulino.h>              // Arduino Modulino Light (LTR381RGB)

// --- Matter endpoints ---
// Each endpoint appears as a separate entity in Home Assistant
MatterTemperature matter_temp;       // Reports max Grid-EYE pixel temp
MatterOccupancy matter_occupancy;    // Reports binary occupied/empty

// --- Hardware objects ---
// IS3741_BGR = color byte order for the Adafruit QT board (Blue-Green-Red)
// Using the buffered variant for smooth animation (double-buffered)
Adafruit_IS31FL3741_QT_buffered ledMatrix(IS3741_BGR);
ModulinoLight light;                 // Modulino Light ambient sensor (LTR381RGB)
bool lightOK = false;                // Flag: was light sensor found during init?

// ============================================================
// Configuration
// ============================================================

// Node identity — change these per board before flashing
#define NODE_NAME       "NanoMatter-1"
#define NODE_ROOM       "Living Room"

// Grid-EYE I2C register map
#define GRIDEYE_ADDR         0x69   // AD_SELECT=HIGH on SparkFun Qwiic Grid-EYE breakout
#define GRIDEYE_PIXEL_START  0x80   // First pixel register (each pixel = 2 bytes, little-endian)
#define GRIDEYE_PIXEL_COUNT  64     // 8x8 = 64 pixels total
#define GRIDEYE_THERMISTOR   0x0E   // On-board thermistor register (room temperature baseline)

// Presence detection tuning (thermal)
#define PRESENCE_THRESHOLD   1.2f   // °C above ambient = "hot" (lowered for 2m+ detection)
#define MIN_HOT_PIXELS       1      // Minimum hot pixels to flag as occupied (lowered for 2m+)
                                     // (1 pixel could be a lamp or sunlight)

// LED matrix settings — adaptive brightness via Modulino Light
// The LED matrix brightness adjusts automatically based on ambient light:
//   Dark room  → MIN brightness (easy on the eyes, saves power)
//   Bright room → MAX brightness (visible in daylight)
#define MATRIX_BRIGHTNESS_MIN  10   // Minimum LED brightness (dark room)
#define MATRIX_BRIGHTNESS_MAX  80   // Maximum LED brightness (bright room)
#define MATRIX_BRIGHTNESS_DEFAULT 40 // Fallback if light sensor not available
#define LUX_DARK             10.0f  // Below this = dark room → min brightness
#define LUX_BRIGHT          500.0f  // Above this = bright room → max brightness

// Heatmap color mapping range
#define TEMP_COLD  20.0f   // Below this = full blue (cool room)
#define TEMP_HOT   35.0f   // Above this = full red (person/warm object)

// Update rates
#define DISPLAY_UPDATE_MS    200    // 5Hz heatmap refresh (smooth animation)
#define MATTER_UPDATE_MS     250    // 4Hz sampling (math: person ~1.4m/s crosses 60°FOV in 1.5s; 250ms catches entry within 1 step)

// ============================================================
// Sensor data
// ============================================================
float pixels[GRIDEYE_PIXEL_COUNT];   // 64 temperature readings (one per pixel)
float maxTemp, minTemp, avgTemp;     // Statistics from current frame
float ambientTemp;                   // On-board thermistor (room baseline)
bool personDetected = false;         // True if hot blob detected
int hotPixelCount = 0;               // How many pixels exceed threshold

// --- Ambient light data ---
float ambientLux = 0.0f;            // Current ambient light level in lux

// --- Timing ---
unsigned long lastDisplayUpdate = 0;
unsigned long lastMatterUpdate = 0;
uint8_t pulsePhase = 0;              // Animation phase for presence pulse effect

// ============================================================
// Grid-EYE I2C functions
// The AMG8833 (Grid-EYE) communicates via I2C.
// Each pixel register is 2 bytes (little-endian, 12-bit signed).
// Temperature resolution: 0.25°C per LSB for pixels,
//                         0.0625°C per LSB for thermistor.
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

// Read the on-board thermistor (ambient room temperature)
// Resolution: 0.0625°C, range: -20°C to +80°C
float gridEyeReadThermistor() {
  Wire.beginTransmission(GRIDEYE_ADDR);
  Wire.write(GRIDEYE_THERMISTOR);
  Wire.endTransmission(false);  // Repeated start (don't release bus)
  Wire.requestFrom((uint8_t)GRIDEYE_ADDR, (uint8_t)2);
  uint8_t low = Wire.read();
  uint8_t high = Wire.read();

  // 12-bit signed value: bit 11 = sign, bits 10-0 = magnitude
  uint16_t raw = ((uint16_t)high << 8) | low;
  raw &= 0x0FFF;
  if (raw & 0x0800) {
    // Negative temperature (two's complement)
    return -((float)((~raw & 0x07FF) + 1)) * 0.0625f;
  }
  return (float)raw * 0.0625f;
}

// Read all 64 thermal pixels and compute statistics
void gridEyeReadPixels() {
  maxTemp = -999.0f;
  minTemp = 999.0f;
  float sum = 0.0f;

  for (uint8_t i = 0; i < GRIDEYE_PIXEL_COUNT; i++) {
    // Each pixel occupies 2 bytes starting at register 0x80
    uint8_t reg = GRIDEYE_PIXEL_START + (i * 2);
    Wire.beginTransmission(GRIDEYE_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)GRIDEYE_ADDR, (uint8_t)2);
    uint8_t low = Wire.read();
    uint8_t high = Wire.read();

    // 12-bit signed value, resolution 0.25°C per LSB
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
  float threshold = ambientTemp + PRESENCE_THRESHOLD;
  hotPixelCount = 0;

  for (uint8_t i = 0; i < GRIDEYE_PIXEL_COUNT; i++) {
    if (pixels[i] >= threshold) {
      hotPixelCount++;
    }
  }

  // Thermal-only presence detection: if enough pixels are hotter
  // than ambient, there's likely a person in the room.
  personDetected = (hotPixelCount >= MIN_HOT_PIXELS);
}

// ============================================================
// Ambient light reading (Modulino Light)
// Reads the LTR381RGB sensor and adjusts the LED matrix
// brightness to match the room lighting. This makes the
// heatmap display comfortable in any lighting condition.
// ============================================================

void readAmbientLight() {
  if (!lightOK) return;

  // update() fetches fresh data from the sensor over I2C.
  // Must be called before reading getLux().
  if (light.update()) {
    ambientLux = light.getLux();

    // Map ambient light to LED brightness range
    // Dark room (< LUX_DARK) → minimum brightness
    // Bright room (> LUX_BRIGHT) → maximum brightness
    // In between → linear interpolation
    float ratio = (ambientLux - LUX_DARK) / (LUX_BRIGHT - LUX_DARK);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    uint8_t brightness = MATRIX_BRIGHTNESS_MIN +
      (uint8_t)(ratio * (MATRIX_BRIGHTNESS_MAX - MATRIX_BRIGHTNESS_MIN));
    ledMatrix.setGlobalCurrent(brightness);
  }
}

// ============================================================
// Temperature → RGB565 color mapping
// Maps a temperature value to a color on the spectrum:
//   Blue(cold) → Cyan → Green → Yellow → Red(hot)
// The IS31FL3741 uses RGB565 format (5 red, 6 green, 5 blue bits).
// ============================================================

uint16_t tempToColor(float temp) {
  // Normalize to 0.0 - 1.0 range between TEMP_COLD and TEMP_HOT
  float t = (temp - TEMP_COLD) / (TEMP_HOT - TEMP_COLD);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  uint8_t r, g, b;

  if (t < 0.25f) {
    // Blue → Cyan (0.0 - 0.25): increasing green, full blue
    float f = t / 0.25f;
    r = 0;
    g = (uint8_t)(f * 255);
    b = 255;
  } else if (t < 0.5f) {
    // Cyan → Green (0.25 - 0.5): full green, decreasing blue
    float f = (t - 0.25f) / 0.25f;
    r = 0;
    g = 255;
    b = (uint8_t)((1.0f - f) * 255);
  } else if (t < 0.75f) {
    // Green → Yellow (0.5 - 0.75): increasing red, full green
    float f = (t - 0.5f) / 0.25f;
    r = (uint8_t)(f * 255);
    g = 255;
    b = 0;
  } else {
    // Yellow → Red (0.75 - 1.0): full red, decreasing green
    float f = (t - 0.75f) / 0.25f;
    r = 255;
    g = (uint8_t)((1.0f - f) * 255);
    b = 0;
  }

  return Adafruit_IS31FL3741::color565(r, g, b);
}

// ============================================================
// Draw 8x8 heatmap on 13x9 LED matrix
//
// Layout (13 columns x 9 rows):
//   Columns 0-1:  presence indicator (pulsing red when occupied)
//   Columns 2-9:  8x8 heatmap (one LED per Grid-EYE pixel)
//   Columns 10-12: presence indicator (pulsing red when occupied)
//   Row 8:         status bar (heat level meter)
// ============================================================

void drawHeatmapOnMatrix() {
  ledMatrix.fill(0);  // Clear all LEDs

  // Draw 8x8 thermal data in the center columns (2-9)
  uint8_t xOffset = 2;
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      float temp = pixels[y * 8 + x];
      uint16_t color = tempToColor(temp);
      ledMatrix.drawPixel(xOffset + x, y, color);
    }
  }

  // Edge columns: presence indicator
  // When someone is detected, the edges pulse red (breathing effect)
  if (personDetected) {
    pulsePhase += 8;  // Advance animation (wraps at 255)
    // Triangle wave: ramp up 0-127, ramp down 128-255
    uint8_t brightness = (pulsePhase < 128) ? (pulsePhase * 2) : ((255 - pulsePhase) * 2);
    uint16_t pulseColor = Adafruit_IS31FL3741::color565(brightness, 0, 0);

    for (uint8_t y = 0; y < 9; y++) {
      ledMatrix.drawPixel(0, y, pulseColor);   // Left edge
      ledMatrix.drawPixel(1, y, pulseColor);
      ledMatrix.drawPixel(10, y, pulseColor);  // Right edge
      ledMatrix.drawPixel(11, y, pulseColor);
      ledMatrix.drawPixel(12, y, pulseColor);
    }
  }

  // Bottom row (row 8): heat level status bar
  // Fills proportionally from left to right based on max temperature
  // Filled portion uses the max temperature color, rest is dim green
  float heatRatio = (maxTemp - TEMP_COLD) / (TEMP_HOT - TEMP_COLD);
  if (heatRatio < 0.0f) heatRatio = 0.0f;
  if (heatRatio > 1.0f) heatRatio = 1.0f;
  uint8_t filledCols = (uint8_t)(heatRatio * 13);

  for (uint8_t x = 0; x < 13; x++) {
    if (x < filledCols) {
      ledMatrix.drawPixel(x, 8, tempToColor(maxTemp));
    } else {
      // Dim green background for unfilled portion
      ledMatrix.drawPixel(x, 8, Adafruit_IS31FL3741::color565(0, 20, 0));
    }
  }

  ledMatrix.show();  // Push buffer to physical LEDs
}

// ============================================================
// Boot animations for the LED matrix
// These provide visual feedback during the Matter commissioning
// process, so the user knows what state the device is in.
// ============================================================

// Commissioning: sweeping blue wave — "searching for Matter hub"
void showCommissioningAnimation() {
  static uint8_t frame = 0;
  ledMatrix.fill(0);

  for (uint8_t x = 0; x < 13; x++) {
    for (uint8_t y = 0; y < 9; y++) {
      // Distance from the wave center (sweeps left to right)
      int16_t dist = abs((int16_t)x - (int16_t)((frame / 2) % 13));
      if (dist < 3) {
        uint8_t bright = (3 - dist) * 80;
        ledMatrix.drawPixel(x, y,
          Adafruit_IS31FL3741::color565(0, bright / 4, bright));
      }
    }
  }

  // Heartbeat: blinking center pixel (white)
  if ((frame / 10) % 2 == 0) {
    ledMatrix.drawPixel(6, 4, Adafruit_IS31FL3741::color565(255, 255, 255));
  }

  ledMatrix.show();
  frame++;
}

// Thread connecting: expanding green ring — "joining mesh network"
void showThreadConnectingAnimation() {
  static uint8_t frame = 0;
  ledMatrix.fill(0);

  uint8_t radius = (frame / 4) % 6;
  for (uint8_t x = 0; x < 13; x++) {
    for (uint8_t y = 0; y < 9; y++) {
      int16_t dx = (int16_t)x - 6;
      int16_t dy = (int16_t)y - 4;
      uint8_t dist = (uint8_t)sqrt(dx * dx + dy * dy);
      if (dist == radius) {
        ledMatrix.drawPixel(x, y,
          Adafruit_IS31FL3741::color565(0, 200, 0));
      }
    }
  }

  ledMatrix.show();
  frame++;
}

// Online: rainbow celebration flash — "connected and ready!"
void showOnlineAnimation() {
  ledMatrix.fill(0);

  // Each column gets a different rainbow color
  for (uint8_t x = 0; x < 13; x++) {
    float hue = (float)x / 13.0f;
    uint8_t r, g, b;
    if (hue < 0.33f) {
      r = 255; g = (uint8_t)(hue * 3 * 255); b = 0;
    } else if (hue < 0.66f) {
      r = (uint8_t)((0.66f - hue) * 3 * 255); g = 255; b = 0;
    } else {
      r = 0; g = (uint8_t)((1.0f - hue) * 3 * 255); b = (uint8_t)((hue - 0.66f) * 3 * 255);
    }
    for (uint8_t y = 0; y < 9; y++) {
      ledMatrix.drawPixel(x, y, Adafruit_IS31FL3741::color565(r, g, b));
    }
  }

  ledMatrix.show();
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Matter Thermal Sensor — " NODE_NAME " ===");
  Serial.println("Room: " NODE_ROOM);
  Serial.println("Grid-EYE + Modulino Light + IS31FL3741 RGB Matrix → Matter over Thread");
  Serial.println();

  // Initialize I2C bus (shared by all Qwiic sensors)
  Wire.begin();

  // Initialize IS31FL3741 RGB LED matrix (13x9, I2C address 0x30)
  Serial.print("IS31FL3741 LED Matrix (0x30)... ");
  if (ledMatrix.begin(IS3741_ADDR_DEFAULT)) {
    ledMatrix.setLEDscaling(0xFF);   // All LEDs at full scaling
    ledMatrix.setGlobalCurrent(MATRIX_BRIGHTNESS_DEFAULT);  // Start with default, light sensor adjusts later
    ledMatrix.enable(true);
    ledMatrix.fill(0);
    ledMatrix.show();
    Serial.println("OK");

    // Quick self-test: flash red → green → blue
    // This confirms all LED colors work and wiring is correct
    ledMatrix.fill(Adafruit_IS31FL3741::color565(255, 0, 0));
    ledMatrix.show(); delay(200);
    ledMatrix.fill(Adafruit_IS31FL3741::color565(0, 255, 0));
    ledMatrix.show(); delay(200);
    ledMatrix.fill(Adafruit_IS31FL3741::color565(0, 0, 255));
    ledMatrix.show(); delay(200);
    ledMatrix.fill(0);
    ledMatrix.show();
  } else {
    Serial.println("FAILED - check Qwiic cable!");
  }

  // Initialize Grid-EYE thermal camera (I2C address 0x69, AD_SELECT=HIGH)
  Serial.printf("Grid-EYE (0x%02X)... ", GRIDEYE_ADDR);
  if (gridEyeInit()) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED - check Qwiic cable!");
  }

  // Initialize Matter protocol FIRST (required before Modulino on SiLabs)
  Matter.begin();

  // Initialize Modulino Light ambient sensor (LTR381RGB, I2C 0x53)
  // Modulino.begin() must be called after Matter.begin() on SiLabs boards
  Modulino.begin();
  Serial.print("Modulino Light (0x53)... ");
  if (light.begin()) {
    lightOK = true;
    Serial.println("OK");
  } else {
    Serial.println("not found (optional — using default brightness)");
  }

  // Initialize remaining Matter endpoints
  matter_temp.begin();
  matter_occupancy.begin();

  Serial.println();
  Serial.println("Matter device initialized");

  // Show pairing code for commissioning via Home Assistant
  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Device is NOT commissioned yet.");
    Serial.println("Commission it to your Matter hub with:");
    Serial.printf("  Manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
    Serial.printf("  QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
  }

  // Wait for commissioning (blue wave animation on LED matrix)
  while (!Matter.isDeviceCommissioned()) {
    showCommissioningAnimation();
    delay(50);
  }
  Serial.println("Device commissioned!");

  // Wait for Thread network (green ring animation)
  Serial.println("Waiting for Thread network...");
  while (!Matter.isDeviceThreadConnected()) {
    showThreadConnectingAnimation();
    delay(50);
  }
  Serial.println("Connected to Thread network!");

  // Wait for Matter discovery
  Serial.println("Waiting for Matter device discovery...");
  while (!matter_temp.is_online() || !matter_occupancy.is_online()) {
    showThreadConnectingAnimation();
    delay(50);
  }
  Serial.println("Matter device is now online!");

  // Celebration: rainbow flash (1.5 seconds)
  showOnlineAnimation();
  delay(1500);

  Serial.println();
  Serial.println("Live heatmap running at 5Hz...");
  Serial.println();
}

// ============================================================
// Loop
// ============================================================

void loop() {
  unsigned long now = millis();

  // --- Heatmap display update at 5Hz ---
  // Fast refresh for smooth LED animation.
  // Reads fresh Grid-EYE data each frame.
  if ((now - lastDisplayUpdate) >= DISPLAY_UPDATE_MS) {
    lastDisplayUpdate = now;

    // Read all 64 thermal pixels from Grid-EYE
    gridEyeReadPixels();

    // Read ambient light and adjust LED brightness
    readAmbientLight();

    // Run thermal presence detection
    detectPresence();

    // Draw live heatmap on the RGB LED matrix
    drawHeatmapOnMatrix();
  }

  // --- Matter updates at 1Hz ---
  // No need to spam the Thread mesh with rapid updates.
  // Temperature and occupancy don't change that fast.
  if ((now - lastMatterUpdate) >= MATTER_UPDATE_MS) {
    lastMatterUpdate = now;

    // Send data to Matter endpoints (visible in Home Assistant)
    matter_temp.set_measured_value_celsius(maxTemp);
    matter_occupancy.set_occupancy(personDetected);

    // Serial output for debugging (visible in Arduino Serial Monitor)
    Serial.printf("[%s] %s | hot:%d | max:%.1f°C | ambient:%.1f°C | light:%.0f lux\n",
      NODE_NAME,
      personDetected ? "OCCUPIED" : "empty   ",
      hotPixelCount,
      maxTemp,
      ambientTemp,
      ambientLux);

    // Print full 8x8 thermal grid every 10 seconds (debugging aid)
    static uint32_t lastGridPrint = 0;
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
}
