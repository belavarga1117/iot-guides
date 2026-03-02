# Smart Home Matter Starter — Image Guide

All images used in the setup guide and documentation.

## Product Photos (downloaded from manufacturer sites)

These are in `docs/images/products/` — downloaded from official sources.

| File | Product | Source |
|------|---------|--------|
| `raspberry-pi-4.jpg` | Raspberry Pi 4 Model B | Adafruit (adafruit.com/product/4296) |
| `rpi-7inch-touchscreen.jpg` | RPi Official 7" Touchscreen | Adafruit (adafruit.com/product/2718) |
| `xiao-mg24-sense.jpg` | Seeed Studio XIAO MG24 (Sense) | Seeed Wiki (wiki.seeedstudio.com) |
| `sparkfun-thing-plus-matter.jpg` | SparkFun Thing Plus Matter (MGM240P) | SparkFun (sparkfun.com/products/20270) |
| `arduino-nano-matter.jpg` | Arduino Nano Matter | Botland Store (ABX00112) |
| `sparkfun-grideye-amg8833.jpg` | SparkFun Grid-EYE AMG8833 (Qwiic) | SparkFun (sparkfun.com/products/14607) |
| `sparkfun-scd40.jpg` | SparkFun SCD40 CO2 Sensor (Qwiic) | SparkFun (sparkfun.com/products/22395) |
| `sparkfun-qwiic-oled.jpg` | SparkFun Qwiic OLED 1.3" (128x64) | SparkFun (sparkfun.com/products/23453) |
| `adafruit-is31fl3741-led-matrix.jpg` | Adafruit IS31FL3741 13x9 RGB LED Matrix | Adafruit (adafruit.com/product/5201) |
| `sparkfun-qwiic-cables.jpg` | SparkFun Qwiic Cable Kit | SparkFun (sparkfun.com/products/15081) |

## Automated Screenshots (Playwright + scrot)

Generated during guide creation. Stored in `docs/screenshots/`.
See `scripts/playwright/specs/` for the Playwright scripts.

| File | Source | Method |
|------|--------|--------|
| `01-rpi-imager-settings.png` | RPi Imager on Mac | Manual `screencapture` |
| `02-ssh-first-login.png` | Terminal SSH session | Terminal export / `script` |
| `03-docker-compose-up.png` | Terminal output | Terminal export / `script` |
| `04-ha-onboarding.png` | HA web UI | Playwright |
| `05-ha-welcome.png` | HA web UI | Playwright |
| `06-ha-integrations.png` | HA web UI | Playwright |
| `07-ha-matter-setup.png` | HA web UI | Playwright |
| `08-ha-otbr-setup.png` | HA web UI | Playwright |
| `09-ha-commissioning.png` | HA web UI | Playwright |
| `10-ha-entities-list.png` | HA web UI | Playwright |
| `11-ha-temperature-entity.png` | HA web UI | Playwright |
| `12-ha-occupancy-entity.png` | HA web UI | Playwright |
| `13-ha-dashboard-cards.png` | HA web UI | Playwright |
| `14-ha-automations.png` | HA web UI | Playwright |
| `15-flask-heatmap.png` | Flask dashboard | Playwright |
| `16-flask-mesh-topology.png` | Flask dashboard | Playwright |
| `17-flask-floorplan.png` | Flask dashboard | Playwright |
| `arduino-01-board-manager.png` | Arduino IDE on Mac | Manual `screencapture -w` |
| `arduino-02-board-select.png` | Arduino IDE on Mac | Manual `screencapture -w` |
| `arduino-03-port-select.png` | Arduino IDE on Mac | Manual `screencapture -w` |
| `arduino-04-sketch-open.png` | Arduino IDE on Mac | Manual `screencapture -w` |
| `arduino-05-compile-ok.png` | Arduino IDE on Mac | Manual `screencapture -w` |
| `arduino-06-upload-ok.png` | Arduino IDE on Mac | Manual `screencapture -w` |
| `arduino-07-serial-monitor.png` | Arduino IDE on Mac | Manual `screencapture -w` |

## User Photos Needed (PLACEHOLDERS)

Only 3 photos needed from you! Take them when the full setup is working.

### PHOTO 1: Hero Shot (REQUIRED)
**File:** `docs/images/user/hero-complete-setup.jpg`
**What:** All hardware assembled and working together
**Include:** RPi 4 + 7" touchscreen showing dashboard, XIAO MG24 plugged into USB, SparkFun + Nano Matter boards with Qwiic sensors connected, cables visible
**Tip:** Good lighting, top-down or 45-degree angle, clean desk background

### PHOTO 2: Kiosk Display (REQUIRED)
**File:** `docs/images/user/kiosk-display-live.jpg`
**What:** The 7" touchscreen showing the live heatmap dashboard in kiosk mode
**Include:** Close-up of the display with the Flask D3.js heatmap visible, RPi behind it
**Tip:** Slight angle to avoid screen glare

### PHOTO 3: Sensor Board Close-up (OPTIONAL but nice)
**File:** `docs/images/user/sensor-board-closeup.jpg`
**What:** One sensor board with Qwiic daisy-chained sensors
**Include:** SparkFun Thing Plus + Grid-EYE + SCD40 + OLED connected via Qwiic cables
**Tip:** Show the blue Qwiic cables clearly

## Architecture Diagrams (generated AFTER final setup is confirmed)

Will be created at the end when the training setup is finalized.
These will include:
- System architecture (RPi + Docker + HA + OTBR + Matter + Thread)
- Hardware wiring diagram (visual, with product photos)
- Thread mesh network topology
- Data flow diagram (sensor -> Matter -> HA -> Dashboard)
