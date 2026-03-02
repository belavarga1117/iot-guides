/*
   Test: Does Modulino.h work with Matter.h?
   If this boots and prints serial output, there's no enum conflict.
   If no serial output -> the conflict is real.
*/

#include <Wire.h>
#include <Matter.h>
#include <MatterTemperature.h>
#include <Modulino.h>

MatterTemperature matter_temp;
ModulinoDistance distanceSensor;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=== Modulino + Matter Test ===");
  Serial.println("If you see this, both libraries coexist!");

  Wire.begin();
  Modulino.begin();

  Serial.print("Distance sensor... ");
  if (distanceSensor.begin()) {
    Serial.println("OK");
  } else {
    Serial.println("not found");
  }

  Matter.begin();
  matter_temp.begin();

  Serial.println("Setup complete - no crash!");
}

void loop() {
  static unsigned long count = 0;
  delay(1000);
  count++;
  Serial.print("Alive #");
  Serial.println(count);
}
