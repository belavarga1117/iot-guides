# Serial Capture Evidence — Stutter Demo

Captured 2026-03-03, all 3 boards running simultaneously.

## Arduino Nano Matter #1 (Blocking loop — GPIO toggle + 3× reads)

Port: `/dev/cu.usbmodemXXXXXXXXX` at 115200 baud (Arduino board)

<pre style="background:#1e1e1e;color:#d4d4d4;padding:14px 18px;border-radius:6px;font-family:'Menlo','Monaco','Courier New',monospace;font-size:12.5px;line-height:1.6;overflow-x:auto;">=============================================
  Arduino LED + Grid-EYE Demo (Stutter Demo)
=============================================
Watch the blue LED on this board.

TARGET:  LED toggles fast -> appears as steady glow
REALITY: 3x sensor reads block ~39ms per loop
         LED only toggles at ~12Hz -> visible blink!

Now look at the Zephyr board: smooth PWM breathing.
Same sensor. Same LED. Different architecture.

AMG8833 Grid-EYE initialized at 0x69.
Starting LED blink + sensor polling...

[STUTTER] Loop #20: 41.4 ms total (41.4 ms I2C) | ~12 Hz blink | Max: 26.0 C
[STUTTER] Loop #40: 40.8 ms total (40.7 ms I2C) | ~12 Hz blink | Max: 25.8 C
[STUTTER] Loop #60: 40.3 ms total (40.3 ms I2C) | ~12 Hz blink | Max: 26.0 C
[STUTTER] Loop #80: 41.0 ms total (41.0 ms I2C) | ~12 Hz blink | Max: 25.8 C
[STUTTER] Loop #100: 41.1 ms total (41.1 ms I2C) | ~12 Hz blink | Max: 25.8 C
</pre>

**Key numbers:**
- 3× sensor reads per loop: **~39ms** of I2C blocking per iteration
- LED toggle rate: **~12Hz** — clearly visible on/off blink (like a turn signal)
- Total loop time: **~41ms** (sensor reads dominate everything)
- Result: crude blink instead of steady glow

## Zephyr on Arduino Nano Matter #2 (Threaded — 3× reads on sensor thread)

Port: `/dev/cu.usbmodemXXXXXXXXX` at 115200 baud (Zephyr board)

<pre style="background:#1e1e1e;color:#d4d4d4;padding:14px 18px;border-radius:6px;font-family:'Menlo','Monaco','Courier New',monospace;font-size:12.5px;line-height:1.6;overflow-x:auto;">*** Booting Zephyr OS build v4.3.0-6636-g3a40c90b9801 ***

=============================================
  Zephyr LED + Grid-EYE + BLE Demo
=============================================
Board: arduino_nano_matter
Threads:
  LED    (priority 5) — PWM breathing
  Sensor (priority 7) — Grid-EYE 3x read every 100 ms
  BLE    (priority 9) — advertise every 1000 ms
=============================================

[LED] Thread started (priority 5, tick 10 ms)
[LED] PWM mode — smooth breathing on blue LED
[SENSOR] Thread started (priority 7, poll 100 ms)
[SENSOR] AMG88xx initialized via devicetree
[BLE] Thread started (priority 9, update 1000 ms)
[BLE] Bluetooth initialized
[BLE] Advertising as 'ZephyrGridEye'
[SENSOR] 3x read took 37 ms | Max: 24.00 C
[BLE] Broadcasting temp: 24.00 C
[SENSOR] 3x read took 37 ms | Max: 23.75 C
[LED] tick #500: 0 ms (target: <10 ms)
[SENSOR] 3x read took 37 ms | Max: 24.00 C
[BLE] Broadcasting temp: 24.00 C
</pre>

**Key numbers:**
- Sensor 3× reads: **37ms** per poll (same workload as Arduino)
- LED thread: runs undisturbed at **10ms ticks** (kernel scheduler)
- LED tick elapsed: **0 ms** — proves zero interference from sensor I2C
- BLE: broadcasting temperature every 1s
- Result: **smooth PWM breathing** — LED never stutters

## Zephyr on SiWx917 DK (Portability Demo — after NWP firmware update)

Port: `/dev/cu.usbmodemXXXXXXXXX` at 115200 baud (SiWx917 board)

**Before NWP update:** BLE failed with `err -19` (ENODEV). The NWP (Network Wireless Processor) firmware was outdated.

**After NWP update** (`commander rps load SiWG917-B.2.15.5.0.0.12.rps`):

<pre style="background:#1e1e1e;color:#d4d4d4;padding:14px 18px;border-radius:6px;font-family:'Menlo','Monaco','Courier New',monospace;font-size:12.5px;line-height:1.6;overflow-x:auto;">*** Booting Zephyr OS build v4.3.0-6636-g3a40c90b9801 ***

=============================================
  Zephyr LED + Grid-EYE + BLE Demo
=============================================
Board: siwx917_dk2605a
Threads:
  LED    (priority 5) — GPIO blink
  Sensor (priority 7) — Grid-EYE 3x read every 100 ms
  BLE    (priority 9) — advertise every 1000 ms
=============================================

[LED] Thread started (priority 5, tick 10 ms)
[LED] GPIO mode — blink pattern on LED0
[SENSOR] Thread started (priority 7, poll 100 ms)
[SENSOR] AMG88xx initialized via devicetree
[BLE] Thread started (priority 9, update 1000 ms)
[BLE] Advertising as 'ZephyrGridEye'
[SENSOR] 3x read took 29 ms | Max: 25.50 C
[BLE] Broadcasting temp: 25.50 C
[SENSOR] 3x read took 28 ms | Max: 25.25 C
[SENSOR] 3x read took 29 ms | Max: 25.25 C
[BLE] Broadcasting temp: 25.25 C
</pre>

**Key numbers:**
- Same code, different board — **zero source code changes**
- Sensor 3× reads: **28-29 ms** (SiWx917 I2C is slightly faster)
- LED: GPIO blink (DK LEDs not on PWM pins — code auto-adapts)
- BLE: **Broadcasting temperature** — all 3 threads running
- NWP firmware update was the only fix needed (one-time, no code changes)

## Comparison Table

| Metric | Arduino (blocking) | Zephyr Nano Matter (threaded) | Zephyr SiWx917 (portability) |
|--------|-------------------|-------------------------------|------------------------------|
| Sensor workload | 3× reads (~39ms) | 3× reads (~37ms) | 3× reads (~29ms) |
| LED behavior | On/off blink at ~12Hz | Smooth PWM breathing at 100Hz | GPIO blink (LEDs not on PWM pins) |
| LED blocked during I2C | **YES** (entire loop) | NO (separate thread) | NO (separate thread) |
| BLE advertising | Not implemented | 1s interval | 1s interval (after NWP update) |
| Cross-board portability | No | Yes (overlay swap) | Yes (overlay swap) |
| Visible stutter | **Yes — crude blink** | **No — smooth breathing** | N/A (GPIO blink by design) |
