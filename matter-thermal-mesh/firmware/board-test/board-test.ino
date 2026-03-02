/*
   Minimal board test — SparkFun Thing Plus Matter #2

   No sensors, no Matter, no libraries.
   Just Serial output + built-in LED blink.
   If this works → board is alive.
   If no serial output → board or USB issue.
*/

void setup() {
  Serial.begin(115200);
  delay(2000);  // Extra wait for USB serial to connect

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("=============================");
  Serial.println("SparkFun #2 - BOARD TEST");
  Serial.println("=============================");
  Serial.println("If you see this, the board is alive!");
  Serial.println();
}

void loop() {
  static unsigned long count = 0;

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);

  count++;
  Serial.print("Heartbeat #");
  Serial.println(count);
}
