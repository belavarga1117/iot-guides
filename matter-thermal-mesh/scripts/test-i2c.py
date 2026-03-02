#!/usr/bin/env python3
"""
test-i2c.py — I2C Bus Scanner for Grid-EYE Debugging

Run on RPi to scan the I2C bus and verify Grid-EYE sensors are visible.
Useful for debugging Qwiic cable connections.

Usage: python3 test-i2c.py

Expected output:
  0x68 — Grid-EYE (default address)
  0x69 — Grid-EYE (alternate address, jumper bridged)
"""

import subprocess
import sys


def scan_i2c(bus=1):
    """Scan I2C bus and return list of detected addresses."""
    try:
        result = subprocess.run(
            ["i2cdetect", "-y", str(bus)],
            capture_output=True,
            text=True,
            check=True,
        )
    except FileNotFoundError:
        print("ERROR: i2cdetect not found.")
        print("Install with: sudo apt-get install i2c-tools")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"ERROR: i2cdetect failed: {e.stderr}")
        sys.exit(1)

    addresses = []
    for line in result.stdout.strip().split("\n")[1:]:  # Skip header
        parts = line.split(":")[1].strip().split() if ":" in line else []
        for part in parts:
            if part != "--" and part != "UU":
                try:
                    addresses.append(int(part, 16))
                except ValueError:
                    continue

    return addresses


KNOWN_DEVICES = {
    0x68: "Grid-EYE AMG8833 (default address)",
    0x69: "Grid-EYE AMG8833 (alternate address — jumper bridged)",
    0x62: "SCD40 CO2 sensor",
    0x70: "TCA9548A I2C Mux (default)",
    0x30: "IS31FL3741 RGB Matrix",
}


def main():
    print("=== Smart Home Matter Starter — I2C Bus Scanner ===")
    print()

    addresses = scan_i2c()

    if not addresses:
        print("No I2C devices found!")
        print()
        print("Check:")
        print("  - Is the Qwiic cable connected properly?")
        print("  - Is I2C enabled? (sudo raspi-config → Interface Options → I2C)")
        print("  - Is the board powered?")
        return

    print(f"Found {len(addresses)} device(s) on I2C bus 1:")
    print()

    grideye_count = 0
    for addr in sorted(addresses):
        name = KNOWN_DEVICES.get(addr, "Unknown device")
        print(f"  0x{addr:02X}  —  {name}")
        if addr in (0x68, 0x69):
            grideye_count += 1

    print()
    print(f"Grid-EYE sensors detected: {grideye_count}")

    if grideye_count == 0:
        print()
        print("No Grid-EYE found. Troubleshooting:")
        print("  1. Check Qwiic cable connection (both ends)")
        print("  2. Make sure the Grid-EYE board has power (LED?)")
        print("  3. Try a different Qwiic cable")
    elif grideye_count == 1:
        print("Tip: For 2 Grid-EYEs, bridge the address jumper on the second one (→ 0x69)")
    else:
        print("Both Grid-EYEs detected. Ready to go!")


if __name__ == "__main__":
    main()
