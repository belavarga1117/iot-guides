/*
   VL53L4CD test using STM32duino library + Matter
   Tests if this library works alongside Matter stack
   Also reads distance to confirm sensor is alive
*/

#include <Wire.h>
#include <Matter.h>
#include <MatterTemperature.h>
#include <vl53l4cd_class.h>

MatterTemperature matter_temp;

// VL53L4CD via STM32duino library
// Default I2C address for VL53L4CD = 0x29 (7-bit)
VL53L4CD sensor(&Wire, -1);  // -1 = no XSHUT pin

void setup() {
  Serial.begin(115200);
  delay(5000);

  Serial.println("=== VL53L4CD + Matter Test ===");
  Wire.begin();

  // Init VL53L4CD with STM32duino library
  Serial.print("VL53L4CD init... ");
  VL53L4CD_ERROR err = sensor.InitSensor();
  if (err == VL53L4CD_ERROR_NONE) {
    Serial.println("OK at default address (0x29)!");

    uint16_t id;
    sensor.VL53L4CD_GetSensorId(&id);
    Serial.printf("Sensor ID: 0x%04X\n", id);

    sensor.VL53L4CD_SetRangeTiming(50, 0);
    sensor.VL53L4CD_StartRanging();
  } else {
    Serial.printf("FAILED (error=%d)\n", err);

    // Try alternate address 0x69
    Serial.print("Trying 0x69... ");
    VL53L4CD_ERROR err2 = sensor.InitSensor(0xD2);  // 0x69 << 1
    if (err2 == VL53L4CD_ERROR_NONE) {
      Serial.println("OK at 0x69!");
      sensor.VL53L4CD_SetRangeTiming(50, 0);
      sensor.VL53L4CD_StartRanging();
    } else {
      Serial.printf("FAILED (error=%d)\n", err2);
    }
  }

  // Init Matter (test coexistence)
  Matter.begin();
  matter_temp.begin();
  Serial.println("Matter init OK — no crash!");

  Serial.println();
}

void loop() {
  static unsigned long lastRead = 0;
  if (millis() - lastRead < 1000) return;
  lastRead = millis();

  VL53L4CD_Result_t results;
  uint8_t ready = 0;

  sensor.VL53L4CD_CheckForDataReady(&ready);
  if (ready) {
    sensor.VL53L4CD_GetResult(&results);
    sensor.VL53L4CD_ClearInterrupt();
    Serial.printf("Distance: %d mm | Status: %d | Signal: %d kcps\n",
      results.distance_mm, results.range_status, results.signal_per_spad_kcps);
  } else {
    Serial.println("waiting for measurement...");
  }
}
