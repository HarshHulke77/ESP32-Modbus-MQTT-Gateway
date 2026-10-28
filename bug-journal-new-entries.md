## 003 — Gateway could not reach Mosquitto: broker bound to localhost only

2026-10-28 · Project 4, Modbus-MQTT gateway · Broker configuration

**Symptom** After starting Mosquitto, the gateway kept printing `MQTT CONNECT FAIL lastError=-3 returnCode=0` and `BUFFERED seq=...` with `dropped=0`. Mosquitto showed no connection attempts from the gateway.

**Hypotheses**

1. The hotspot gave the laptop a new IP and the sketch's `BROKER_IP` was stale.
2. Windows Firewall was blocking port 1883.
3. Mosquitto was not listening on the WiFi interface.
4. The broker was rejecting the client (login or protocol problem).

**Falsification**
- (1) `ipconfig` showed the laptop still at 10.248.26.27, same as the sketch. Ruled out.
- (4) `returnCode=0` means the broker never answered at all, so it was not a rejection. Ruled out.
- (2) and (3) `netstat -an | findstr 1883` showed only `127.0.0.1:1883` and `[::1]:1883`, both loopback. A firewall rule would not matter for a socket nothing outside the laptop can reach. Pointed at (3).

**Localisation** Changed `p4.conf` to `listener 1883 0.0.0.0` and restarted. `netstat` then showed `0.0.0.0:1883 LISTENING` and the gateway connected immediately, with PUBLISH and PUBACK lines in the Mosquitto window.

**Root cause** Mosquitto was listening on loopback only, so no other device could connect. The exact reason it bound to localhost is not established. The likely candidates are that the running instance was not using `p4.conf`, or that this version's default for a bare `listener` line is local-only. I did not test which.

**Fix** Explicit bind address in `p4.conf`: `listener 1883 0.0.0.0`.

**Cost** 30min

**Prevention** After starting any broker, run `netstat -an | findstr 1883` and confirm `0.0.0.0:1883` before blaming the device or the network. Read the error code first: `lastError=-3` with `returnCode=0` means the TCP connection failed, which is a network-layer problem, not an MQTT one.

---

## 004 — `sta is connecting, cannot set config`: reconnect logic restarting its own attempts

2026-10-27 · Project 4, Modbus-MQTT gateway · WiFi reconnect

**Symptom** Serial output printed `sta is connecting, cannot set config` repeatedly and WiFi did not recover. Earlier Mosquitto lines showed `connection closed by client` and `exceeded timeout (keepalive k10)`.

**Hypotheses** I thought it was the mqtt problem but then I realised it was just I forget to add the password changes in the actual code.

**Falsification** Quick Realisation when pressured on what might be wrong.

**Localisation** The message pointed at `WiFi.begin()` being called while an attempt was in progress. My `tryReconnect()` called `WiFi.begin()` every 5 s, and a connection attempt can take longer than 5 s, so each call restarted the attempt before it finished.

**Root cause** The retry interval was shorter than the time one attempt needs, so the reconnect logic kept resetting itself.

**Fix** Turned on `WiFi.setAutoReconnect(true)` and `WiFi.setSleep(false)`, set MQTT keepalive options with `mqtt.setOptions(30, true, 5000)`, and limited manual `WiFi.begin()` to once per 30 s.

**Cost** 20min

**Prevention** A retry timer must be longer than the slowest successful attempt. When an API returns "busy" or "already connecting", suspect your own retry loop first. Note: after the fix I believe there were no disconnects, but I never counted the `disconnected:` lines, so that claim is not measured.

---

## 005 — Long run of TIMEOUT after a re-flash: wires in the wrong breadboard holes

2026-10-27 · Project 4, Modbus-MQTT gateway · RS-485 wiring

**Symptom** After a re-flash, the gateway printed `TIMEOUT` continuously instead of readings.

**Hypotheses** After the coded uploaded I connected one wire in adjacent hole on the breadboard instead of the actual hole.

**Falsification** The same sketch had worked before the re-flash, which made a code fault less likely. I thought the mqtt got disconnected.

**Localisation** Inspecting the breadboard showed wires connected to the wrong holes.

**Root cause** Wiring error, not a code fault.

**Fix** Moved the wires back to the correct holes.

**Cost** 30min

**Prevention** When a working sketch suddenly fails after a re-flash, check the physical wiring before the code. A breadboard photo of the working state, committed to the repo, would settle this in seconds.

---

## 006 — Real WiFi password committed to a public repo

2026-10-28 · Project 4, Modbus-MQTT gateway · Repository hygiene

**Symptom** None at runtime. Found by running `git log -S"harsh123" --oneline` at the start of the session.

**Hypotheses** Not applicable. This was a deliberate check, prompted by the sketch holding the real password.

**Falsification** The search returned commit `0db07a2`. `git log -S"harsh123" --oneline origin/main` after the push showed `0db07a2` (added) and `9487c1d` (removed).

**Localisation** The password was added in `0db07a2`, and I pushed that commit to the public repo along with the one that removed it.

**Root cause** I committed a sketch with the real password in it. Removing it from the file in a later commit does not remove it from history, so anyone could read the old commit on GitHub.

**Fix** Changed the hotspot password so the leaked one is worthless. I chose this over rewriting history, because a force-push costs more than the exposure does on a private hotspot. The new password lives only in the local sketch. The repo uses placeholders.

**Cost** 20min

**Prevention** Before every commit, I changed the password in the code directly. Never use `git add .` while a credentialed sketch is inside the repo folder. History is permanent, so a leak is fixed by rotating the secret, not by editing the file.

---

## 007 — Re-flash did not take the new WiFi password (gateway stayed on the old one)

2026-10-28 · Project 4, Modbus-MQTT gateway · Deployment

**Symptom** After changing the hotspot password and powering the setup back on, the gateway kept printing `BUFFERED seq=...` and never printed `MQTT OK`, even though Mosquitto was running on the correct IP.

**Hypotheses**

1. Broker not reachable (as in entry 003).
2. Gateway could not join the hotspot.

**Falsification** `ipconfig` and the Mosquitto startup output were both correct, which pointed away from the broker. I then recalled that I had not uploaded the sketch after changing the password.

**Localisation** The board was still running firmware with the old password.

**Root cause** I changed the hotspot password but did not re-flash the gateway.

**Fix** Put the new password in the local sketch and flashed. `MQTT OK` appeared right after.

**Cost** 5min

**Prevention** When a credential changes, update and flash the device in the same step, then confirm `MQTT OK` before starting any test.

---

## OPEN — not bugs yet, no root cause

These do not get a root cause until they are measured.

- **`PUBLISH FAIL ... lastError=-9`** during an early large flush after the broker fix. Not reproduced in the later 527-line flush. Cause unknown.
- **Garbage characters on USB serial** before large `FLUSHED` lines, and once with no flush (before `SENT seq=6322`). Not shown to be a firmware reset, not ruled out.
- **Zero-point shift** from about -15 to +16 mA (2026-10-26) to about -57 to -143 mA after the power source changed. Cause not established. Take the 500-sample noise measurement with the power setup you keep.

---

## 008 — <next entry>

