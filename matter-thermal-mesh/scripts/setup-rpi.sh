#!/bin/bash
# setup-rpi.sh — Raspberry Pi OS Setup for Smart Home Matter Starter
#
# Run on RPi 4 after flashing Raspberry Pi OS (64-bit Desktop).
# Installs: Docker, Home Assistant (container), OTBR, Python dashboard deps, kiosk mode.
#
# Prerequisites:
#   - Fresh Raspberry Pi OS (64-bit with Desktop)
#   - SSH access: ssh smartpi@smartpi.local
#   - XIAO MG24 with RCP firmware plugged into USB
#
# Usage:
#   chmod +x setup-rpi.sh && ./setup-rpi.sh

set -euo pipefail

echo "========================================="
echo "  Smart Home Matter Starter — RPi Setup"
echo "  Training Platform v1.0"
echo "========================================="
echo ""

# ============================================================
# [1/8] System Update
# ============================================================
echo "[1/8] Updating system packages..."
sudo apt-get update && sudo apt-get upgrade -y

# ============================================================
# [2/8] Install Docker
# ============================================================
echo "[2/8] Installing Docker..."
if ! command -v docker &> /dev/null; then
    curl -fsSL https://get.docker.com | sh
    sudo usermod -aG docker "$USER"
    echo "Docker installed. Logging out to apply group changes..."
    # Apply docker group without logout
    newgrp docker << 'DOCKEREOF'
    echo "Docker group applied."
DOCKEREOF
else
    echo "Docker already installed."
fi

# Verify Docker
docker --version || { echo "ERROR: Docker not available. Log out and back in, then re-run."; exit 1; }

# Install Docker Compose plugin if not present
if ! docker compose version &> /dev/null; then
    echo "Installing Docker Compose plugin..."
    sudo apt-get install -y docker-compose-plugin
fi

# ============================================================
# [3/8] Detect XIAO MG24 RCP
# ============================================================
echo "[3/8] Detecting XIAO MG24 RCP radio..."
XIAO_DEV=""
for dev in /dev/ttyACM* /dev/ttyUSB*; do
    if [ -e "$dev" ]; then
        XIAO_DEV="$dev"
        echo "Found serial device: $XIAO_DEV"
        break
    fi
done

if [ -z "$XIAO_DEV" ]; then
    echo ""
    echo "WARNING: No serial device found!"
    echo "Make sure the XIAO MG24 with RCP firmware is plugged into USB."
    echo "Expected: /dev/ttyACM0"
    echo ""
    read -p "Continue anyway with /dev/ttyACM0? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
    XIAO_DEV="/dev/ttyACM0"
fi

# ============================================================
# [4/8] Start Home Assistant + OTBR with Docker Compose
# ============================================================
echo "[4/8] Starting Home Assistant + OTBR..."

# Create docker-compose directory
COMPOSE_DIR="/home/smartpi/silabs-training/matter-thermal-mesh/rpi-config"
if [ ! -f "$COMPOSE_DIR/docker-compose.yml" ]; then
    echo "ERROR: docker-compose.yml not found at $COMPOSE_DIR"
    echo "Copy the project to /home/smartpi/silabs-training/matter-thermal-mesh/ first:"
    echo "  scp -r silabs-training/matter-thermal-mesh/ smartpi@smartpi.local:~/"
    exit 1
fi

cd "$COMPOSE_DIR"
docker compose up -d

echo ""
echo "Home Assistant starting at http://smartpi.local:8123"
echo "OTBR REST API at http://localhost:8081"
echo "First boot takes 3-5 minutes..."
echo ""

# Wait for HA to be ready
echo "Waiting for Home Assistant to start..."
for i in $(seq 1 60); do
    if curl -s -o /dev/null -w "%{http_code}" http://localhost:8123 2>/dev/null | grep -qE "200|401"; then
        echo "Home Assistant is ready!"
        break
    fi
    printf "."
    sleep 5
done
echo ""

# ============================================================
# [5/8] Install Python for Dashboard
# ============================================================
echo "[5/8] Installing Python dependencies..."
sudo apt-get install -y python3-pip python3-venv

DASH_DIR="/home/smartpi/silabs-training/matter-thermal-mesh/dashboard/backend"
if [ -d "$DASH_DIR" ]; then
    cd "$DASH_DIR"
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    deactivate
    echo "Dashboard Python venv created at $DASH_DIR/venv"
else
    echo "Dashboard directory not found — skip pip install"
fi

# ============================================================
# [6/8] Configure 7" Touchscreen Display
# ============================================================
echo "[6/8] Configuring 7\" touchscreen display..."

# The official RPi 7" touchscreen works out of the box with RPi OS Desktop.
# Just disable screen blanking for always-on dashboard.
sudo raspi-config nonint do_blanking 1 2>/dev/null || true

# ============================================================
# [7/8] Install Kiosk Dependencies
# ============================================================
echo "[7/8] Installing kiosk mode + screenshot dependencies..."
sudo apt-get install -y chromium-browser unclutter scrot

# ============================================================
# [8/8] Summary
# ============================================================
echo ""
echo "========================================="
echo "  Setup Complete!"
echo "========================================="
echo ""
echo "Services:"
echo "  Home Assistant:  http://smartpi.local:8123"
echo "  OTBR REST API:   http://localhost:8081"
echo "  Dashboard:       http://smartpi.local:5000 (after starting)"
echo ""
echo "RCP Device: $XIAO_DEV"
echo ""
echo "Next steps:"
echo "  1. Open http://smartpi.local:8123 in a browser"
echo "  2. Complete HA onboarding (user: smartpi, pass: smartpi)"
echo "  3. Add Matter Server integration"
echo "  4. Add OpenThread Border Router integration"
echo "  5. Commission your sensor boards (pairing code: 34970112332)"
echo "  6. Create a Long-Lived Access Token in HA"
echo "  7. Start dashboard: cd ~/silabs-training/matter-thermal-mesh/dashboard/backend && source venv/bin/activate && HA_TOKEN=xxx python app.py"
echo "  8. Set up kiosk: cd ~/silabs-training/matter-thermal-mesh/rpi-config && ./setup-kiosk.sh"
echo ""
echo "========================================="
