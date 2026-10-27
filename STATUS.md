# Status Log
<!-- One line per session. Absolute dates. Append, never rewrite. -->

## Log
- 2026-10-20 - Gateway-side MAX485 wired (RO via 9.9k/21k divider to GPIO26, DI to GPIO27, DE+RE to GPIO4, VCC 5 V from VIN). First power-up passed: VCC 5.00 V, junction 2.42 V idle.
- 2026-10-21 - Logic analyzer working (PulseView, fx2lafw, Zadig WinUSB on USB ID 0925 3881; fix was a different USB port). Idle-bus capture, 1 MHz, 5 s, CH1 on the junction row: flat high, no flicker, so no external bias resistors.
- 2026-10-23 - Modbus CRC-16 verified on ESP32: request 01 03 00 00 00 01 gives 84 0A. Open: send the frame over UART2 with DE control (GPIO4, uart_wait_tx_done before DE low).
2026-10-26 - P4 step 5 closed: poll gap 4.277 ms, request 8.3 ms, reply 7.3 ms, DE falls before reply. Timeout 100 -> 30 ms.

2026-10-26 - P4 timeout 100 -> 30 ms. A-wire-pull retest passed (TIMEOUT, immediate recovery, no BAD FRAME). Garbage on serial not reproduced, cause unknown. 
2026-10-26 - P4 step 6 piece 1 closed: seq counter (RAM) verified. seq continues across TIMEOUT, resets to 1 on reboot. Flash-saved seq (every 10) comes with LittleFS.
2026-10-27 - P4 step 6 piece 2 closed: WiFi + MQTT QoS 1 happy path. PUBACK on every publish, m6-m20 consecutive, ~1.1 s interval. Broker: Mosquitto installed fresh, listener 1883, p4.conf.
2026-10-27 - P4 step 6 piece 3 closed: store-and-forward verified (seq 88-94 buffered, FLUSHED 7, dropped=0). Power cycle: BOOT after 450, first seq 451, no duplicates. Zero point shifted to about -70 mA after power source change.