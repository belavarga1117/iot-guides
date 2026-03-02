/*
 * Test: Modulino Distance + Matter on SparkFun Thing Plus Matter
 *
 * Based on the official Arduino Matter Discovery course pattern:
 * - Matter.begin() FIRST
 * - Modulino.begin() AFTER Matter commissioning
 *
 * This tests whether the init ORDER matters and whether
 * ModulinoDistance (VL53L4CD) works on SiLabs boards.
 */

#include <Matter.h>
#include <MatterTemperature.h>
#include <Modulino.h>

// Matter endpoint
MatterTemperature matter_temp_sensor;

// Modulino Distance sensor
ModulinoDistance distance;

void setup() {
  Serial.begin(115200);
  delay(5000);  // long delay to open serial monitor

  Serial.println("=== Modulino Distance + Matter Test ===");
  Serial.println("Following Arduino course init pattern...");
  Serial.println();

  // STEP 1: Matter first (as per Arduino course)
  Serial.print("1. Matter.begin()... ");
  Matter.begin();
  Serial.println("OK");

  Serial.print("2. matter_temp_sensor.begin()... ");
  matter_temp_sensor.begin();
  Serial.println("OK");

  // Show commissioning info
  if (!Matter.isDeviceCommissioned()) {
    Serial.println("   Device not commissioned yet.");
    Serial.printf("   Manual code: %s\n", Matter.getManualPairingCode().c_str());
  }

  // STEP 2: Modulino AFTER Matter (as per Arduino course)
  Serial.print("3. Modulino.begin()... ");
  Modulino.begin();
  Serial.println("OK");

  Serial.print("4. distance.begin()... ");
  if (distance.begin()) {
    Serial.println("OK — sensor found!");
  } else {
    Serial.println("FAILED — sensor not found (optional)");
  }

  Serial.println();
  Serial.println("Setup complete — no crash!");
  Serial.println("Reading distance every 1 second...");
  Serial.println();
}

void loop() {
  static unsigned long lastRead = 0;

  if (millis() - lastRead >= 1000) {
    lastRead = millis();

    if (distance) {
      // Sensor was initialized successfully
      if (distance.available()) {
        float mm = distance.get();
        Serial.printf("Distance: %.0f mm\n", mm);
      } else {
        Serial.println("Waiting for measurement...");
      }
    } else {
      // Sensor not found — just print heartbeat
      static unsigned long count = 0;
      count++;
      Serial.printf("Heartbeat #%lu (no distance sensor)\n", count);
    }
  }
}
