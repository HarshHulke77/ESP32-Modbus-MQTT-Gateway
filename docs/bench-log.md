# Bench Log (problems hit and fixes)

## 2026-10-21 - Logic analyzer: "Capture failed, generic/unspecified error"
- Symptom: device detected in PulseView (fx2lafw, D0 to D7 shown) but capture failed.
- Checked: Zadig showed WinUSB already installed on USB ID 0925 3881, so the driver was not the problem.
- Cause: unstable USB port.
- Fix: plugged into a different USB port directly on the laptop. Capture then completed.

## 2026-10-21 - Logic analyzer: D0 flat low on the junction
- Symptom: D0 read low for the whole capture with the CH1 lead on the junction row.
- Cause: ESP32 USB was not plugged in. The MAX485 takes its 5 V from VIN, so it was unpowered and the junction sat at 0 V.
- Fix: power the ESP32 first. Idle junction is about 2.42 V and D0 reads flat high.
- Side note: unconnected channels (D1 to D7) read high by themselves, so "D0 high" alone does not prove contact. Proof of contact: tie CH1 to GND and see D0 go low.

## 2026-10-23 - Arduino IDE 2.x Serial Monitor froze (blank window)
- Symptom: upload succeeded, then the IDE went blank and had to be killed in Task Manager.
- Fix: close the IDE fully, use PuTTY (Serial, CP210x COM port, 115200) instead.
