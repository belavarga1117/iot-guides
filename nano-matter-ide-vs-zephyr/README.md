# Arduino IDE vs Zephyr RTOS: A Hands-On Comparison

A training guide that demonstrates **why** an Arduino developer should learn Zephyr RTOS -- with visible, screencastable proof on real hardware.

## The Demo

Two Arduino Nano Matter boards run side by side. One runs an Arduino sketch, the other runs a Zephyr app. Both do the same thing: breathe an LED while reading a Grid-EYE thermal sensor over I2C.

The Arduino board stutters. The Zephyr board does not.

The same Zephyr code also runs on a SiWx917 Dev Kit -- a completely different SoC family -- with zero code changes.

## What You Need

| Board | Qty | Role |
|-------|-----|------|
| Arduino Nano Matter + Carrier | 2 | Arduino demo + Zephyr demo |
| SiWx917 Dev Kit (BRD2605A) | 1 | Portability demo |
| SparkFun Grid-EYE AMG8833 | 3 | Thermal sensor (I2C, Qwiic) |

## Quick Start

**Arduino (the "before"):**
```bash
arduino-cli compile --fqbn SiliconLabs:silabs:nano_matter firmware/arduino/led-sensor/
arduino-cli upload -p /dev/cu.usbmodemXXX --fqbn SiliconLabs:silabs:nano_matter firmware/arduino/led-sensor/
```

**Zephyr (the "after"):**
```bash
cd ~/zephyrproject
west build -b arduino_nano_matter /path/to/firmware/zephyr/led-sensor
west flash
```

## Guide

The full setup guide with explanations, serial output evidence, and troubleshooting:

**[docs/SETUP-GUIDE.md](docs/SETUP-GUIDE.md)**

## Project Structure

```
firmware/
  arduino/led-sensor/         Arduino sketch (single-threaded, blocking)
  zephyr/led-sensor/          Zephyr app (3 threads: LED, sensor, BLE)
    boards/                   Board-specific overlays + configs
docs/
  SETUP-GUIDE.md              Full guide (Parts 0-8 + Troubleshooting)
  serial-capture-evidence.md  Raw serial output from all 3 boards
  diagrams/                   Mermaid source + rendered PNGs
  screenshots/                Step-by-step screenshots
```

## Key Results

| Metric | Arduino | Zephyr |
|--------|---------|--------|
| LED blocked during I2C? | Yes (13ms) | No |
| Concurrent tasks | 1 | 3 |
| Cross-board portability | No | Yes |
| BLE advertising | Not implemented | Works (Nano Matter + SiWx917) |

## License

Personal hobby project. Not affiliated with any company.
