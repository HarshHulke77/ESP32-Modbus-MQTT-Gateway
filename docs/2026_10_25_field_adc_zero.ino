void setup() {
  Serial.begin(115200);
  analogSetPinAttenuation(34, ADC_11db);
}
void loop() {
  long sum = 0;
  int lo = 9999, hi = 0;
  for (int i = 0; i < 500; i++) {
    int mv = analogReadMilliVolts(34);
    sum += mv;
    if (mv < lo) lo = mv;
    if (mv > hi) hi = mv;
    delay(2);
  }
  Serial.printf("avg %ld  min %d  max %d\n", sum / 500, lo, hi);
}


2026-10-25 field device ADC zero, no load current
ESP32 #2, GPIO34, ADC_11db, analogReadMilliVolts, 500 samples/block, 2 ms spacing
Divider 9.55k / 21k (measured), meter at junction 1.70 V, VCC 5.01 V
avg 1790  min 1727  max 1843
avg 1791  min 1732  max 1871
avg 1790  min 1716  max 1911
avg 1789  min 1700  max 1871
avg 1790  min 1718  max 1839