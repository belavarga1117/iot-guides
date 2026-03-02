# HeatMap Home — Project History & Build Journal

> **Purpose:** Raw, honest record of what actually happened during the build.
> Used later for blog posts, talks, and authentic content.
> Includes failures, surprises, emotions, and real timestamps.
>
> **NOT open source** — this is our content source material.

---

## Timeline Overview

| Date | Milestone | Mood |
|------|-----------|------|
| Late Feb 2026 | Research phase: SiLabs ecosystem analysis | Curious, slightly frustrated |
| Feb 28 | Hardware arrives, first experiments | Excited |
| Feb 28 | XIAO MG24 RCP nightmare begins | Frustrated → determined |
| Feb 28 | darkxst firmware discovery + UART breakthrough | Eureka moment |
| Feb 28 | DTR bug discovery + socat workaround | "Are you kidding me" → relief |
| Feb 28 | HAOS OTBR working, both Matter boards commissioned | Triumph |
| Mar 1 | Decision: scrap HAOS, rebuild on RPi OS Desktop | Bold pivot |
| Mar 1 | Full rebuild: RPi OS + Docker + HA + OTBR | Systematic, confident |
| Mar 1 | HA onboarding automated, screenshots taken | Satisfying |
| Mar 1 | All 3 boards re-flashed from scratch | Validated |
| Mar 1 | Strategy & content planning | Ambitious, energized |

---

## Pre-History: The Research Phase (Late February 2026)

### The Starting Point
A Product Marketing Manager at Silicon Labs who:
- Markets microcontrollers and wireless chips daily
- Has never written a line of firmware
- Has never SSH'd into a server
- Feels like an imposter at a deeply technical company
- Decided to do something about it

### The AI Experiment
"What if I used Claude Code as a co-pilot to actually BUILD something with the products I market?"

Not a toy demo. A real, useful system:
- Whole-home thermal presence detection
- Matter over Thread (the protocols SiLabs champions)
- Real-time visualization
- Open source, reproducible, beginner-friendly

### The Research (SILABS-MASTERPLAN-V2.md)
Claude analyzed the entire Silicon Labs developer ecosystem. The findings were... sobering:

**Silicon Labs vs ESP32 — The Brutal Scoreboard:**
- Beginner friendliness: SiLabs 3/10, ESP32 10/10
- Community size: SiLabs 2/10, ESP32 10/10
- Arduino stars: SiLabs 148, ESP32 16,400+ (111x fewer)
- ESPHome support: SiLabs ZERO, ESP32 complete

**Key Quote from a SiLabs Forum:**
> "This is absolutely ridiculous" — developer describing days of failed attempts to get started

This research was honest, uncomfortable, and exactly what a PMM should know. It fueled the mission: if I can make SiLabs accessible to ME, I can show it's accessible to anyone.

---

## Day 1: Hardware Arrives (February 28, 2026)

### The Unboxing
Hardware spread across the desk:
- **Raspberry Pi 4** (4GB) — the brain
- **Seeed XIAO MG24 (Sense)** — tiny Thread radio, $7.90
- **SparkFun Thing Plus Matter** — sensor node with MGM240P
- **Arduino Nano Matter** — sensor node with MGM240S
- **Grid-EYE AMG8833** — 8x8 thermal camera (Qwiic)
- **SCD40** — CO2 + humidity sensor (Qwiic)
- **SparkFun Qwiic OLED 1.3"** — small status display
- **Adafruit IS31FL3741** — 13x9 RGB LED matrix
- **Qwiic cables** — the magic that means no soldering

**First impression:** These are TINY boards. The XIAO MG24 is the size of a postage stamp. The fact that it can run a Thread Border Router is wild.

**Emotional note:** Physically holding the chips I've been marketing from slide decks. They're real. They have weight. This changes things.

### Qwiic = No Soldering
The single most important decision for this project: **everything connects via Qwiic cables** (small 4-pin JST connectors). Click in, click out. No soldering iron, no breadboard, no wiring diagrams.

This is what makes the project reproducible by a complete beginner. If I had to solder, the project would have died on Day 1.

---

## Day 1: The XIAO MG24 RCP Nightmare (February 28)

### The Plan (Simple)
1. Flash OpenThread RCP firmware to XIAO MG24
2. Plug into Raspberry Pi
3. OTBR container talks to it
4. Thread network is live

### What Actually Happened (Not Simple)

#### Attempt 1: Arduino Core Pre-built RCP
The Arduino SiLabs core includes a pre-built `openthread_rcp_xiao_mg24.hex`. Should be plug-and-play, right?

**Result:** Zero response. No communication at any baud rate. On Mac AND on RPi.

**Hours spent:** ~2 trying different baud rates, cables, USB ports.

**Emotion:** "Is the board broken? Is my cable bad? Am I that incompetent?"

#### The UART Discovery (Breakthrough #1)
Claude helped me dig into the XIAO MG24 schematic. The discovery:

- **USART0 (PA8 TX / PA9 RX)** = connected to the CMSIS-DAP USB bridge chip = what you see on USB
- **EUSART1 (PC6 TX / PC7 RX)** = broken out to external header pins = Arduino `Serial1`

The pre-built Arduino RCP was sending data to the **external header pins**, not through USB. It was literally talking to thin air.

**Quote to self:** "The official firmware for this board... doesn't work with this board's USB port."

**Content gold:** This is exactly the kind of pain point that makes SiLabs score 3/10 on beginner friendliness. The pre-built firmware assumes you know which UART is which. A beginner doesn't.

#### Finding darkxst Firmware (Breakthrough #2)
Claude found a community firmware builder: [darkxst/silabs-firmware-builder](https://github.com/darkxst/silabs-firmware-builder) on GitHub. One person, maintaining firmware builds for multiple SiLabs boards.

The darkxst XIAO MG24 RCP firmware:
- Correctly targets USART0/PA8/PA9 (the USB-connected UART)
- Baud rate: 460800
- Version: SL-OPENTHREAD/2.5.3.0

**Flashing with OpenOCD:**
- Commander CLI (Silicon Labs' official tool) **cannot detect the XIAO MG24** because it uses CMSIS-DAP, not J-Link
- Must use OpenOCD with CMSIS-DAP support
- The Arduino-bundled OpenOCD works; the system OpenOCD doesn't have the right driver

**Result:** Flashed bootloader + RCP app. Verified with `universal-silabs-flasher`. **IT WORKS.**

**Emotion:** Pure relief. Then anger that the official firmware doesn't work. Then gratitude for darkxst.

#### The DTR Bug (Nightmare #2)
RCP firmware works on Mac. Plug into RPi, start OTBR... and nothing.

**Root cause:** The XIAO MG24's CMSIS-DAP bridge chip (a tiny SAMD11) requires **DTR (Data Terminal Ready) = True** for serial data to flow from host to device. The HA OTBR add-on's `universal-silabs-flasher` explicitly sets **DTR = False**.

This is an obscure hardware detail that probably affects 50 people in the world. We are now those 50 people.

**The proof:** A Python script that sends a Spinel RESET frame:
```python
s = serial.Serial("/dev/ttyACM0", 460800, timeout=3)
s.dtr = True   # 8 bytes response (Spinel RESET)
s.dtr = False  # 0 bytes response (silence)
```

**Workaround:** `socat` TCP-serial bridge that leaves DTR in its default (True) state:
```bash
socat TCP-LISTEN:9999,reuseaddr,fork FILE:/dev/ttyACM0,b460800,raw,echo=0 &
```

**Content gold:** This story has everything:
- A bug that took hours to find
- AI + human collaboration to debug it (Claude analyzed the protocol, I physically tested cables)
- A clever workaround
- The feeling of solving something nobody has documented before

---

## Day 1: First Matter Success (February 28)

### OTBR Running
With the socat bridge, OTBR finally connects to the XIAO MG24. Thread network is live.

### Matter Commissioning
Both boards (SparkFun Thing Plus + Arduino Nano Matter) commissioned via Home Assistant:
- Pairing code: `34970112332`
- Both detected via BLE, joined Thread network, appeared as Matter devices in HA

**Key discovery:** Matter Server add-on needs `bluetooth_adapter_id: 0` — BLE is disabled by default! Another undocumented gotcha.

**Emotion:** Seeing "SparkFun-1: Occupied, 28.5°C" in Home Assistant for the first time. The Grid-EYE thermal sensor is reading my body heat through Matter over Thread. THIS is the moment.

**The OLED moment:** The tiny OLED display on the SparkFun board shows an 8x8 heatmap with "OCC 31°C" at the bottom. It's crude. It's beautiful. It works.

---

## Day 2: The Bold Pivot — Scrap HAOS, Rebuild Everything (March 1, 2026)

### Why
HAOS (Home Assistant Operating System) is great for end users but terrible for our training platform:
- No desktop environment (can't show dashboard on 7" touchscreen natively)
- No Docker control (HA manages everything, hidden from user)
- The DTR/socat workaround requires SSH add-on hacks
- Can't run Flask dashboard or Playwright on the same device
- Can't automate screenshots properly

### The Decision
Wipe the SD card. Start fresh with **Raspberry Pi OS Desktop** (Debian 13, trixie). Run HA and OTBR as Docker containers we control.

**Emotion:** Scary. Everything works now — do we really want to throw it away and start over? Yes. Because "works for me" isn't good enough for a training guide. It needs to be clean, reproducible, and documented.

### RPi OS Setup
RPi Imager on Mac → flash → boot → SSH.

**Small drama:** The hostname came out as `smart-pi` instead of `smartpi` (RPi Imager added a hyphen). Had to fix it manually. Lesson: always verify what Imager actually wrote.

**Another drama:** `dpkg` was in a broken state (`rpi-chromium-mods` package stuck on interactive prompt). Docker installation blocked until we forced it through:
```bash
sudo DEBIAN_FRONTEND=noninteractive dpkg --configure -a --force-confnew
```
Not something a beginner would know. Needs to be in the troubleshooting section.

### Docker + HA + OTBR
Docker installed in one command. Then the OTBR container image problem:

**The original OTBR image (`connectedhomeip/otbr:latest`) has been REMOVED from Docker Hub.** Just... gone. The official Matter documentation still references it.

Found a replacement: `ghcr.io/ownbee/hass-otbr-docker` — specifically designed for HA Docker setups. Rewrote `docker-compose.yml` with environment variables instead of command-line arguments.

**Content gold:** Official documentation pointing to a deleted Docker image. This is the SiLabs ecosystem experience in a nutshell — things change, docs don't update, beginners get stuck.

Both containers running within 10 minutes of Docker installation. HA at `:8123`, OTBR REST API at `:8081`.

### HA Onboarding (Automated)
Instead of clicking through the HA setup wizard manually, Claude automated the entire 4-step onboarding via REST API:
1. Create user (smartpi/smartpi)
2. Set core config (Budapest, metric)
3. Skip analytics
4. Skip integrations

**Then:** Created a Long-Lived Access Token via WebSocket API for dashboard integration.

**Then:** Playwright screenshots from Mac — but the login form failed because HA uses Web Components with Shadow DOM. Standard CSS selectors don't work.

**Fix:** Keyboard-based input instead of clicking elements:
```javascript
await page.keyboard.type(HA_USER, { delay: 50 });
await page.keyboard.press('Tab');
await page.keyboard.type(HA_PASS, { delay: 50 });
await page.keyboard.press('Enter');
```

Two screenshots captured: login page + dashboard after login.

### Firmware Re-Flash (Validation Test)
All three boards re-flashed from scratch, as if a brand new user were following the guide:

**SparkFun Thing Plus Matter:**
- arduino-cli compile + upload
- 938,252 bytes (59% flash), 138,112 bytes (52% RAM)
- Success

**Arduino Nano Matter:**
- arduino-cli compile + upload
- 945,048 bytes (60% flash), 142,472 bytes (54% RAM)
- Success

**XIAO MG24 RCP:**
- OpenOCD flash bootloader + RCP app
- **BUG FOUND:** OpenOCD can't handle paths with spaces! Our project is in "Slab MM project" — the space breaks the flash command.
- **Fix:** Copy hex files to `/tmp/xiao-rcp/` (space-free path). **Must warn in guide.**
- Verified: `universal-silabs-flasher` confirms `SL-OPENTHREAD/2.5.3.0`
- Success

**Content gold:** The path-with-spaces bug. Exactly the kind of thing that stops a beginner cold. "Error: image.base_address option value ('MM') is not valid" — what does that even mean? It means your folder name has a space in it.

---

## Day 2: Strategy & Vision (March 1, Evening)

### The Bigger Picture Emerges
This isn't just one project. It's a **training creation platform**.

The realization: every guide we create follows the same pattern:
1. Hardware shopping list (with product photos)
2. Step-by-step setup (with screenshots at every step)
3. Firmware (Claude writes it, user uploads it)
4. Integration with HA (Matter commissioning)
5. Visualization (dashboard)
6. Automated testing (Playwright verifies guide works)

Swap the hardware and sensors, keep the template. A **training engine**.

### Future Guides Planned
- xG24 Explorer Kit + PIR motion sensor
- xG24 Dev Kit + accelerometer + Zephyr RTOS
- SparkFun Thing Plus + relay actuator
- Nano Matter + air quality sensor (SGP40)
- Simplicity Studio SDK workflows
- Zephyr RTOS on Silicon Labs (top SiLabs strategic priority)

### Personal Brand Strategy
"The PMM Who Builds IoT with AI" — a positioning that:
- Is compatible with the Silicon Labs role
- Shows genuine product understanding
- Demonstrates ecosystem accessibility
- Creates authentic content (not corporate marketing)
- Builds credibility in the developer community

The narrative arc: **Imposter → Experimenter → Builder → Advocate → Leader**

### Blog Series Planned
7 posts, each standalone but forming a series:
1. Day 0: Setting Up a Raspberry Pi
2. I Can See Heat: First Thermal Sensor Data
3. Matter Over Thread: My Sensors Talk to Home Assistant
4. Building a Real-Time Heatmap Dashboard
5. Thread Mesh Topology: Making the Invisible Visible
6. The AI Co-Pilot: How Claude Code Wrote My Firmware
7. From Zero to Smart Home: The Complete Journey

### The CLAUDE.md Innovation
An AI-readable project file in the repo. When someone clones the project and opens it with Claude Code, the AI immediately understands the project structure, known issues, and workarounds. It's like embedding a support engineer in the repo.

This is a genuinely new concept for open source. **AI-supported open source projects.**

---

## Pricing Research (March 1)

### "Cheapest Thread RCP" Claim
Researched whether XIAO MG24 (~$7.90) is the cheapest Thread Border Router radio on the market.

**Finding:** It's the cheapest **Silicon Labs EFR32MG24-based** Thread RCP. But ESP32-C6 boards exist for ~$3.86-5.20. However, ESP32-C6 uses a different radio (802.15.4 via Espressif, not SiLabs) and has less mature Thread support.

**Nuanced claim:** "Cheapest production-quality SiLabs-based Thread RCP" — accurate and defensible.

---

## Key Bugs & Workarounds Registry

| # | Bug | Impact | Discovery Method | Workaround | Guide Section |
|---|-----|--------|-----------------|------------|---------------|
| 1 | Arduino RCP uses wrong UART (EUSART1 instead of USART0) | Total blocker — RCP doesn't work via USB | Schematic analysis + protocol testing | Use darkxst firmware | RCP Firmware |
| 2 | XIAO CMSIS-DAP bridge requires DTR=True | OTBR can't communicate with RCP | Python serial testing (DTR toggle) | socat TCP-serial bridge | OTBR Setup |
| 3 | Commander can't detect XIAO MG24 | Can't flash via Commander CLI | Testing all debug interfaces | Use OpenOCD with CMSIS-DAP | Flashing Guide |
| 4 | System OpenOCD missing efm32s2 driver | Flash fails with wrong OpenOCD | Comparing OpenOCD versions | Use Arduino-bundled OpenOCD | Flashing Guide |
| 5 | OpenOCD can't handle paths with spaces | Flash fails with cryptic error | Reproduction during guide validation | Copy files to /tmp or space-free path | Flashing Guide |
| 6 | connectedhomeip/otbr:latest removed from Docker Hub | docker pull fails | Building docker-compose.yml | Use ghcr.io/ownbee/hass-otbr-docker | Docker Setup |
| 7 | HA Shadow DOM blocks Playwright selectors | Login automation fails | Playwright test execution | Keyboard-based input | Playwright |
| 8 | RPi Imager adds hyphen to hostname | mDNS name doesn't match expected | SSH connection after first boot | Manual hostname fix | RPi Setup |
| 9 | dpkg broken state (rpi-chromium-mods) | Docker installation blocked | apt install failure | DEBIAN_FRONTEND=noninteractive dpkg --configure | RPi Setup |
| 10 | Matter Server BLE disabled by default | Commissioning can't find boards | BLE scan returns nothing | Set bluetooth_adapter_id: 0 | Matter Setup |
| 11 | socat doesn't survive HAOS reboot | OTBR loses RCP connection | After first reboot | Restart socat, then restart OTBR | OTBR Setup |

---

## Emotional Journey (For Blog Authenticity)

### The Low Points
- **"Am I too stupid for this?"** — When the Arduino RCP didn't work and I couldn't figure out why
- **"The documentation lies"** — When connectedhomeip/otbr:latest was a dead link
- **"This is impossible for beginners"** — Realizing how many undocumented gotchas exist
- **"Should I just buy commercial sensors?"** — Brief moment of doubt

### The High Points
- **"I see heat!"** — First Grid-EYE reading on the OLED
- **"It's talking to Home Assistant"** — First Matter entity appearing in HA
- **"Claude literally wrote my firmware"** — Describing what I wanted in English, getting working C code
- **"The mesh is alive"** — Watching Thread topology form between two nodes
- **"This could help thousands of people"** — Realizing the training platform potential

### The Surprises
- **Qwiic cables are magic** — Click in, works. No wiring errors possible.
- **Matter actually works** — After years of hype, it genuinely auto-discovers devices
- **AI can debug hardware** — Claude analyzing UART mappings from schematics was unexpected
- **Thread self-healing is real** — Unplug a node, mesh reconfigures automatically
- **The hardest part isn't the code** — It's finding the right firmware, right UART, right config. AI writes the code instantly; debugging the toolchain takes hours.

---

## Quotes & Moments (For Content)

### Real Quotes (paraphrased from chat)

> "I work at a chip company but I can't blink an LED on our own products."

> "Claude wrote 385 lines of firmware. I wrote zero. But I knew exactly what I wanted it to do."

> "The official pre-built firmware for this board doesn't work with this board's USB port. How is a beginner supposed to figure that out?"

> "OpenOCD can't handle a space in a folder name. In 2026."

> "The DTR bug affects maybe 50 people in the world. Today I became one of them."

> "I never memorize commands — I just ask Claude. The AI is my external memory."

> "Seeing the OLED light up with a heatmap for the first time — that's when it stopped being a project and became something I need to share."

> "If I can build this, anyone can. That's not a marketing slogan — that's the actual thesis."

---

## Content Mining Guide

### For Blog Post 1 (RPi Setup)
Use: RPi Imager hostname bug, dpkg broken state, Docker install simplicity, "first SSH connection" moment

### For Blog Post 2 (Thermal Sensor)
Use: Qwiic magic, first OLED heatmap moment, Claude writing firmware description, "I see heat!" emotion

### For Blog Post 3 (Matter + Thread)
Use: ENTIRE XIAO RCP story (wrong UART, darkxst discovery, DTR bug, socat workaround), Matter commissioning moment, Thread mesh forming

### For Blog Post 4 (Dashboard)
Use: TBD — dashboard not deployed yet

### For Blog Post 5 (Mesh Topology)
Use: TBD — mesh visualization not built yet

### For Blog Post 6 (AI Co-Pilot)
Use: All Claude interactions, firmware writing workflow, UART debugging story, the "AI can debug hardware" surprise, CLAUDE.md innovation

### For Blog Post 7 (Complete Journey)
Use: Full emotional arc, before/after capabilities, personal brand transformation, open source launch

---

## Status: What's Next

### Immediate (Phase 3c)
- [ ] Plug XIAO MG24 back into RPi
- [ ] Verify OTBR container reconnects (DTR issue may not exist on RPi OS Docker!)
- [ ] Commission SparkFun Thing Plus via Matter
- [ ] Commission Arduino Nano Matter via Matter
- [ ] Verify entities in HA (temperature, occupancy, humidity)

### Then
- [ ] Deploy Flask dashboard to RPi (Phase 4)
- [ ] 7" touchscreen kiosk mode (Phase 5)
- [ ] HA dashboard + automations (Phase 6)
- [ ] Full guide with screenshots (Phase 7)
- [ ] Blog posts + launch (Phase 8)

---

*This document is updated as the project progresses. Every bug, breakthrough, and emotion is recorded for future content creation.*
