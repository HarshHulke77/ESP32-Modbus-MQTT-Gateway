// gateway_poll.ino
// P4 gateway: polls field device over RS-485 (Modbus RTU), validates the reply,
// and stamps each good reading with a sequence number (RAM only for now).
//
// Wiring (unchanged): RO -> divider -> GPIO26, DI -> GPIO27, DE+RE -> GPIO4
// Serial monitor: PuTTY at 115200 (CP210x port)

#include <HardwareSerial.h>
#include "driver/uart.h"

#define RX_PIN         26
#define TX_PIN         27
#define DE_RE_PIN      4
#define MODBUS_BAUD    9600
#define SLAVE_ID       1
#define REPLY_LEN      7      // id, func, bytecount, data hi, data lo, crc lo, crc hi
#define TIMEOUT_MS     30     // measured: reply done ~11.6 ms after request ends
#define POLL_PERIOD_MS 1000

// Read holding register 0, 1 register, slave 1. CRC bytes: 84 0A (low byte first).
static const uint8_t REQUEST[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};

uint32_t seq = 0;             // counts good readings, resets to 0 on reboot (RAM)

// Modbus CRC-16 (init 0xFFFF, polynomial 0xA001, low byte sent first)
uint16_t crc16(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
      else              crc >>= 1;
    }
  }
  return crc;
}

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);                       // start in receive mode
  Serial2.begin(MODBUS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(500);
  Serial.println("gateway_poll ready");
}

void loop() {
  // 1. Throw away any stale bytes left from earlier
  while (Serial2.available()) Serial2.read();

  // 2. Send the request
  digitalWrite(DE_RE_PIN, HIGH);                      // transmit
  Serial2.write(REQUEST, sizeof(REQUEST));
  uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(100));  // wait for last bit to leave
  digitalWrite(DE_RE_PIN, LOW);                       // back to receive

  // 3. Collect the reply until 7 bytes arrive or the timeout expires
  uint8_t reply[16];
  size_t n = 0;
  uint32_t start = millis();
  while ((millis() - start) < TIMEOUT_MS && n < REPLY_LEN) {
    if (Serial2.available()) {
      reply[n++] = Serial2.read();
    }
  }

  // 4. Check the reply
  if (n == 0) {
    Serial.println("TIMEOUT");
  } else if (n != REPLY_LEN) {
    Serial.println("BAD FRAME");                      // wrong length
  } else if (reply[0] != SLAVE_ID) {
    Serial.println("BAD FRAME");                      // wrong slave ID
  } else {
    uint16_t calc = crc16(reply, REPLY_LEN - 2);
    uint16_t got  = (uint16_t)reply[5] | ((uint16_t)reply[6] << 8);
    if (calc != got) {
      Serial.println("BAD FRAME");                    // CRC mismatch
    } else {
      int16_t current_mA = (int16_t)(((uint16_t)reply[3] << 8) | reply[4]);
      seq++;                                          // only good readings get a number
      Serial.printf("seq=%lu current=%d mA\n", (unsigned long)seq, current_mA);
    }
  }

  delay(POLL_PERIOD_MS);
}
