# Flashing RCP Firmware to XIAO MG24

The XIAO MG24 acts as a Radio Co-Processor (RCP) for the Thread Border Router.
It provides the 802.15.4 radio that the RPi needs to communicate with the Thread mesh.

## Prerequisites

- XIAO MG24 board
- USB-C cable
- Arduino IDE with SiLabs Arduino Core installed

## Steps

### 1. Install SiLabs Arduino Core (if not done)

In Arduino IDE:
- Go to **File → Preferences → Additional Boards Manager URLs**
- Add: `https://siliconlabs.github.io/arduino/package_arduinosilabs_index.json`
- Go to **Tools → Board → Boards Manager**
- Search "Silicon Labs" and install **Silicon Labs** board package

### 2. Select Board

- **Tools → Board → Silicon Labs → XIAO MG24**

### 3. Open RCP Sketch

The RCP firmware is included in the SiLabs Arduino Core examples:

- **File → Examples → Silicon Labs → openthread_rcp**

If not visible, check:
- The board package version (needs latest)
- Or find it at: `~/.arduino15/packages/SiliconLabs/hardware/silabs/<version>/libraries/OpenThread/examples/openthread_rcp/`

### 4. Flash

1. Connect XIAO MG24 via USB-C
2. Put it in bootloader mode if needed:
   - Hold BOOT button, press RESET, release BOOT
3. Select the correct port in **Tools → Port**
4. Click **Upload**

### 5. Verify

After flashing:
- The XIAO MG24 should appear as a serial device on the RPi
- Check with: `ls /dev/ttyACM* /dev/ttyUSB*`
- OTBR will communicate with it via the `spinel+hdlc+uart` protocol

### 6. Connect to RPi

- Plug XIAO MG24 into RPi USB port
- Run `setup-rpi.sh` (it auto-detects the serial device)
- OTBR container starts and talks to the RCP

## Troubleshooting

- **No serial device**: Try a different USB cable (data, not charge-only)
- **Permission denied**: `sudo chmod 666 /dev/ttyACM0` or add user to `dialout` group
- **OTBR can't connect**: Check baud rate (460800 is default for SiLabs RCP)
- **Wrong firmware**: Re-flash with the correct `openthread_rcp` example
