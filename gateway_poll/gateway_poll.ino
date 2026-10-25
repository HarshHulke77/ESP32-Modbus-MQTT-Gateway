#include <driver/uart.h>

#define RX_PIN 26
#define TX_PIN 27
#define DE_RE  4
#define SLAVE_ID   0x01
#define REPLY_LEN  7

const uint8_t request[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};
uint8_t buf[16];

uint16_t crc16(const uint8_t *data, int len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  return crc;
}

bool replyOk(const uint8_t *r, int n) {
  if (n != REPLY_LEN) return false;            // check 1: length
  if (r[0] != SLAVE_ID) return false;          // check 2: slave ID
  uint16_t crc = crc16(r, n - 2);              // check 3: CRC over first 5 bytes
  return r[n - 2] == (crc & 0xFF) && r[n - 1] == (crc >> 8);
}

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  while (Serial2.available()) Serial2.read();   // flush stale bytes

  digitalWrite(DE_RE, HIGH);
  Serial2.write(request, 8);
  uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(100));
  digitalWrite(DE_RE, LOW);

  int n = 0;
  unsigned long start = millis();
  while (millis() - start < 100 && n < 16) {
    if (Serial2.available()) buf[n++] = Serial2.read();
  }

  if (n == 0) {
    Serial.println("TIMEOUT");
  } else if (replyOk(buf, n)) {
    int16_t mA = (int16_t)((buf[3] << 8) | buf[4]);
    Serial.printf("OK  current = %d mA\n", mA);
  } else {
    Serial.printf("BAD FRAME (%d bytes): ", n);
    for (int i = 0; i < n; i++) Serial.printf("%02X ", buf[i]);
    Serial.println();
  }
  delay(1000);
}