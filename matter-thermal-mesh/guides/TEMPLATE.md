# [Project Name] — Setup Guide

> **Difficulty:** Beginner | **Time:** ~2 hours | **Last verified:** YYYY-MM-DD
> **Tested on:** macOS (see notes for Windows differences)

## What You'll Build

[1-2 sentences describing the finished product]

<!-- USER PHOTO: hero-complete-setup.jpg — Complete assembled setup -->
![Finished Result](../docs/images/user/hero-complete-setup.jpg)

## Hardware Shopping List

| Item | Photo | Price | Link |
|------|-------|-------|------|
| Raspberry Pi 4 (4GB+) | ![RPi4](../docs/images/products/raspberry-pi-4.jpg) | ~$55 | [raspberrypi.com](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/) |
| RPi 7" Touchscreen | ![Display](../docs/images/products/rpi-7inch-touchscreen.jpg) | ~$60 | [raspberrypi.com](https://www.raspberrypi.com/products/raspberry-pi-touch-display/) |
| Seeed XIAO MG24 (Sense) | ![XIAO](../docs/images/products/xiao-mg24-sense.jpg) | ~$10 | [seeedstudio.com](https://www.seeedstudio.com/Seeed-XIAO-MG24-Sense-p-6248.html) |
| [DevKit Name] | ![DevKit](../docs/images/products/[devkit-image].jpg) | ~$XX | [link] |
| [Sensor Name] | ![Sensor](../docs/images/products/[sensor-image].jpg) | ~$XX | [link] |
| Qwiic Cable Kit | ![Qwiic](../docs/images/products/sparkfun-qwiic-cables.jpg) | ~$8 | [sparkfun.com](https://www.sparkfun.com/products/15081) |
| USB-C Cable | - | ~$5 | - |
| microSD Card (32GB+) | - | ~$10 | - |

**Total: ~$XXX**

## Prerequisites

Before starting, make sure you have:
- [ ] Raspberry Pi OS installed and SSH working (see [Base Setup Guide](../SETUP-GUIDE.md#part-1))
- [ ] Docker + Home Assistant running (see [Base Setup Guide](../SETUP-GUIDE.md#part-2))
- [ ] Arduino IDE installed on your computer

## Step 1: Flash the Firmware

**Goal:** Get [sensor name] data flowing from [devkit name]
**Time:** ~15 minutes

### 1.1 Open Arduino IDE

Open Arduino IDE on your computer.

### 1.2 Install the Board Package

Go to **Tools > Board > Boards Manager** and search for `Silicon Labs`:

![Board Manager](../docs/screenshots/arduino-01-board-manager.png)

Install **Silicon Labs** (version X.X.X or later).

### 1.3 Select Your Board

Go to **Tools > Board > Silicon Labs** and select **[Board Name]**.

![Board Selection](../docs/screenshots/arduino-02-board-select.png)

### 1.4 Select Port

Go to **Tools > Port** and select the serial port for your board.

![Port Selection](../docs/screenshots/arduino-03-port-select.png)

> **Windows note:** The port will show as `COMx` instead of `/dev/cu.usbmodemXXX`.
> Open Device Manager to find the correct COM port.

### 1.5 Open the Firmware Sketch

Open the file: `firmware/[board-name]/[board-name].ino`

![Sketch Open](../docs/screenshots/arduino-04-sketch-open.png)

### 1.6 Upload

Click the **Upload** button (right arrow icon).

![Upload Success](../docs/screenshots/arduino-06-upload-ok.png)

**Verify:** Open **Tools > Serial Monitor** at 115200 baud. You should see:
```
=== [Project Name] — [Node Name] ===
Grid-EYE (0x68)... OK
Matter device initialized
```

![Serial Monitor](../docs/screenshots/arduino-07-serial-monitor.png)

## Step 2: Commission to Home Assistant

**Goal:** Your sensor appears in Home Assistant
**Time:** ~10 minutes

### 2.1 Get the Pairing Code

In the Serial Monitor, find the line:
```
Manual pairing code: 34970112332
```

### 2.2 Add the Device in HA

1. Go to **Settings > Devices & Services > Matter**
2. Click **Add Device**
3. Enter the pairing code: `34970112332`
4. Wait for commissioning (~30 seconds)

![Matter Commissioning](../docs/screenshots/09-ha-commissioning.png)

**Verify:** The device appears under **Matter** integration with sensor entities.

![Entity List](../docs/screenshots/10-ha-entities-list.png)

## Step 3: [Next Step]

**Goal:** [What this achieves]
**Time:** ~X minutes

[Step instructions...]

**Verify:** [How to check it worked]

## Step N: Verify on Dashboard

<!-- USER PHOTO: kiosk-display-live.jpg — 7" display showing live dashboard -->
![Kiosk Display](../docs/images/user/kiosk-display-live.jpg)

## Troubleshooting

### [Common Issue 1]
**Symptom:** [What you see]
**Cause:** [Why it happens]
**Fix:** [How to fix it]

### [Common Issue 2]
**Symptom:** [What you see]
**Cause:** [Why it happens]
**Fix:** [How to fix it]

### SSH connection refused (Windows)

**Symptom:** `ssh: connect to host smartpi.local port 22: Connection refused`
**Cause:** Windows may not resolve `.local` mDNS hostnames by default
**Fix:** Install [Bonjour Print Services](https://support.apple.com/kb/DL999) or use the RPi's IP address directly. Find it via your router's admin page or with `arp -a` on your network.

## Platform Notes

> This guide was tested on **macOS**. Here are the key differences for Windows users:

| Step | macOS | Windows |
|------|-------|---------|
| SSH | `ssh smartpi@smartpi.local` (built-in Terminal) | Use PowerShell, PuTTY, or Windows Terminal |
| Serial ports | `/dev/cu.usbmodemXXX` | `COMx` (check Device Manager) |
| SCP file copy | `scp -r folder/ smartpi@smartpi.local:~/` | Use WinSCP or `scp` in PowerShell |
| mDNS (.local) | Works out of the box | May need Bonjour installed |
| Arduino IDE | Same | Same |
| Playwright | Same | Same |

## What's Next?

- [ ] Add more sensors to the mesh
- [ ] Create HA automations
- [ ] Build a custom dashboard

---

*Guide created with the [Smart Home Matter Starter](../README.md). Verified on [date].*
