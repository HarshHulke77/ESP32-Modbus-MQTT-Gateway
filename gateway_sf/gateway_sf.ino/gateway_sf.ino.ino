#include <WiFi.h>
#include <MQTT.h>
#include <LittleFS.h>

const char* WIFI_SSID = "USERNAME";
const char* WIFI_PASS = "YOUR_PASSWORD";
const char* BROKER_IP = "10.248.26.27";
const char* TOPIC     = "gateway/p4/current";

#define RO_PIN    26
#define DI_PIN    27
#define DE_RE_PIN 4

const char* BUF_FILE = "/buffer.txt";
const char* SEQ_FILE = "/seq.txt";
const uint32_t SEQ_SAVE_EVERY = 10;
const uint32_t MAX_BUF_BYTES  = 200000;
 
WiFiClient net;
MQTTClient mqtt;
uint32_t seq = 0;
uint32_t lastSavedSeq = 0;
uint32_t dropped = 0;
uint32_t wifiDownSince = 0;

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

int pollOnce(int16_t &mA) {
  while (Serial2.available()) Serial2.read();
  digitalWrite(DE_RE_PIN, HIGH);
  Serial2.write(REQ, 8);
  Serial2.flush();
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

void saveSeq(uint32_t v) {
  File f = LittleFS.open(SEQ_FILE, "w");
  if (f) { f.print(v); f.close(); }
  lastSavedSeq = v;
}

uint32_t loadSeq() {
  File f = LittleFS.open(SEQ_FILE, "r");
  if (!f) return 0;
  uint32_t v = f.readString().toInt();
  f.close();
  return v;
}

void bufferLine(const String &msg) {
  File f = LittleFS.open(BUF_FILE, "a");
  if (!f) { dropped++; return; }
  if (f.size() > MAX_BUF_BYTES) { f.close(); dropped++; return; }
  f.println(msg);
  f.close();
}

void flushBuffer() {
  if (!LittleFS.exists(BUF_FILE)) return;
  File f = LittleFS.open(BUF_FILE, "r");
  if (!f) return;
  int sent = 0;
  bool ok = true;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (!mqtt.publish(TOPIC, line, false, 1)) {
      Serial.printf("PUBLISH FAIL at line %d (%s), lastError=%d, returnCode=%d, wifi=%d\n",
                    sent, line.c_str(), (int)mqtt.lastError(),
                    (int)mqtt.returnCode(), (int)WiFi.status());
      ok = false;
      break;
    }
    sent++;
    mqtt.loop();
    delay(50);
  }
  f.close();
  if (ok) {
    LittleFS.remove(BUF_FILE);
    Serial.printf("FLUSHED %d buffered lines\n", sent);
  } else {
    Serial.println("FLUSH interrupted, file kept");
  }
}

bool online() {
  return WiFi.status() == WL_CONNECTED && mqtt.connected();
}

void tryReconnect() {
  static uint32_t lastTry = 0;
  if (millis() - lastTry < 5000) return;
  lastTry = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (wifiDownSince == 0) wifiDownSince = millis();
    if (millis() - wifiDownSince > 30000) {
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      wifiDownSince = millis();
    }
    return;
  }
  wifiDownSince = 0;

  if (!mqtt.connected()) {
    if (mqtt.connect("p4-gateway")) {
      Serial.println("MQTT OK");
      flushBuffer();
    } else {
      Serial.printf("MQTT CONNECT FAIL lastError=%d returnCode=%d\n",
                    (int)mqtt.lastError(), (int)mqtt.returnCode());
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  Serial2.begin(9600, SERIAL_8N1, RO_PIN, DI_PIN);

  if (!LittleFS.begin(true)) Serial.println("LittleFS FAILED");
  uint32_t saved = loadSeq();
  seq = (saved == 0) ? 0 : saved + SEQ_SAVE_EVERY;
  lastSavedSeq = seq;
  Serial.printf("BOOT seq starts after %lu\n", (unsigned long)seq);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  mqtt.begin(BROKER_IP, net);
  mqtt.setOptions(30, true, 5000);
}

void loop() {
  mqtt.loop();
  if (!online()) tryReconnect();

  int16_t mA;
  int r = pollOnce(mA);
  if (r == 0) {
    seq++;
    String msg = "seq=" + String(seq) + " current=" + String(mA) + " mA";
    if (online() && mqtt.publish(TOPIC, msg, false, 1)) {
      Serial.println("SENT " + msg);
    } else {
      bufferLine(msg);
      Serial.println("BUFFERED " + msg);
    }
    if (seq - lastSavedSeq >= SEQ_SAVE_EVERY) saveSeq(seq);
  } else if (r == 1) {
    Serial.println("TIMEOUT");
  } else {
    Serial.println("BAD FRAME");
  }
  Serial.printf("dropped=%lu\n", (unsigned long)dropped);
  delay(1000);
}