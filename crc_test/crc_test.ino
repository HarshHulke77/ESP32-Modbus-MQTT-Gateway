// Modbus RTU CRC-16 test (P4 gateway)
// Request: 01 03 00 00 00 01  ->  expected CRC bytes on the wire: 84 0A
// CRC is sent low byte first, then high byte.

uint16_t modbus_crc(const uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

void setup() {
    Serial.begin(115200);
    uint8_t req[6] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    uint16_t crc = modbus_crc(req, 6);
    Serial.printf("%02X %02X\n", crc & 0xFF, crc >> 8);
}

void loop() {}
