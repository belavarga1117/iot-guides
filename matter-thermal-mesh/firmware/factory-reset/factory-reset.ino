/*
   Matter Factory Reset — instant decommission on boot

   Upload this sketch to any commissioned Matter board to clear
   its fabric credentials. The board will reboot into commissioning
   mode (BLE advertising). Then re-upload the production firmware.

   Compatible boards:
   - SparkFun Thing Plus MGM240P
   - Arduino Nano Matter
   - xG24 Explorer Kit / Dev Kit
   - XIAO MG24
*/
#include <Matter.h>

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Matter Factory Reset ===");

  Matter.begin();

  if (Matter.isDeviceCommissioned()) {
    Serial.println("Device is commissioned — decommissioning now...");
    // This clears all fabric credentials and reboots the device
    Matter.decommission();
    // decommission() does not return — device reboots
  } else {
    Serial.println("Device is NOT commissioned — nothing to do.");
    Serial.println("You can now upload the production firmware.");
  }
}

void loop()
{
  // Blink LED to indicate "ready for re-flash"
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ACTIVE);
  delay(500);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_INACTIVE);
  delay(500);
}
