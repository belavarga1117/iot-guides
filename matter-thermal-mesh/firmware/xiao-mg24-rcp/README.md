# XIAO MG24 - OpenThread RCP Firmware

Working OpenThread RCP firmware for Seeed Studio XIAO MG24, to be used as a Thread Border Router radio with Home Assistant.

**Verified working:** 2026-03-01, RPi 4, RPi OS Desktop + Docker

## Source

Built by [darkxst/silabs-firmware-builder](https://github.com/darkxst/silabs-firmware-builder) release `20250627`, SDK 2024.6.3.

The original release only provides `.gbl` (Gecko Bootloader) format files. OpenOCD cannot flash `.gbl` — it requires Intel HEX (`.hex`) format. We converted them using Silicon Labs Commander:
```bash
commander gbl parse xiao_mg24_ot_rcp.gbl --app darkxst_ot_rcp_2.5.3_460800.hex
commander gbl parse xiao_mg24_bootloader.gbl --bootloader darkxst_bootloader_2.5.3.hex
```

**Download firmware from this repository, not from darkxst directly.** The darkxst release only has `.gbl` files that need conversion.

## Why custom firmware?

Two issues prevent the stock Arduino SiLabs core RCP from working:

### Issue 1: Wrong UART
The pre-built RCP firmware from the Arduino SiLabs core (`openthread_rcp_xiao_mg24.hex`) does NOT work via USB because it uses a different UART than the one connected to the CMSIS-DAP USB bridge.

- **USART0 (PA8 TX / PA9 RX)** = connected to CMSIS-DAP bridge = USB serial = **THIS firmware uses this**
- **EUSART1 (PC6 TX / PC7 RX)** = external header pins = Arduino `Serial1` = the stock RCP likely uses this

The darkxst firmware correctly targets USART0 on PA8/PA9.

### Issue 2: DTR requirement (CRITICAL for OTBR)
The XIAO MG24's CMSIS-DAP bridge (SAMD11 chip) requires **DTR=True** for serial data to flow from host to device. The HA OTBR add-on's `universal-silabs-flasher` explicitly sets DTR=False, causing communication to fail. A `socat` TCP bridge workaround is required (see OTBR Setup below).

## Files

| File | Description |
|------|-------------|
| `darkxst_bootloader_2.5.3.hex` | Gecko Bootloader (Intel HEX, ready to flash) |
| `darkxst_ot_rcp_2.5.3_460800.hex` | OpenThread RCP application, 460800 baud, no flow control |

## Firmware details

- **Protocol:** Spinel (OpenThread RCP)
- **Version:** `SL-OPENTHREAD/2.5.3.0_GitHub-1fceb225b; EFR32; Jun 27 2025 03:49:21`
- **Baud rate:** 460800
- **Flow control:** None
- **UART:** USART0 - PA8 (TX) / PA9 (RX)
- **Chip:** EFR32MG24B220F1536IM48

## Flashing instructions

See the main [SETUP-GUIDE.md](../../SETUP-GUIDE.md#part-3-flash-xiao-mg24-as-thread-radio) Part 3 for step-by-step instructions with screenshots.

**Quick reference:**

### Requirements
- XIAO MG24 connected to computer via USB
- Arduino IDE with SiLabs board support installed (provides OpenOCD)

### Step 1: Burn Bootloader (Arduino IDE GUI)
1. Select **Seeed Studio XIAO MG24** as board
2. **Tools** → **Programmer** → **OpenOCD**
3. **Tools** → **Burn Bootloader**

### Step 2: Flash RCP firmware (Terminal)
```bash
OPENOCD="$HOME/Library/Arduino15/packages/SiliconLabs/tools/openocd/0.12.0-arduino1-static/bin/openocd"
SCRIPTS="$HOME/Library/Arduino15/packages/SiliconLabs/tools/openocd/0.12.0-arduino1-static/share/openocd/scripts/"

"$OPENOCD" -s "$SCRIPTS" \
  -f interface/cmsis-dap.cfg \
  -c "transport select swd" \
  -f target/efm32s2_g23.cfg \
  -c "init" -c "reset halt" \
  -c "flash write_image erase darkxst_ot_rcp_2.5.3_460800.hex" \
  -c "verify_image darkxst_ot_rcp_2.5.3_460800.hex" \
  -c "reset run" -c "exit"
```

### Step 3: Verify
```bash
pip3 install universal-silabs-flasher
universal-silabs-flasher --device /dev/cu.usbmodemXXX --probe-methods "spinel:460800" probe
```

Expected output:
```
Detected ApplicationType.SPINEL, version 'SL-OPENTHREAD/2.5.3.0_GitHub-1fceb225b' at 460800 baudrate
```

## OTBR Setup (Home Assistant)

### The DTR problem and socat workaround

The XIAO MG24's CMSIS-DAP bridge requires DTR=True for bidirectional serial communication. The HA OTBR add-on sets DTR=False, breaking communication. The workaround is a `socat` TCP-serial bridge running in the SSH add-on container.

### Step 1: Install socat in SSH add-on
SSH into HAOS and run:
```bash
apk add socat
```

### Step 2: Install Python3 in SSH add-on
In the SSH add-on configuration (Settings > Apps > Terminal & SSH > Configuration), add `python3` and `py3-pip` to the `apks` list, then restart.

Then install pyserial:
```bash
pip3 install --break-system-packages pyserial
```

### Step 3: Start the socat TCP-serial bridge
```bash
socat TCP-LISTEN:9999,reuseaddr,fork FILE:/dev/ttyACM0,b460800,raw,echo=0 &
```
This keeps DTR in its default state (True) while exposing the serial port as a TCP socket on port 9999.

### Step 4: Configure OTBR add-on
In the OTBR add-on configuration set:
- **Device:** `/dev/serial/by-id/usb-Seeed_Studio_...` (standard serial device)
- **Baudrate:** `460800`
- **Flow control:** `false`
- **network_device:** `172.30.33.0:9999` (SSH add-on IP on Docker network)
- **otbr_log_level:** `notice` (or `debug` for troubleshooting)

Or via API:
```bash
curl -s -X POST http://supervisor/addons/core_openthread_border_router/options \
  -H "Authorization: Bearer $SUPERVISOR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"options":{"device":"/dev/serial/by-id/usb-Seeed_Studio_Seeed_Studio_XIAO_MG24__Sense__CMSIS-DAP_9AD1F1E2-if02","baudrate":"460800","flow_control":false,"otbr_log_level":"notice","firewall":true,"nat64":false,"network_device":"172.30.33.0:9999"}}'
```

### Step 5: Start OTBR
Start the OTBR add-on. It should connect via the TCP bridge.

**Important:** The socat bridge must be running BEFORE the OTBR add-on starts. After HAOS reboot, you need to restart socat manually (or set up autostart).

### Verifying OTBR is working
Check the logs:
```bash
ha apps logs core_openthread_border_router -n 30
```
You should see Spinel frames being sent and received, MLE advertisements, and no errors.

## Troubleshooting

### OTBR fails with "TimeoutError" in migrate_otbr_settings.py
This is the DTR issue. Make sure the socat bridge is running and `network_device` is configured. See "OTBR Setup" above.

### Pre-built Arduino RCP doesn't work
The `openthread_rcp_xiao_mg24.hex` from the Arduino SiLabs core uses the wrong UART. Use this darkxst firmware instead.

### universal-silabs-flasher can't communicate
Make sure you're using 460800 baud, not 115200.

### Verifying DTR is the issue
Run this Python test on the RPi SSH:
```python
import serial, time, struct

def crc16_x25(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0x8408
            else:
                crc >>= 1
    return crc ^ 0xFFFF

def make_frame(payload):
    crc = crc16_x25(payload)
    return b"\x7e" + payload + struct.pack("<H", crc) + b"\x7e"

s = serial.Serial("/dev/ttyACM0", 460800, timeout=3)
s.dtr = True  # Change to False to reproduce the bug
s.rts = True
time.sleep(0.5)
s.reset_input_buffer()
s.write(make_frame(bytes([0x80, 0x01])))
time.sleep(2)
data = s.read(512)
print("Response (%d bytes): %s" % (len(data), data.hex() if data else "EMPTY"))
# With DTR=True: you get 8 bytes (Spinel RESET response)
# With DTR=False: you get 0 bytes (no response)
s.close()
```

### socat bridge stops after HAOS reboot
The socat process doesn't survive reboots. Run it again after each reboot:
```bash
socat TCP-LISTEN:9999,reuseaddr,fork FILE:/dev/ttyACM0,b460800,raw,echo=0 &
```
Then restart the OTBR add-on.
