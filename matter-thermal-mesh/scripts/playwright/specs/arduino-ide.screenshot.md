# Arduino IDE Screenshot Plan

Arduino IDE is a native macOS app — Playwright can't control it.
We use macOS `screencapture` commands instead.

## Screenshots to capture (manual flow on Mac)

Run these commands WHILE the Arduino IDE is in the foreground at each step.
The `-l` flag captures a specific window by ID.

### Get Arduino IDE window ID:
```bash
osascript -e 'tell application "Arduino IDE" to get the id of window 1'
```

### Or use interactive capture (click on the window):
```bash
# Click on the Arduino IDE window to capture it
screencapture -w docs/screenshots/arduino-XX-description.png
```

## Screenshot sequence:

1. **Board Manager** — installing Silicon Labs package
   ```bash
   screencapture -w docs/screenshots/arduino-01-board-manager.png
   ```

2. **Board Selection** — Tools > Board > Silicon Labs > [board name]
   ```bash
   screencapture -w docs/screenshots/arduino-02-board-select.png
   ```

3. **Port Selection** — Tools > Port > /dev/cu.usbmodemXXX
   ```bash
   screencapture -w docs/screenshots/arduino-03-port-select.png
   ```

4. **Sketch open** — firmware .ino file loaded
   ```bash
   screencapture -w docs/screenshots/arduino-04-sketch-open.png
   ```

5. **Compile output** — successful compilation
   ```bash
   screencapture -w docs/screenshots/arduino-05-compile-ok.png
   ```

6. **Upload output** — successful upload to board
   ```bash
   screencapture -w docs/screenshots/arduino-06-upload-ok.png
   ```

7. **Serial Monitor** — showing sensor data + QR code
   ```bash
   screencapture -w docs/screenshots/arduino-07-serial-monitor.png
   ```

## Full screenshot list (Arduino IDE):
```
docs/screenshots/
  arduino-01-board-manager.png
  arduino-02-board-select.png
  arduino-03-port-select.png
  arduino-04-sketch-open.png
  arduino-05-compile-ok.png
  arduino-06-upload-ok.png
  arduino-07-serial-monitor.png
```
