/*
 * Grid-EYE AMG8833 address test for SiLabs boards.
 * Tests both 0x68 and 0x69 to determine the actual address.
 * Reads thermistor + first pixel row from whichever responds.
 */

#include <Wire.h>

// Read thermistor temperature from Grid-EYE at given address
float readThermistor(uint8_t addr) {
  Wire.beginTransmission(addr);
  Wire.write(0x0E);  // thermistor register
  Wire.endTransmission(true);
  uint8_t n = Wire.requestFrom(addr, (uint8_t)2);
  if (n < 2) return -999.0f;  // no data = not here
  uint8_t lo = Wire.read();
  uint8_t hi = Wire.read();
  uint16_t raw = ((uint16_t)hi << 8) | lo;
  raw &= 0x0FFF;
  if (raw & 0x0800) return -((float)((~raw & 0x07FF) + 1)) * 0.0625f;
  return (float)raw * 0.0625f;
}

// Read one pixel from Grid-EYE
float readPixel(uint8_t addr, uint8_t pixelIdx) {
  uint8_t reg = 0x80 + (pixelIdx * 2);
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(true);
  uint8_t n = Wire.requestFrom(addr, (uint8_t)2);
  if (n < 2) return -999.0f;
  uint8_t lo = Wire.read();
  uint8_t hi = Wire.read();
  uint16_t raw = ((uint16_t)hi << 8) | lo;
  raw &= 0x0FFF;
  if (raw & 0x0800) return -((float)((~raw & 0x07FF) + 1)) * 0.25f;
  return (float)raw * 0.25f;
}

void testAddress(uint8_t addr) {
  Serial.printf("\n--- Grid-EYE at 0x%02X ---\n", addr);

  // Check if device ACKs with data-write trick
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)0x00);  // write to power control register
  uint8_t err = Wire.endTransmission(true);
  Serial.printf("  Data-write endTx = %d (%s)\n", err, err == 0 ? "ACK" : "NACK");

  // Check buggy address-only probe for comparison
  Wire.beginTransmission(addr);
  uint8_t buggy = Wire.endTransmission();
  Serial.printf("  Address-only endTx = %d (%s — SiLabs bug: always 0)\n",
                buggy, buggy == 0 ? "ACK" : "NACK");

  // Try reading thermistor
  float therm = readThermistor(addr);
  Serial.printf("  Thermistor: %.1f C %s\n", therm,
                (therm > 10 && therm < 45) ? "(valid!)" :
                therm == -999 ? "(no data)" : "(suspicious)");

  // Try reading first 4 pixels
  Serial.print("  Pixels 0-3: ");
  for (int i = 0; i < 4; i++) {
    float px = readPixel(addr, i);
    Serial.printf("%.1f ", px);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(5000);

  Serial.println("=== Grid-EYE Address Test ===");
  Wire.begin();

  testAddress(0x68);
  testAddress(0x69);

  Serial.println("\n=== Continuous reading from detected address ===");
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last < 2000) return;
  last = millis();

  // Read from both and print whoever responds
  float t68 = readThermistor(0x68);
  float t69 = readThermistor(0x69);

  Serial.printf("0x68: %.1f C | 0x69: %.1f C\n", t68, t69);
}
