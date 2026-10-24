#include "driver/uart.h"
#define DE_PIN 4

void setup() {
    Serial.begin(115200); // PuTTY par result print karne ke liye
    pinMode(DE_PIN, OUTPUT);
    digitalWrite(DE_PIN, LOW);
    Serial2.begin(9600, SERIAL_8N1, 26, 27);
}

void loop() {
    uint8_t frame[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
    
    // Step 1: Naya data bhejne se pehle purana kachra clear karo
    while (Serial2.available()) Serial2.read();

    digitalWrite(DE_PIN, HIGH);
    Serial2.write(frame, 8);
    uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(100));
    digitalWrite(DE_PIN, LOW);

    // Step 2 & 3: Timer start karo aur 100ms tak data read karo
    uint8_t rx[16];
    int n = 0;
    unsigned long t0 = millis();
    
    while (millis() - t0 < 100) {
        if (Serial2.available()) {
            rx[n] = Serial2.read(); // Yahan blank fill kiya gaya hai
            n++;
        }
    }

    // Step 4: Check karo kitne bytes aaye aur print karo
    if (n == 0) {
        Serial.println("TIMEOUT");
    } else { 
        Serial.print("got bytes: "); 
        Serial.println(n); 
    }

    delay(1000);
}