# ESP32-Modbus-MQTT-Gateway

P4: Modbus RTU master gateway on ESP32 + MAX485. Polls a Modbus RTU slave over RS-485 and publishes each reading to an MQTT broker at QoS 1. If the broker or WiFi is unreachable, readings are buffered in flash and sent in order on reconnect.

Status (2026-10-28): firmware and fault tests done. Custom PCB not started. See `STATUS.md` for the session log and `docs/bench-log.md` for problems hit and fixes.

## Architecture

```
Field device (ESP32 #2)        Gateway (ESP32)                    Laptop
Modbus RTU slave, ID 1   <-->  MAX485 -> UART2 -> poll loop  -->  Mosquitto (port 1883)
9600 8N1, register 0 = mA      CRC check, seq counter             MQTT Explorer to watch
                               LittleFS buffer + saved seq
```

The gateway runs a single poll loop, about one reading per second (1 s delay plus the Modbus round trip). It is not split into FreeRTOS tasks.

- **Poll:** read holding register 0, request `01 03 00 00 00 01 84 0A`. The reply is a signed 16-bit current in mA. A reading is accepted only if the slave ID, function code, byte count and CRC are all valid. Timeout is 30 ms.
- **Sequence number:** `seq` increments only on a good reading, so a gap in seq means lost data, not a failed poll.
- **Publish:** topic `gateway/p4/current`, payload `seq=N current=X mA`, QoS 1.
- **Store-and-forward:** if offline, the line is appended to `/buffer.txt` on LittleFS (cap 200000 bytes). On MQTT reconnect the file is flushed in order, then deleted. Lines past the cap are counted in `dropped`.
- **Reconnect:** WiFi auto-reconnect is on and WiFi sleep is off. The sketch calls `WiFi.begin()` at most once per 30 s.
- **Seq persistence:** `seq` is saved to `/seq.txt` every 10 readings. After an unclean reboot it resumes at the saved value + 10, so up to 9 seq numbers are skipped by design and never reused.

## Wiring

| Gateway pin | Connects to |
|---|---|
| GPIO26 | MAX485 RO, through a voltage divider (values: [FILL]) |
| GPIO27 | MAX485 DI |
| GPIO4 | MAX485 DE and RE tied together (HIGH = transmit) |

Field device is a second ESP32 running the Project 2 slave firmware. A and B go to the MAX485 A and B terminals. Lift the DE/RE wire off GPIO4 before flashing the gateway.

## Broker setup

Mosquitto 2.1.2 on a Windows laptop, started by hand, not as a service. `p4.conf`:

```
listener 1883 0.0.0.0
allow_anonymous true
```

Anonymous access is for a private hotspot only. The `0.0.0.0` bind matters: a bare `listener 1883` here bound to localhost only, and the gateway failed with `lastError=-3`.

## Build and configure

Arduino IDE, board ESP32 Dev Module, libraries `MQTT` (arduino-mqtt) and `LittleFS`. Set `WIFI_SSID`, `WIFI_PASS` and `BROKER_IP` at the top of the sketch. The committed sketch uses placeholders; real credentials stay in the local copy and are never committed. The laptop IP changes if the hotspot changes, so run `ipconfig` first if MQTT will not connect.

## Results

### Happy path (2026-10-27)

QoS 1 over WiFi: the Mosquitto log shows `q1` and a PUBACK for every publish, consecutive message IDs, about 1.1 s apart. Sequence numbers in MQTT Explorer matched the serial output.

### Store-and-forward (2026-10-27)

| Test | Result |
|---|---|
| Broker stopped | seq 88 to 94 buffered, `FLUSHED 7`, next SENT 95 |
| Broker stopped (earlier run) | seq 31 to 35 buffered, `FLUSHED 5`, next SENT 36 |
| Larger flushes | 15 and 30 lines flushed |
| Power cycle | `BOOT seq starts after 450`, first reading 451 |
| Accidental power cut after seq 2762 | last save 2760, resumed at 2770, buffered 2771 to 2775, `FLUSHED 5`, SENT 2776 |

Seq 2763 to 2770 never appears. That is the intended cost of saving every 10 readings. `dropped=0` throughout.

### Fault tests (2026-10-28)

| Test | Result | Evidence |
|---|---|---|
| WiFi off 10 min | Last SENT seq 3787. Buffered seq 3788 to 4314. `FLUSHED 527`, `dropped=0`. First seq after flush 4315 (3787 + 1 + 527). No gap, no duplicate. | Screenshots of PuTTY output |
| RS-485 A wire pulled 30 s | 28 TIMEOUT, 0 BAD FRAME. Gateway resumed sending after the wire was replugged. | Counted by eye on screen, not from a saved log. Seq continuity not recorded. |
| Power cut while buffering | About 25+ readings in flash (buffered through about seq 6260). Gateway rebooted, reconnected and resumed sending, `dropped=0`. | Partial. `BOOT` line and the main `FLUSHED N` were not read, so buffer survival is indicated, not confirmed. |

## Bugs found

| Symptom | Cause | Fix |
|---|---|---|
| `sta is connecting, cannot set config` | `tryReconnect()` called `WiFi.begin()` every 5 s, restarting an attempt that took longer than 5 s | Auto-reconnect on, one restart per 30 s |
| `MQTT CONNECT FAIL lastError=-3` with laptop IP correct | Mosquitto listening on 127.0.0.1 only | `listener 1883 0.0.0.0` |
| Long run of TIMEOUT after a re-flash | Wires in wrong breadboard holes | Wiring corrected, not a code fault |

## Known limits and open questions

- Not measured: seconds from WiFi return to `FLUSHED`, and the Mosquitto `disconnected:` count during the WiFi test.
- One `PUBLISH FAIL ... lastError=-9` appeared during an early large flush after the broker fix. Cause not established; it did not recur in the 527-line flush.
- Garbage characters sometimes appear on the USB serial output, before large `FLUSHED` lines and once with no flush. Cause unknown. It has not been shown to be a firmware reset problem or ruled out as one.
- If a flush is interrupted, the sketch keeps only the unsent lines and retries from there. This path was added in `gateway_sf2` but has not been exercised, because no `FLUSH interrupted` has occurred since.
- The 200000-byte buffer cap has not been tested.
- The current reading is uncalibrated here: the zero point shifted when the power source changed (about -15 to +16 mA on 2026-10-26, then about -57 to -143 mA). The numbers are for fault-handling tests, not accurate current measurement.

## Next

Custom PCB for the gateway (KiCad), schematic checkpoint 2027-01-31.