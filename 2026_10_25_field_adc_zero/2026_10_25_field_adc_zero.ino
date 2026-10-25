#include <driver/uart.h>

#define RX_PIN 26
#define TX_PIN 27
#define DE_RE  4

const uint8_t request[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};
uint8_t buf[16];

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);                       // receive by default
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  while (Serial2.available()) Serial2.read();     // flush stale bytes

  digitalWrite(DE_RE, HIGH);                      // transmit
  Serial2.write(request, 8);
  uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(100));
  digitalWrite(DE_RE, LOW);                       // back to receive

  int n = 0;
  unsigned long start = millis();
  while (millis() - start < 100 && n < 16) {      // 100 ms reply timeout
    if (Serial2.available()) buf[n++] = Serial2.read();
  }

  if (n == 0) {
    Serial.println("TIMEOUT");
  } else {
    Serial.printf("got bytes: %d  ->  ", n);
    for (int i = 0; i < n; i++) Serial.printf("%02X ", buf[i]);
    Serial.println();
  }
  delay(1000);
}