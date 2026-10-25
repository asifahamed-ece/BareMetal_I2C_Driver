#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  // Use GPIO16 for RX and GPIO17 for TX.
  // Do NOT call pinMode() on 16 afterwards: on core 3.x the peripheral
  // manager treats it as a bus change and detaches UART2 from the pin.
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  Serial.println("ESP32 Bridge Ready!"); // This should appear on boot
}

void loop() {
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  delay(1);
}