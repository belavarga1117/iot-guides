# Wiring & Connection Diagrams

All diagrams are in Mermaid format. Source files: `docs/diagrams/*.mmd` — PNG renders: `docs/diagrams/*.png`.

---

## System Architecture

![System Architecture](diagrams/system-architecture.png)

<details><summary>Mermaid source</summary>

```mermaid
graph TD
    subgraph RPI["🖥 Raspberry Pi 4 · Docker"]
        HA["🏠 Home Assistant\n:8123"]
        MS["⚙️ Matter Server\n:5580"]
        OTBR["📡 OpenThread Border Router\n:8081"]

        HA <-->|"WebSocket"| MS
        HA <-->|"REST API · Thread config"| OTBR
    end

    XIAO["📻 XIAO MG24 Sense\nThread RCP · USB"]

    OTBR <-->|"spinel · USB serial"| XIAO
    MS -.->|"Matter over Thread"| XIAO

    XIAO <-->|"Thread 802.15.4"| R1
    XIAO <-->|"Thread 802.15.4"| R2
    XIAO <-->|"Thread 802.15.4"| R3

    subgraph N1["Node 1 · Living Room"]
        R1("SparkFun Thing Plus Matter")
        GE1["Grid-EYE AMG8833\n8×8 Thermal Camera"]
        SCD["SCD40\nCO₂ + Temp + Humidity"]
        OLED["Micro OLED\n128×64 Display"]
    end

    subgraph N2["Node 2 · Bedroom"]
        R2("SparkFun Thing Plus Matter")
        GE2["Grid-EYE AMG8833\n8×8 Thermal Camera"]
        DIST["Modulino Distance\nToF Proximity"]
        LED2["IS31FL3741\n9×13 RGB Matrix"]
    end

    subgraph N3["Node 3 · Hallway"]
        R3("Arduino Nano Matter")
        GE3["Grid-EYE AMG8833\n8×8 Thermal Camera"]
        LIGHT["Modulino Light\nAmbient Light"]
        LED3["IS31FL3741\n9×13 RGB Matrix"]
    end

    R1 -.-|"mesh"| R2
    R2 -.-|"mesh"| R3
    R3 -.-|"mesh"| R1

    style RPI fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    style XIAO fill:#ede7f6,stroke:#4527a0,stroke-width:2px
    style N1 fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px
    style N2 fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style N3 fill:#fce4ec,stroke:#c62828,stroke-width:2px
```

</details>

---

## Node 1 — Living Room (SparkFun Thing Plus Matter)

![SparkFun Thing Plus Matter #1 wiring](diagrams/wiring-sparkfun1.png)

<details><summary>Mermaid source</summary>

```mermaid
graph LR
    PC["💻 Computer\nUSB-C"]
    SF1["🖥 SparkFun Thing Plus Matter #1\nLiving Room Sensor"]
    GE["📡 Grid-EYE AMG8833\n8×8 Thermal Camera · 0x69"]
    SCD["🌬 SCD40\nCO₂ + Temp + Humidity · 0x62"]
    OLED["🖥 Micro OLED\n128×64 Display · 0x3D"]

    PC -->|"USB data + power"| SF1
    SF1 -->|"Qwiic I2C"| GE
    GE -->|"Qwiic I2C"| SCD
    SCD -->|"Qwiic I2C"| OLED

    style PC fill:#f5f5f5,stroke:#9e9e9e,stroke-width:1px
    style SF1 fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    style GE fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px
    style SCD fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style OLED fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px
```

</details>

**Cables:** 1× USB-C + 3× Qwiic — zero soldering.

---

## Node 2 — Bedroom (SparkFun Thing Plus Matter #2)

![SparkFun Thing Plus Matter #2 wiring](diagrams/wiring-sparkfun2.png)

<details><summary>Mermaid source</summary>

```mermaid
graph LR
    PC["💻 Computer\nUSB-C"]
    SF2["🖥 SparkFun Thing Plus Matter #2\nBedroom Sensor"]
    LED["💡 IS31FL3741\n9×13 RGB LED Matrix · 0x30"]
    GE["📡 Grid-EYE AMG8833\n8×8 Thermal Camera · 0x69"]
    DIST["📏 Modulino Distance\nToF Proximity Sensor · 0x29"]

    PC -->|"USB data + power"| SF2
    SF2 -->|"Qwiic I2C"| LED
    LED -->|"Qwiic I2C"| GE
    GE -->|"Qwiic I2C"| DIST

    style PC fill:#f5f5f5,stroke:#9e9e9e,stroke-width:1px
    style SF2 fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    style LED fill:#fce4ec,stroke:#c62828,stroke-width:2px
    style GE fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px
    style DIST fill:#fff3e0,stroke:#e65100,stroke-width:2px
```

</details>

**Cables:** 1× USB-C + 2× Qwiic + 1× STEMMA QT — zero soldering.

---

## Node 3 — Hallway (Arduino Nano Matter)

![Arduino Nano Matter wiring](diagrams/wiring-nano-matter.png)

<details><summary>Mermaid source</summary>

```mermaid
graph LR
    PC["💻 Computer\nUSB-C"]
    NM["🖥 Arduino Nano Matter\nHallway Sensor"]
    LED["💡 IS31FL3741\n9×13 RGB LED Matrix · 0x30"]
    GE["📡 Grid-EYE AMG8833\n8×8 Thermal Camera · 0x69"]
    LIGHT["☀️ Modulino Light\nAmbient Light Sensor · 0x53"]

    PC -->|"USB data + power"| NM
    NM -->|"Qwiic I2C"| LED
    LED -->|"Qwiic I2C"| GE
    GE -->|"Qwiic I2C"| LIGHT

    style PC fill:#f5f5f5,stroke:#9e9e9e,stroke-width:1px
    style NM fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    style LED fill:#fce4ec,stroke:#c62828,stroke-width:2px
    style GE fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px
    style LIGHT fill:#fff9c4,stroke:#f57f17,stroke-width:2px
```

</details>

**Cables:** 1× USB-C + 1× Qwiic + 1× STEMMA QT + 1× Qwiic — zero soldering.

---

## Cable Summary (All 3 Nodes + RPi)

| # | Type | From | To |
|---|------|------|----|
| 1 | USB-C → USB-A | XIAO MG24 | RPi USB port |
| 2 | USB-C | SparkFun #1 | USB charger |
| 3 | Qwiic | SparkFun #1 | Grid-EYE #1 |
| 4 | Qwiic | Grid-EYE #1 | SCD40 |
| 5 | Qwiic | SCD40 | Micro OLED |
| 6 | USB-C | SparkFun #2 | USB charger |
| 7 | Qwiic | SparkFun #2 | IS31FL3741 #1 |
| 8 | Qwiic | IS31FL3741 #1 | Grid-EYE #2 |
| 9 | STEMMA QT | Grid-EYE #2 | Modulino Distance |
| 10 | USB-C | Nano Matter | USB charger |
| 11 | Qwiic | Nano Matter | IS31FL3741 #2 |
| 12 | Qwiic | IS31FL3741 #2 | Grid-EYE #3 |
| 13 | STEMMA QT | Grid-EYE #3 | Modulino Light |

**Total: 4× USB-C + 7× Qwiic + 2× STEMMA QT = 13 cables**

---

*Rendered with: `mmdc -i diagram.mmd -o diagram.png -t neutral -b white -w 1200 -s 2`*
