constexpr uint32_t heartbeat_interval_ms = 1000;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("rgb-matrix: up");
}

void loop() {
  Serial.printf("uptime %lu ms\r\n", millis());
  delay(heartbeat_interval_ms);
}
