# Status Log
<!-- One line per session. Absolute dates. Append, never rewrite. -->

## Log
- 2026-10-20 - Gateway-side MAX485 wired (RO via 9.9k/21k divider to GPIO26, DI to GPIO27, DE+RE to GPIO4, VCC 5 V from VIN). First power-up passed: VCC 5.00 V, junction 2.42 V idle.
- 2026-10-21 - Logic analyzer working (PulseView, fx2lafw, Zadig WinUSB on USB ID 0925 3881; fix was a different USB port). Idle-bus capture, 1 MHz, 5 s, CH1 on the junction row: flat high, no flicker, so no external bias resistors.
- 2026-10-23 - Modbus CRC-16 verified on ESP32: request 01 03 00 00 00 01 gives 84 0A. Open: send the frame over UART2 with DE control (GPIO4, uart_wait_tx_done before DE low).
