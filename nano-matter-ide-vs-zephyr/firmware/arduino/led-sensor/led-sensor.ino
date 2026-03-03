/*
 * Arduino IDE: LED + Grid-EYE Demo (The "Before")
 *
 * Demonstrates the fundamental problem with Arduino's single-threaded loop():
 * - LED toggle (on/off blink) runs in loop()
 * - Grid-EYE I2C reads also happen in loop()
 * - 3 sensor reads × ~13ms = ~39ms blocked per iteration
 * - LED only toggles at ~12Hz → visible blinking like a turn signal
 *
 * Compare with the Zephyr version:
 * - LED thread does smooth PWM breathing at 100Hz (buttery smooth)
 * - Sensor thread does the same 3 reads independently
 * - Zero interference — the kernel scheduler handles concurrency
 *
 * This is NOT a bug — it's a design limitation of the Arduino model.
 * The loop() function can only do one thing at a time.
 *
 * Hardware: Arduino Nano Matter + Carrier + Grid-EYE AMG8833 (Qwiic)
 * Grid-EYE I2C address: 0x69
 */

#include <Wire.h>
#include <Adafruit_AMG88xx.h>

/* Grid-EYE sensor instance */
Adafruit_AMG88xx amg;

/* 64-pixel temperature array (8×8 grid) */
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

/*
 * RGB LED pins on Arduino Nano Matter
 * LEDR (pin 22) = Red   = PC1 on MGM240S
 * LEDG (pin 23) = Green = PC2
 * LEDB (pin 24) = Blue  = PC3
 * All are active LOW — digitalWrite(LEDB, LOW) = ON, HIGH = OFF
 */

/* LED toggle state (on/off blink pattern) */
bool ledState = false;

/* Timing statistics */
unsigned long loopCount = 0;
float maxTemp = 0.0;

void setup() {
  Serial.begin(115200);

  /* Wait for USB serial to connect (important for SiLabs boards) */
  delay(2000);

  /* Configure LED pins as digital output (NOT PWM — simple on/off) */
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  /* Start with all LEDs off (active LOW: HIGH = off) */
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  Serial.println();
  Serial.println("=============================================");
  Serial.println("  Arduino LED + Grid-EYE Demo (Stutter Demo)");
  Serial.println("=============================================");
  Serial.println("Watch the blue LED on this board.");
  Serial.println();
  Serial.println("TARGET:  LED toggles fast -> appears as steady glow");
  Serial.println("REALITY: 3x sensor reads block ~39ms per loop");
  Serial.println("         LED only toggles at ~12Hz -> visible blink!");
  Serial.println();
  Serial.println("Now look at the Zephyr board: smooth PWM breathing.");
  Serial.println("Same sensor. Same LED. Different architecture.");
  Serial.println();

  /* Initialize Grid-EYE at address 0x69 */
  if (!amg.begin(0x69)) {
    Serial.println("ERROR: AMG8833 not found at 0x69!");
    Serial.println("Check: Qwiic cable connected? Sensor powered?");
    /* Blink red LED to signal error */
    while (1) {
      digitalWrite(LEDR, LOW);
      delay(200);
      digitalWrite(LEDR, HIGH);
      delay(200);
    }
  }
  Serial.println("AMG8833 Grid-EYE initialized at 0x69.");
  Serial.println("Starting LED blink + sensor polling...");
  Serial.println();
}

void loop() {
  unsigned long loopStart = micros();
  loopCount++;

  /*
   * TOGGLE LED — simple on/off blink.
   *
   * In a properly threaded RTOS, the LED could run at 100Hz
   * (appears as a steady glow) or use PWM for smooth breathing.
   * But in loop(), the I2C reads below will block us for ~39ms,
   * so the LED can only toggle at ~12Hz — a visible, crude blink.
   */
  ledState = !ledState;
  digitalWrite(LEDB, ledState ? LOW : HIGH);  /* Active LOW */

  /*
   * SENSOR READS — THIS IS WHERE THE BLOCKING HAPPENS
   *
   * Each readPixels() reads 128 bytes over I2C at 100kHz → ~13ms.
   * We read 3 times to represent a realistic IoT workload:
   *   - Read #1: primary temperature measurement
   *   - Read #2: verification/averaging read
   *   - Read #3: simulates a second I2C device (display, IMU, etc.)
   *
   * In real IoT firmware, loop() often handles:
   *   - Multiple sensor reads (temperature + humidity + motion)
   *   - Display updates (OLED refresh over I2C: 10-50ms)
   *   - Communication (BLE advertising, serial logging)
   * Total blocking of 30-100ms per loop is common.
   *
   * Result: ~39ms of I2C blocking → LED toggles at ~12Hz.
   * That's a visible blink, like a turn signal.
   *
   * On Zephyr, these same reads happen on a SEPARATE thread.
   * The LED thread keeps running at 100Hz — zero interference.
   */
  unsigned long sensorStart = micros();

  for (int r = 0; r < 3; r++) {
    amg.readPixels(pixels);  /* ~13ms each — BLOCKS everything */
  }

  unsigned long sensorDuration = micros() - sensorStart;

  /* Find the hottest pixel (uses last read's data) */
  maxTemp = pixels[0];
  for (int i = 1; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    if (pixels[i] > maxTemp) {
      maxTemp = pixels[i];
    }
  }

  unsigned long loopDuration = micros() - loopStart;

  /* Print timing proof every 20 loops (~1.6 seconds at 12Hz) */
  if (loopCount % 20 == 0) {
    Serial.print("[STUTTER] Loop #");
    Serial.print(loopCount);
    Serial.print(": ");
    Serial.print(loopDuration / 1000.0, 1);
    Serial.print(" ms total (");
    Serial.print(sensorDuration / 1000.0, 1);
    Serial.print(" ms I2C) | ~");
    Serial.print(1000000.0 / loopDuration / 2, 0);
    Serial.print(" Hz blink | Max: ");
    Serial.print(maxTemp, 1);
    Serial.println(" C");
  }

  /*
   * Minimal delay (1ms) — standard Arduino practice.
   * The real bottleneck is the ~39ms of I2C reads above.
   * Total loop time: ~40ms → LED toggles at ~12Hz → visible blink.
   *
   * POWER NOTE: The SiLabs Arduino core (v3.0.0) includes ArduinoLowPower,
   * but LowPower.sleep(ms) internally just calls delay() — no EM2 entry.
   * The untimed LowPower.sleep() does enter EM2, but has no timer wakeup.
   * See ArduinoLowPower.cpp line 144: timedSleep() { delay(); }
   * In Zephyr, CONFIG_PM=y makes sleep automatic — the kernel enters EM2
   * (1.3 µA) whenever all threads are in k_sleep(). Zero code changes.
   */
  delay(1);
}
