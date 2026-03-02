#!/bin/bash
# setup-kiosk.sh — One-shot setup for Chromium kiosk mode on RPi 7" touchscreen
#
# Run once after RPi OS is installed:
#   chmod +x setup-kiosk.sh && ./setup-kiosk.sh

set -euo pipefail

echo "=== Smart Home Matter Starter — Kiosk Setup ==="

# Install kiosk dependencies
echo "[1/4] Installing kiosk dependencies..."
sudo apt-get install -y chromium-browser unclutter

# Disable screen blanking
echo "[2/4] Disabling screen blanking..."
sudo raspi-config nonint do_blanking 1 2>/dev/null || true

# Create autostart directory
echo "[3/4] Setting up autostart..."
AUTOSTART_DIR="/home/smartpi/.config/autostart"
mkdir -p "$AUTOSTART_DIR"

cat > "$AUTOSTART_DIR/heatmap-kiosk.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=HeatMap Kiosk
Exec=/home/smartpi/silabs-training/matter-thermal-mesh/rpi-config/kiosk-autostart.sh
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
EOF

# Install Flask dashboard systemd service
echo "[4/4] Installing dashboard service..."
sudo cp /home/smartpi/silabs-training/matter-thermal-mesh/rpi-config/heatmap-dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable heatmap-dashboard.service

echo ""
echo "=== Kiosk Setup Complete ==="
echo ""
echo "On next reboot:"
echo "  1. Docker starts HA + OTBR automatically"
echo "  2. Flask dashboard starts as systemd service"
echo "  3. Chromium launches in kiosk mode on the 7\" display"
echo ""
echo "To test now: sudo systemctl start heatmap-dashboard && bash kiosk-autostart.sh"
