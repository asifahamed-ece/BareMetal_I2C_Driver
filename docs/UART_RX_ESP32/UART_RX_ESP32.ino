/**
 * @file    UART_RX_ESP32.ino
 * @brief   ESP32 bench receiver: UART2 in on GPIO16, echoed to USB serial.
 *
 * Wiring to the STM32F411:
 *     STM32 PA2 (USART2_TX) -> ESP32 GPIO16 (UART2 RX)
 *     STM32 GND             -> ESP32 GND          (always, for any UART)
 *     ESP32 GPIO17 (UART2 TX) left unconnected unless you want the reverse path
 *
 * Board: DOIT ESP32 DEVKIT V1, core esp32:esp32 3.3.12.
 */

#include <Arduino.h>

/** @brief Start USB serial (UART0) as the console and UART2 on GPIO16/17. */
void setup() {
  Serial.begin(115200);

  /*
   * Serial2.begin() does three things at once: configures the UART, routes
   * the pins through the GPIO matrix, AND registers both pins with the
   * peripheral manager as ESP32_BUS_TYPE_UART_RX / _TX.
   *
   * That registration is why nothing below may call pinMode() on 16 or 17.
   * On core 3.x, pinMode() sees the pin is not owned by ESP32_BUS_TYPE_GPIO
   * and clears the existing owner first - running the UART's detach, which
   * unroutes the pin. The sketch then runs perfectly, prints this banner,
   * and receives nothing, forever. Pull-ups are not needed here: the line is
   * driven by the STM32, and the UART input already has a buffer.
   */
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("ESP32 Bridge Ready!");
}

/**
 * @brief   Echo every byte that arrives on UART2 out to the USB console.
 *
 * @note    delay(1) is not decoration. It yields to the FreeRTOS scheduler so
 *          the idle task can run and the task watchdog is fed. A tight loop
 *          with no yield trips the watchdog and reboots the ESP32 mid-test.
 */
void loop() {
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  delay(1);
}
