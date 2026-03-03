# IoT Training Guides

Hands-on guides for building real IoT projects. Every guide is written by doing -- tested on real hardware, with screenshots at every step.

## Guides

| Guide | What You Build | Hardware | Level |
|-------|---------------|----------|-------|
| [Matter Thermal Mesh](./matter-thermal-mesh/) | Whole-home presence sensor network with Home Assistant | SparkFun Thing Plus Matter, Arduino Nano Matter, XIAO MG24, RPi 4 | Beginner |
| [Arduino IDE vs Zephyr RTOS](./nano-matter-ide-vs-zephyr/) | Side-by-side comparison with visible stutter demo, BLE, energy analysis | Arduino Nano Matter (x2), SiWx917 Dev Kit | Intermediate |

---

### [Matter Thermal Mesh](./matter-thermal-mesh/)

Build a 3-sensor presence detection network using Matter/Thread. Includes Home Assistant integration, custom dashboard with animated person tracking, and a 7" touchscreen kiosk display.

**Stack:** Arduino IDE, Matter, Thread, Home Assistant, Flask + D3.js, Docker, Raspberry Pi

### [Arduino IDE vs Zephyr RTOS](./nano-matter-ide-vs-zephyr/)

See the 13-millisecond problem with your own eyes. An LED stutters on Arduino while reading an I2C sensor -- then runs perfectly smooth on Zephyr with three concurrent threads (LED + sensor + BLE). Same code runs on two different SoC families with zero changes.

**Stack:** Arduino IDE, Zephyr RTOS, BLE, I2C, Devicetree, west build system

---

## Who This Is For

Makers and developers who want to build real IoT projects, not just blink an LED. Each guide starts from zero -- no prior embedded experience needed. Follow along, copy-paste the commands, and you will have a working system.

## About

Personal hobby project. Built on weekends, tested on real hardware, documented for anyone to follow. Not affiliated with any company.

## License

MIT
