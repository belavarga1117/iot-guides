#!/bin/bash
# kiosk-autostart.sh — Launch Chromium in kiosk mode on RPi 7" display
#
# This script is called by the autostart desktop entry.
# It waits for the Flask dashboard to be ready, then launches Chromium fullscreen.

DASHBOARD_URL="http://localhost:5000"
MAX_WAIT=60  # seconds to wait for dashboard

echo "[kiosk] Waiting for dashboard at ${DASHBOARD_URL}..."

# Wait for dashboard to be available
for i in $(seq 1 $MAX_WAIT); do
    if curl -s -o /dev/null -w "%{http_code}" "$DASHBOARD_URL" | grep -q "200"; then
        echo "[kiosk] Dashboard is ready!"
        break
    fi
    sleep 1
done

# Hide cursor after 3 seconds of inactivity
unclutter -idle 3 &

# Disable screen blanking
xset s off
xset -dpms
xset s noblank

# Launch Chromium in kiosk mode
chromium-browser \
    --kiosk \
    --incognito \
    --disable-infobars \
    --disable-session-crashed-bubble \
    --disable-restore-session-state \
    --noerrdialogs \
    --check-for-update-interval=31536000 \
    --disable-component-update \
    "$DASHBOARD_URL"
