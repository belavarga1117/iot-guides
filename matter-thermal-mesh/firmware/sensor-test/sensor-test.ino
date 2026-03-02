/*
   Step-by-step sensor test for SparkFun Thing Plus Matter #2
   Tests each component individually to find what crashes.
   No Matter stack — just hardware.
*/

#include <Wire.h>
#include <Adafruit_IS31FL3741.h>

// Grid-EYE
#define GRIDEYE_ADDR 0x68

// VL53L4CD (Modulino Distance)
#define VL53_ADDR 0x69

// LED Matrix
Adafruit_IS31FL3741_QT_buffered ledMatrix(IS3741_BGR);

void setup() {
  Serial.begin(115200);
  delay(8000);  // Very long wait — open Serial Monitor before this expires!

  Serial.println("=============================");
  Serial.println("SENSOR TEST — SparkFun #2");
  Serial.println("=============================");

  // Step 1: I2C bus
  Serial.print("1. I2C bus init... ");
  Wire.begin();
  Serial.println("OK");

  // Step 2: I2C scan — what's on the bus?
  Serial.println("2. I2C scan:");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("   Found device at 0x%02X", addr);
      if (addr == 0x30) Serial.print(" (IS31FL3741 LED Matrix)");
      if (addr == 0x68) Serial.print(" (Grid-EYE AMG8833)");
      if (addr == 0x69) Serial.print(" (VL53L4CD / Modulino Distance)");
      Serial.println();
    }
  }

  // Step 3: Grid-EYE
  Serial.print("3. Grid-EYE (0x68)... ");
  Wire.beginTransmission(GRIDEYE_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("FOUND");
  } else {
    Serial.println("NOT FOUND");
  }

  // Step 4: VL53L4CD
  Serial.print("4. VL53L4CD (0x69)... ");
  Wire.beginTransmission(VL53_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("FOUND");
  } else {
    Serial.println("NOT FOUND");
  }

  // Step 5: LED Matrix
  Serial.print("5. IS31FL3741 (0x30)... ");
  if (ledMatrix.begin(IS3741_ADDR_DEFAULT)) {
    ledMatrix.setLEDscaling(0xFF);
    ledMatrix.setGlobalCurrent(40);
    ledMatrix.enable(true);
    ledMatrix.fill(Adafruit_IS31FL3741::color565(0, 255, 0));
    ledMatrix.show();
    Serial.println("OK — green flash");
  } else {
    Serial.println("NOT FOUND");
  }

  Serial.println();
  Serial.println("Test complete!");
}

void loop() {
  static unsigned long count = 0;
  delay(1000);
  count++;
  Serial.print("Heartbeat #");
  Serial.println(count);
}
