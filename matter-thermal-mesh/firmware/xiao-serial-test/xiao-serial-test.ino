// Minimal serial test for XIAO MG24
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("=== XIAO SERIAL TEST ===");
}

void loop() {
  static int count = 0;
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  Serial.printf("tick %d\n", count++);
  delay(1000);
}
