#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  // Use GPIO16 for RX and GPIO17 for TX
  Serial2.begin(115200, SERIAL_8N1, 16, 17); 
  pinMode(16, INPUT_PULLUP);
  Serial.println("ESP32 Bridge Ready!"); // This should appear on boot
}

void loop() {
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  delay(1);
}