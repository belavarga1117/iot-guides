/*
   Minimal OLED test for SparkFun Thing Plus Matter
   Step 1: Just LED + Serial (no I2C)
   Step 2: Add Wire + OLED if step 1 works
*/

#include <Wire.h>
#include <SparkFun_Qwiic_OLED.h>

QwiicMicroOLED oled;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("=== OLED TEST START ===");

  // Alive blink
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }

  Serial.println("Step 1: LED OK");

  // I2C init
  Serial.println("Step 2: Wire.begin()...");
  Wire.begin();
  delay(200);
  Serial.println("Step 2: Wire OK");

  // Probe 0x3D only (Micro OLED default)
  Serial.println("Step 3: Probe 0x3D...");
  Wire.beginTransmission(0x3D);
  uint8_t err3D = Wire.endTransmission();
  Serial.printf("  0x3D = %d\n", err3D);

  Serial.println("Step 4: Probe 0x3C...");
  Wire.beginTransmission(0x3C);
  uint8_t err3C = Wire.endTransmission();
  Serial.printf("  0x3C = %d\n", err3C);

  // Probe Grid-EYE
  Serial.println("Step 5: Probe 0x68 (Grid-EYE)...");
  Wire.beginTransmission(0x68);
  uint8_t err68 = Wire.endTransmission();
  Serial.printf("  0x68 = %d\n", err68);

  // Probe SCD40
  Serial.println("Step 6: Probe 0x62 (SCD40)...");
  Wire.beginTransmission(0x62);
  uint8_t err62 = Wire.endTransmission();
  Serial.printf("  0x62 = %d\n", err62);

  // Try OLED begin
  uint8_t oledAddr = (err3D == 0) ? 0x3D : ((err3C == 0) ? 0x3C : 0);
  if (oledAddr) {
    Serial.printf("Step 7: oled.begin(0x%02X)...\n", oledAddr);
    bool ok = oled.begin(Wire, oledAddr);
    Serial.printf("  begin() = %s\n", ok ? "OK" : "FAIL");

    if (ok) {
      Serial.println("Step 8: Drawing white screen...");
      oled.erase();
      oled.rectangleFill(0, 0, oled.getWidth(), oled.getHeight());
      oled.display();
      delay(2000);

      Serial.println("Step 9: Drawing text...");
      oled.erase();
      oled.text(0, 0, "HELLO");
      oled.text(0, 16, "WORLD");
      oled.display();
    }
  } else {
    Serial.println("NO OLED FOUND!");
  }

  Serial.println("=== SETUP DONE ===");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  Serial.println("tick");
}
