# HELIOS – Hybrid Logic Solar Yield Maximizer
### Closed-Loop Dual-Axis Solar Tracking System
**Team HELIOS | Control Systems Project Review | School of Electronics Engineering, VIT**

---

## Team Members

| Name | Reg. No. | Role |
|---|---|---|
| Tanuj (Lead) | 24BEC0539 | System Architecture & Hybrid Logic Algorithm |
| Srujan Pratap Powar | 24BVD0077 | Embedded Firmware & IoT Integration |
| Tarun K | 23BEC0277 | Data, Documentation & Literature Survey |
| Praveen Rajan T | 23BEC0421 | Power Systems & Battery Management |
| AL Humd | 24BVD0140 | Mechanical Design & Servo Mounting |

---

## Table of Contents
1. [Project Overview](#1-project-overview)
2. [File Structure](#2-file-structure)
3. [Bill of Materials](#3-bill-of-materials-bom)
4. [Circuit Assembly Guide](#4-circuit-assembly-guide)
5. [Software Setup (PlatformIO)](#5-software-setup-platformio)
6. [Running the Project](#6-running-the-project)
7. [Configuration & Tuning](#7-configuration--tuning)
8. [System Architecture](#8-system-architecture)
9. [Troubleshooting](#9-troubleshooting)
10. [Demo](#Demo)

---

## 1. Project Overview

Fixed solar panels lose up to **40% efficiency** as the sun moves across the sky. The HELIOS system solves this by continuously aligning a solar panel perpendicular to incoming sunlight using a **closed-loop, dual-axis tracking algorithm**.

**Key Innovation – "Hybrid Logic" with Deadband Damping:**
Standard LDR-based trackers suffer from the "Hunting Effect" — constant micro-movements in cloudy or diffuse light that waste motor energy and negate solar gains. Our Hybrid Logic system applies a **threshold-based deadband**: the servo only moves when the LDR differential exceeds a configurable threshold, eliminating jitter entirely in low-contrast conditions.

**Expected Performance:**
- Static panel baseline: ~15–20% yield
- Single-axis tracking: ~31% yield improvement
- This system (dual-axis + hybrid logic): **~38%+ yield improvement** over static

---

## 2. File Structure

```
helios_project/
│
├── platformio.ini          ← PlatformIO project config (board, libraries, port)
│
├── include/
│   └── config.h            ← ALL pin assignments & tuning constants (edit here)
│
├── src/
│   └── main.cpp            ← Main firmware (tracking logic, servo control, telemetry)
│
└── README.md               ← This file
```

> **Important:** Never hardcode GPIO pins or constants in `main.cpp`. All configuration belongs in `include/config.h`.

---

## 3. Bill of Materials (BOM)

| # | Component | Specification | Qty | Notes |
|---|---|---|---|---|
| 1 | ESP32 DevKit v1 | Dual-core 240 MHz, 4 MB Flash, built-in WiFi/BT | 1 | Any 38-pin ESP32 DevKit works |
| 2 | SG90 Micro Servo | 5V, 180° rotation, ~1.8 kg·cm torque | 2 | One per axis (Azimuth + Elevation) |
| 3 | LDR (GL5528) | 10–20 kΩ in daylight | 4 | Quadrant array |
| 4 | Resistor – 10 kΩ | 1/4 W, 5% | 4 | Pull-down for each LDR |
| 5 | INA219 Module | I2C, ±3.2 A current, 0–26 V bus | 1 | Current/Voltage monitoring |
| 6 | Solar Panel | 12 V, 5 W, ~417 mA short-circuit | 1 | Small mono/poly crystalline |
| 7 | Li-ion Battery Pack | 7.4 V (2S), ≥2000 mAh | 1 | Powers servos + ESP32 |
| 8 | Voltage Divider | 100 kΩ + 10 kΩ resistors | 1 | Scales 12 V panel → ADC-safe 1.1 V |
| 9 | LM7805 / AMS1117-3.3 | 5 V regulator (for servo rail) | 1 | Or use a 5V BEC/buck converter |
| 10 | Breadboard (full-size) | 830 tie-points | 1 | Prototyping |
| 11 | Jumper Wires | Male-Male, Male-Female | ~30 | Assorted lengths |
| 12 | USB-to-Serial adapter | CP2102 or CH340 (usually on DevKit) | 1 | For flashing via USB |
| 13 | Small separator / baffle | Cardboard or 3D-printed cross | 1 | Divides LDR array into 4 quadrants |

**Optional:**
- 100 µF electrolytic capacitor across the servo 5 V rail (reduces voltage spikes)
- 3D-printed pan-tilt mount for the servos and panel

---

## 4. Circuit Assembly Guide

### 4.1 LDR Quadrant Array

Build a **voltage divider** for each LDR:

```
3.3V ────┬──── [LDR] ────┬──── ADC Pin (GPIO 34/35/32/33)
         │               │
         └───────────────┤
                         │
                       [10kΩ]
                         │
                        GND
```

> When light increases, LDR resistance drops → voltage at ADC pin rises → higher ADC reading.

**Physical LDR Arrangement:**

Mount the 4 LDRs in a 2×2 grid with a **cross-shaped baffle** (a folded piece of cardboard 2–3 cm tall) between them. This ensures each LDR sees only its own quadrant of the sky:

```
  ┌──────┬──────┐
  │  TL  │  TR  │   GPIO34    GPIO35
  │(34)  │ (35) │
  ├──────┼──────┤  ← Cross baffle (2–3 cm tall)
  │  BL  │  BR  │   GPIO32    GPIO33
  │(32)  │ (33) │
  └──────┴──────┘
```

**Wiring Summary:**

| LDR Position | ADC Pin | GPIO# |
|---|---|---|
| Top-Left | ADC1_CH6 | GPIO 34 |
| Top-Right | ADC1_CH7 | GPIO 35 |
| Bottom-Left | ADC1_CH4 | GPIO 32 |
| Bottom-Right | ADC1_CH5 | GPIO 33 |

> GPIO 34, 35, 36 are **input-only** on ESP32 — perfect for ADC, no pull-ups/downs.

---

### 4.2 Servo Motors (SG90)

Each SG90 has 3 wires:

| SG90 Wire | Colour | Connect To |
|---|---|---|
| Signal | Orange/Yellow | GPIO (13 = Azimuth, 12 = Elevation) |
| Power | Red | 5 V rail (from regulator, NOT 3.3 V) |
| Ground | Brown/Black | Common GND |

> **Warning:** Never power SG90 servos directly from the ESP32's 3.3 V pin — they require 5 V and draw up to 500 mA stall current. Use a 5 V regulator or a dedicated BEC.

**Mechanical Setup:**
- Servo 1 (Azimuth, GPIO 13): Rotates the platform **left/right** (East–West sun tracking)
- Servo 2 (Elevation, GPIO 12): Tilts the platform **up/down** (altitude tracking)
- Mount the panel on a **pan-tilt bracket**: Elevation servo is on the rotating arm of the Azimuth servo

---

### 4.3 INA219 Current/Voltage Sensor

The INA219 is placed **in series** with the solar panel's positive output lead:

```
Solar Panel (+) ──→ [INA219 VIN+] ──[Shunt]──[INA219 VIN-] ──→ Load (+)
Solar Panel (−) ──────────────────────────────────────────────→ Load (−)
```

**I2C Connections:**

| INA219 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

Default I2C address: **0x40** (both A0, A1 pins floating/GND).

---

### 4.4 Voltage Divider (Panel Voltage Monitoring)

To safely read the 12 V panel voltage on the ESP32's 3.3 V-tolerant ADC:

```
Panel (+12V) ──[100kΩ]──┬──[10kΩ]── GND
                         │
                       GPIO36 (VP)
```

Scaling factor: V_panel = V_adc × (100 + 10) / 10 = V_adc × 11

---

### 4.5 Power Supply

```
7.4V Li-ion Pack
        │
        ├──→ LM7805 (5V regulator) ──→ Servo 5V Rail + ESP32 VIN
        │
        └──→ Solar Panel charges the battery via a TP4056/BMS module
```

> Alternatively: use a small 5V 2A buck converter (MP1584/LM2596 module) instead of the LM7805 for better efficiency.

---

### 4.6 Full Wiring Reference Table

| Signal | ESP32 GPIO | Notes |
|---|---|---|
| LDR Top-Left | GPIO 34 | Input-only ADC, 10kΩ pull-down |
| LDR Top-Right | GPIO 35 | Input-only ADC, 10kΩ pull-down |
| LDR Bottom-Left | GPIO 32 | ADC, 10kΩ pull-down |
| LDR Bottom-Right | GPIO 33 | ADC, 10kΩ pull-down |
| Servo Azimuth Signal | GPIO 13 | PWM output |
| Servo Elevation Signal | GPIO 12 | PWM output |
| INA219 SDA | GPIO 21 | I2C |
| INA219 SCL | GPIO 22 | I2C |
| Voltage Divider | GPIO 36 | Input-only ADC (VP) |
| 5V Rail | VIN / 5V Pin | From regulator to ESP32 and servos |
| GND | GND | Common ground for all components |

---

## 5. Software Setup (PlatformIO)

### 5.1 Prerequisites

Install the following **in order**:

**Step 1 – Install Visual Studio Code**
Download from: https://code.visualstudio.com/

**Step 2 – Install PlatformIO IDE Extension**
1. Open VS Code
2. Click the Extensions icon (Ctrl+Shift+X)
3. Search for **"PlatformIO IDE"**
4. Click Install and wait for it to complete
5. Restart VS Code when prompted

**Step 3 – Install Python (required by PlatformIO)**
Download Python 3.8+ from: https://www.python.org/downloads/
During install, check **"Add Python to PATH"**

---

### 5.2 Opening the Project

1. Open VS Code
2. Click **File → Open Folder**
3. Navigate to and select the `helios_project/` folder
4. PlatformIO will auto-detect `platformio.ini` and configure the project

---

### 5.3 Installing Libraries

PlatformIO downloads all required libraries automatically from `platformio.ini`:

```
ESP32Servo       v0.13.0    (Servo motor control for ESP32)
Adafruit INA219  v1.2.3     (Current/Voltage sensor)
Adafruit BusIO   v1.16.1    (I2C/SPI abstraction dependency)
```

To trigger a manual library install/update:
- Click the **PlatformIO icon** (alien head) in the left sidebar
- Go to **Libraries → Install** if needed, or simply build the project (libraries auto-install on first build)

---

## 6. Running the Project

### Step 1 – Connect the ESP32

1. Connect the ESP32 DevKit to your computer via USB
2. The driver should install automatically (CP2102 or CH340)
   - If not: download CP210x driver from Silicon Labs, or CH340 driver from WCH
3. Note the COM port:
   - Windows: Device Manager → Ports (COMx)
   - Linux: `ls /dev/ttyUSB*`
   - macOS: `ls /dev/cu.*`

### Step 2 – Configure the COM Port (if needed)

Open `platformio.ini` and uncomment the appropriate upload_port line:
```ini
; Windows:
upload_port = COM3

; Linux:
upload_port = /dev/ttyUSB0

; macOS:
upload_port = /dev/cu.SLAB_USBtoUART
```

### Step 3 – Build the Firmware

In VS Code with PlatformIO:
- Press **Ctrl+Shift+P** → type `PlatformIO: Build` → Enter
- Or click the **✓ (checkmark)** button in the PlatformIO toolbar at the bottom

Wait for: `[SUCCESS] Took X.XX seconds`

### Step 4 – Upload to ESP32

- Press **Ctrl+Shift+P** → type `PlatformIO: Upload` → Enter
- Or click the **→ (arrow)** button in the PlatformIO toolbar

If upload fails with "A fatal error occurred":
- Hold the **BOOT** button on the ESP32 while clicking Upload
- Release BOOT once "Connecting..." appears in the terminal

### Step 5 – Open Serial Monitor

- Press **Ctrl+Shift+P** → type `PlatformIO: Monitor` → Enter
- Or click the **plug icon** in the PlatformIO toolbar
- Baud rate: **115200** (already set in `platformio.ini`)

You should see:
```
====================================
  HELIOS – Solar Yield Maximizer
  VIT Control Systems Project
====================================

[SERVO] Initialized at center position (90°/90°).
[INA219] Current/Voltage sensor ready.
[BOOT] System ready. Entering tracking loop.

─────────────── HELIOS Telemetry ───────────────
  LDR Raw  |  TL: 2341  TR: 1820  BL: 2290  BR: 1750
  Servos   |  Azimuth:  87°   Elevation:  91°
  Power    |  V: 11.45 V   I: 124.3 mA   P: 1423.1 mW
  Tracking |  Est. Incidence Angle: 3.1°  Cos-Eff: 99.9%
────────────────────────────────────────────────
```

---

## 7. Configuration & Tuning

All tunable parameters are in `include/config.h`. **Do not edit `main.cpp`** for adjustments.

### Key Parameters to Tune in the Field

| Parameter | Default | Effect |
|---|---|---|
| `DEADBAND_THRESHOLD` | 100 | Raise (150–200) in cloudy areas to reduce hunting. Lower (50–80) for precise sunny-day tracking. |
| `STEP_DEG` | 1 | Increase (2–3) for faster tracking at the cost of smoothness. |
| `NIGHT_THRESHOLD` | 150 | Raise if system incorrectly detects night during overcast days. |
| `LOOP_DELAY_MS` | 500 | Increase to reduce motor wear; decrease for faster response. |
| `SERVO_MIN_DEG` / `SERVO_MAX_DEG` | 10 / 170 | Adjust based on your mechanical mount's physical limits to avoid servo binding. |
| `SERVO_NIGHT_PARK_AZ` | 60 | Set to the azimuth angle pointing East for your installation location. |

### Calibration Procedure

1. Place the system in direct sunlight
2. Open the Serial Monitor
3. Observe the LDR raw values — all four should be roughly equal when the panel is perfectly aligned
4. If they are significantly unequal at what you believe is correct alignment, adjust `DEADBAND_THRESHOLD` or check LDR sensor placement

---

## 8. System Architecture

```
┌──────────────────────────────────────────────────────────┐
│                     SOLAR PANEL (12V 5W)                 │
│                          ↓ light                         │
│  ┌──────────────────────────────────────────────────┐    │
│  │       4× LDR Quadrant Array (TL, TR, BL, BR)     │    │
│  └───────────────────┬──────────────────────────────┘    │
│                       │ ADC readings (0–4095)             │
│  ┌────────────────────▼────────────────────────────────┐  │
│  │              ESP32 DevKit v1                         │  │
│  │                                                      │  │
│  │  1. Sample LDRs (8× averaged)                       │  │
│  │  2. Calculate H/V differential errors               │  │
│  │  3. Hybrid Logic:                                   │  │
│  │     if |error| > DEADBAND_THRESHOLD → move servo    │  │
│  │     else → hold position (anti-hunting)             │  │
│  │  4. Clamp to [10°, 170°] servo limits               │  │
│  │  5. Write PWM to SG90 servos                        │  │
│  │  6. Read INA219 (V, I, P)                           │  │
│  │  7. Print telemetry to Serial @ 115200 baud         │  │
│  └────────────────┬──────────────────┬─────────────────┘  │
│                   │                  │                     │
│     ┌─────────────▼──────┐  ┌────────▼──────────┐        │
│     │  SG90 Azimuth      │  │  SG90 Elevation   │        │
│     │  (Horizontal/X)    │  │  (Vertical/Y)     │        │
│     └────────────────────┘  └───────────────────┘        │
│                                                            │
│  INA219 ──I2C──→ ESP32  (monitors panel V, I, P)          │
│  7.4V Li-ion Battery Pack  →  5V Regulator → System       │
└──────────────────────────────────────────────────────────┘
```

### Control Law (Hybrid Logic)

```
topAvg   = (LDR_TL + LDR_TR) / 2
botAvg   = (LDR_BL + LDR_BR) / 2
leftAvg  = (LDR_TL + LDR_BL) / 2
rightAvg = (LDR_TR + LDR_BR) / 2

errElevation = topAvg  - botAvg     // + → sun is above panel normal
errAzimuth   = leftAvg - rightAvg   // + → sun is to the left

IF |errElevation| > DEADBAND_THRESHOLD:
    posElevation ± STEP_DEG

IF |errAzimuth| > DEADBAND_THRESHOLD:
    posAzimuth ± STEP_DEG
```

Power output relationship:
```
P_actual = P_max × cos(θ)
```
Where θ is the incidence angle between sun rays and panel normal. The closed-loop system minimises θ → maximises P_actual.

---

## 9. Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| Upload fails: "A fatal error occurred" | ESP32 not in boot mode | Hold BOOT button during upload |
| Serial Monitor shows garbled text | Wrong baud rate | Set monitor to 115200 |
| Servos jitter constantly | Deadband too low | Increase `DEADBAND_THRESHOLD` to 150–200 |
| Servos don't move at all | Wrong GPIO or power issue | Check GPIO 12/13; verify 5V on servo rail |
| INA219 not found | Wrong I2C address or wiring | Check SDA→21, SCL→22; verify 3.3V to INA219 VCC |
| LDR values all 0 | Pull-down resistors missing | Add 10kΩ from ADC pin to GND |
| LDR values all 4095 | Short circuit or no resistor | Check resistor values and orientation |
| System parks at night in daylight | Night threshold too high | Lower `NIGHT_THRESHOLD` to 80–100 |
| Panel doesn't track East→West | Azimuth servo polarity reversed | Swap the `+=` and `-=` in `hybridLogicControl()` for Azimuth |


## 10. Demo

![](Helios-demonstration.mp4)
---

*HELIOS Team – VIT School of Electronics Engineering – Control Systems Project Review*
