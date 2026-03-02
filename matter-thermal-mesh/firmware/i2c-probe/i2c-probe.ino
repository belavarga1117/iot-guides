/*
 * I2C probe v7 — VL53L4CD deep diagnostic for Nano Matter
 *
 * The Modulino Distance board (VL53L4CD) WORKS with the Zephyr driver
 * on Nano Matter, but NACK at 0x29 with Arduino Wire. This sketch tries
 * multiple I2C access methods to find what's different.
 *
 * Zephyr uses i2c_write_read_dt() = combined write+read in one transaction
 * Arduino Wire uses separate beginTransmission/endTransmission + requestFrom
 * This might make a difference on SiLabs I2C hardware.
 */

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(5000);

  Serial.println("=== VL53L4CD Deep Diagnostic — Nano Matter ===");
  Serial.println("Hardware known-good (Zephyr driver worked)");
  Serial.println();

  // ====== TEST 1: Wire (default I2C) ======
  Serial.println("========== Wire (default I2C) ==========");
  Wire.begin();

  // 1a: Address-only probe (SiLabs bug: always returns 0)
  Serial.print("1a. Address-only probe 0x29: ");
  Wire.beginTransmission(0x29);
  uint8_t err = Wire.endTransmission();
  Serial.printf("err=%d %s\n", err, err == 0 ? "(ACK or SiLabs bug)" : "(NACK)");

  // 1b: Data-write probe (our standard method)
  Serial.print("1b. Data-write probe 0x29: ");
  Wire.beginTransmission(0x29);
  Wire.write(0x00);
  Wire.write(0x00);
  err = Wire.endTransmission();
  Serial.printf("err=%d %s\n", err, err == 0 ? "ACK" : "NACK");

  // 1c: Pure read (no write first)
  Serial.print("1c. Pure read from 0x29 (2 bytes): ");
  uint8_t cnt = Wire.requestFrom((uint8_t)0x29, (uint8_t)2);
  Serial.printf("%d bytes: ", cnt);
  for (int i = 0; i < cnt; i++) Serial.printf("0x%02X ", Wire.read());
  if (cnt == 0) Serial.print("(none)");
  Serial.println();

  // 1d: Write register + repeated start + read (Zephyr-style)
  Serial.print("1d. Write-read with repeated start (model ID 0x010F): ");
  Wire.beginTransmission(0x29);
  Wire.write(0x01);
  Wire.write(0x0F);
  err = Wire.endTransmission(false);  // repeated start, NOT stop
  Serial.printf("endTx(false)=%d, ", err);
  cnt = Wire.requestFrom((uint8_t)0x29, (uint8_t)2);
  if (cnt >= 2) {
    uint16_t id = ((uint16_t)Wire.read() << 8) | Wire.read();
    Serial.printf("model=0x%04X %s\n", id,
      id == 0xEBAA ? "VL53L4CD!" : id == 0xF403 ? "VL53L4ED!" : "");
  } else {
    Serial.printf("%d bytes (fail)\n", cnt);
  }

  // 1e: Write register + stop + read (separate transactions)
  Serial.print("1e. Write-read with stop (model ID 0x010F): ");
  Wire.beginTransmission(0x29);
  Wire.write(0x01);
  Wire.write(0x0F);
  err = Wire.endTransmission(true);  // full stop
  Serial.printf("endTx(true)=%d, ", err);
  if (err == 0) {
    cnt = Wire.requestFrom((uint8_t)0x29, (uint8_t)2);
    if (cnt >= 2) {
      uint16_t id = ((uint16_t)Wire.read() << 8) | Wire.read();
      Serial.printf("model=0x%04X\n", id);
    } else {
      Serial.printf("%d bytes\n", cnt);
    }
  } else {
    Serial.println("skipped (NACK)");
  }

  // 1f: Try firmware status register (what Zephyr's SensorInit reads first)
  Serial.print("1f. Firmware status (0x00E5): ");
  Wire.beginTransmission(0x29);
  Wire.write(0x00);
  Wire.write(0xE5);
  err = Wire.endTransmission(false);
  Serial.printf("endTx(false)=%d, ", err);
  cnt = Wire.requestFrom((uint8_t)0x29, (uint8_t)1);
  if (cnt >= 1) {
    uint8_t status = Wire.read();
    Serial.printf("status=0x%02X %s\n", status,
      status == 0x03 ? "BOOTED" : "NOT READY");
  } else {
    Serial.printf("%d bytes\n", cnt);
  }

  // 1g: Try VL53L4CD at 8-bit address convention
  // VL53L4CD datasheet uses 0x52 (8-bit write). Arduino Wire uses 7-bit = 0x29.
  // But just in case, try a few nearby addresses
  Serial.println("1g. Nearby address scan:");
  uint8_t tryAddrs[] = {0x28, 0x29, 0x2A, 0x52, 0x53};
  for (int i = 0; i < 5; i++) {
    Wire.beginTransmission(tryAddrs[i]);
    Wire.write(0x01);
    Wire.write(0x0F);
    err = Wire.endTransmission();
    Serial.printf("    0x%02X: %s\n", tryAddrs[i], err == 0 ? "ACK" : "NACK");
  }

  // ====== TEST 1h: Full scan on Wire ======
  Serial.println("1h. Full scan on Wire (data-write):");
  int found0 = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    Wire.write(0x00);
    Wire.write(0x00);
    if (Wire.endTransmission() == 0) {
      found0++;
      Serial.printf("    0x%02X", addr);
      if (addr == 0x29) Serial.print(" (VL53L4CD)");
      if (addr == 0x30) Serial.print(" (IS31FL3741)");
      if (addr == 0x36) Serial.print(" (MAX17048)");
      if (addr == 0x3D) Serial.print(" (OLED)");
      if (addr == 0x53) Serial.print(" (Modulino Light/LTR381RGB)");
      if (addr == 0x62) Serial.print(" (SCD40)");
      if (addr == 0x69) Serial.print(" (Grid-EYE)");
      Serial.println();
    }
  }
  Serial.printf("    Wire total: %d devices\n\n", found0);

  // ====== TEST 2: Wire1 (second I2C) — only on boards with 2 I2C ======
#if NUM_HW_I2C > 1
  Serial.println("\n========== Wire1 (second I2C) ==========");
  Wire1.begin();
  Serial.print("2b. Data-write probe 0x29: ");
  Wire1.beginTransmission(0x29);
  Wire1.write(0x00);
  Wire1.write(0x00);
  err = Wire1.endTransmission();
  Serial.printf("err=%d %s\n", err, err == 0 ? "ACK" : "NACK");
#else
  Serial.println("\n========== Wire1: not available (single I2C) ==========");
#endif

  // ====== TEST 3: Grid-EYE sanity check ======
  Serial.println("\n========== Grid-EYE sanity check ==========");
  // Confirm Grid-EYE works on Wire
  Wire.beginTransmission(0x69);
  Wire.write(0x0E);
  Wire.endTransmission(false);
  cnt = Wire.requestFrom((uint8_t)0x69, (uint8_t)2);
  if (cnt >= 2) {
    uint8_t low = Wire.read();
    uint8_t high = Wire.read();
    uint16_t raw = ((uint16_t)high << 8) | low;
    raw &= 0x0FFF;
    Serial.printf("Grid-EYE on Wire: %.1f C (OK)\n", (float)raw * 0.0625f);
  } else {
    Serial.println("Grid-EYE on Wire: FAIL");
  }

#if NUM_HW_I2C > 1
  // Try Grid-EYE on Wire1 too
  Wire1.beginTransmission(0x69);
  Wire1.write(0x0E);
  Wire1.endTransmission(false);
  cnt = Wire1.requestFrom((uint8_t)0x69, (uint8_t)2);
  if (cnt >= 2) {
    uint8_t low = Wire1.read();
    uint8_t high = Wire1.read();
    uint16_t raw = ((uint16_t)high << 8) | low;
    raw &= 0x0FFF;
    Serial.printf("Grid-EYE on Wire1: %.1f C (OK)\n", (float)raw * 0.0625f);
  } else {
    Serial.println("Grid-EYE on Wire1: not found");
  }
#endif

  Serial.println("\n=== Done ===");
}

void loop() {
  delay(30000);
}
