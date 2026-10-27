#include <WiFi.h>
#include <MQTT.h>   // library "MQTT" by Joel Gaehwiler (arduino-mqtt)

const char* WIFI_SSID = "WIFI Username";
const char* WIFI_PASS = "YOUR_Password";
const char* BROKER_IP = "10.248.26.27";
const char* TOPIC     = "gateway/p4/current";

#define RO_PIN    26
#define DI_PIN    27
#define DE_RE_PIN 4

WiFiClient net;
MQTTClient mqtt;
uint32_t seq = 0;

const uint8_t REQ[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};

uint16_t crc16(const uint8_t* d, size_t n) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < n; i++) {
    crc ^= d[i];
    for (int b = 0; b < 8; b++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  return crc;
}

// 0 = OK, 1 = TIMEOUT, 2 = BAD FRAME
int pollOnce(int16_t &mA) {
  while (Serial2.available()) Serial2.read();
  digitalWrite(DE_RE_PIN, HIGH);
  Serial2.write(REQ, 8);
  Serial2.flush();                 // waits for last stop bit
  digitalWrite(DE_RE_PIN, LOW);

  uint8_t buf[7];
  int n = 0;
  uint32_t t0 = millis();
  while (n < 7 && millis() - t0 < 30) {
    if (Serial2.available()) buf[n++] = Serial2.read();
  }
  if (n == 0) return 1;
  if (n < 7) return 2;
  if (buf[0] != 1 || buf[1] != 3 || buf[2] != 2) return 2;
  uint16_t crc = crc16(buf, 5);
  if (buf[5] != (crc & 0xFF) || buf[6] != (crc >> 8)) return 2;
  mA = (int16_t)((buf[3] << 8) | buf[4]);
  return 0;
}

void connectAll() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.print("WiFi OK, IP ");
    Serial.println(WiFi.localIP());
  }
  if (!mqtt.connected()) {
    Serial.println("MQTT connecting...");
    while (!mqtt.connect("p4-gateway")) delay(500);
    Serial.println("MQTT OK");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  Serial2.begin(9600, SERIAL_8N1, RO_PIN, DI_PIN);
  WiFi.mode(WIFI_STA);
  mqtt.begin(BROKER_IP, net);
  connectAll();
}

void loop() {
  connectAll();
  mqtt.loop();

  int16_t mA;
  int r = pollOnce(mA);
  if (r == 0) {
    seq++;
    String msg = "seq=" + String(seq) + " current=" + String(mA) + " mA";
    Serial.println(msg);
    mqtt.publish(TOPIC, msg, false, 1);   // retained=false, QoS 1
  } else if (r == 1) {
    Serial.println("TIMEOUT");
  } else {
    Serial.println("BAD FRAME");
  }
  delay(1000);
}