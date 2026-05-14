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
3. [Bill of Materials](#3-bill-of-materials)
4. [Design Decisions Explained](#4-design-decisions-explained)
5. [Circuit Assembly Guide](#5-circuit-assembly-guide)
6. [Software Setup (PlatformIO)](#6-software-setup-platformio)
7. [Running the Project](#7-running-the-project)
8. [Configuration & Tuning](#8-configuration--tuning)
9. [System Architecture](#9-system-architecture)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Project Overview

Fixed solar panels lose up to **40% efficiency** as the sun moves across the sky. The HELIOS system continuously realigns a solar panel perpendicular to incoming sunlight using a closed-loop, dual-axis tracking algorithm running on an ESP32.

**Hardware summary for this build:**
- **3V 250mA solar panel** — acts as the tracked energy source and sensing subject
- **USB power bank** — powers the entire electronics system (ESP32 + servos)
- **4× LDR quadrant array** — detects which direction light is coming from
- **2× SG90 servos** — physically move the panel on two axes
- **INA219 sensor** — measures the panel's live voltage, current, and power output
- **1× LED indicator** — glows whenever the panel is detecting usable light

**Key Innovation — Hybrid Logic Deadband:**
Standard LDR trackers suffer from the "Hunting Effect": constant micro-jitter in diffuse light wastes motor energy and negates solar gains. Our algorithm only moves the servos when the LDR differential exceeds a configurable threshold, eliminating jitter in low-contrast or cloudy conditions.

---

## 2. File Structure

```
helios_project/
│
├── platformio.ini          <- PlatformIO project config (board, libraries, port)
│
├── include/
│   └── config.h            <- ALL pin assignments and tuning constants (edit here)
│
├── src/
│   └── main.cpp            <- Main firmware (tracking, LED, servo control, telemetry)
│
└── README.md               <- This file
```

> **Rule:** Never hardcode GPIO numbers or magic numbers in `main.cpp`. All configuration belongs in `include/config.h`.

---

## 3. Bill of Materials

| # | Component | Specification | Qty | Notes |
|---|---|---|---|---|
| 1 | ESP32 DevKit v1 | Dual-core 240 MHz, 4 MB Flash, built-in WiFi/BT | 1 | Any 38-pin ESP32 DevKit works |
| 2 | SG90 Micro Servo | 5V, 180° rotation, ~1.8 kg·cm torque | 2 | One per axis |
| 3 | LDR (GL5528 or similar) | 10–20 kΩ in daylight | 4 | Quadrant array |
| 4 | Resistor – 10 kΩ | 1/4 W, 5% | 4 | Pull-down for each LDR |
| 5 | INA219 Module | I2C, ±3.2 A, 0–26 V | 1 | Current/Voltage/Power monitoring |
| 6 | Solar Panel | **3V, 250mA** | 1 | Small polycrystalline or thin-film panel |
| 7 | USB Power Bank | 5V output, ≥1000 mAh | 1 | Powers all electronics via USB |
| 8 | LED (any colour) | Standard 3mm or 5mm | 1 | Panel light detection indicator |
| 9 | Resistor – 330 Ω | 1/4 W | 1 | Current limiting for LED |
| 10 | Breadboard | 830 tie-points | 1 | Prototyping |
| 11 | Jumper Wires | Male-Male and Male-Female | ~25 | Assorted |
| 12 | Small cardboard cross / baffle | ~2–3 cm tall | 1 | Divides LDR array into 4 optical quadrants |

**Not needed in this build (and why):**
- ~~Li-ion battery pack~~ → replaced by USB power bank
- ~~Voltage regulator (LM7805)~~ → power bank USB provides stable 5V; ESP32 has onboard 3.3V LDO
- ~~Voltage divider resistors (100kΩ / 10kΩ)~~ → not needed (see Section 4.2)
- ~~BMS / charger module~~ → not needed; no external battery

---

## 4. Design Decisions Explained

This section explains three common questions about this specific hardware configuration.

---

### 4.1 Do We Need a Shunt / Is the INA219 Shunt Worth Using?

**Short answer: Yes, keep it — but understand what it measures.**

The INA219 has a built-in 0.1Ω shunt resistor on its module board. It measures current by reading the tiny voltage drop across this shunt. At 250mA max panel current, the voltage drop across the shunt is:

```
V_shunt = 0.1 Ω × 0.250 A = 25 mV
```

The INA219's shunt ADC has 10µV resolution and can read this easily. With `setCalibration_16V_400mA()` (used in the firmware), current resolution is **1 mA**, which is **perfectly adequate** for demonstrating power output differences between tracking and non-tracking configurations.

**The shunt is not optional if you want the INA219 to work.** The shunt is built into the INA219 breakout module — you do not add a separate shunt. Simply wire the module in series with the panel output.

**What the INA219 gives you:**
- Bus Voltage (V) — voltage the panel delivers to the load
- Current (mA) — current flowing out of the panel
- Power (mW) — product of the above; this is what you compare between tests

For this panel (3V, 250mA), expect power readings in the range of **0–750 mW** depending on light conditions and tracking accuracy.

---

### 4.2 Do We Need a Voltage Divider?

**No. Omit it entirely.**

The old 12V panel design needed a voltage divider (100kΩ / 10kΩ) to scale 12V down to below 3.3V before it could be safely read by the ESP32's ADC.

With the 3V panel:
- **Panel open-circuit voltage ≈ 3.6–4V** under full sun (common for nominally-3V panels)
- The ESP32's ADC input is rated for **0–3.3V** maximum

So the panel output is close to the ADC limit. However, **you do not need to measure panel voltage via ADC at all**, because:

1. The **INA219 already measures** the bus voltage via I2C with much better accuracy (4mV resolution vs the ESP32 ADC's ~0.8mV but poor linearity)
2. Adding a voltage divider introduces a current leakage path that would artificially load the panel

**Conclusion:** Leave GPIO36 unconnected. Trust the INA219 for all panel electrical measurements.

> ⚠️ Do NOT directly connect the solar panel positive terminal to any ESP32 GPIO pin without a divider. Under full sun, 3V panels often output 3.8–4.2V open-circuit, which exceeds the ESP32's 3.3V GPIO absolute maximum. The INA219's VIN+ and VIN- pins are rated to 26V and are safe.

---

### 4.3 Power Bank as System Supply

The power bank replaces the Li-ion battery pack and voltage regulator from the original design. Here is how the power flows:

```
Power Bank (5V USB)
        |
        └──> ESP32 micro-USB port
                    |
                    ├──> 3.3V LDO (onboard ESP32)
                    │        └──> LDRs, INA219 VCC, LED
                    │
                    └──> 5V pin (VIN on DevKit)
                             └──> SG90 servo power (red wires)
```

The ESP32 DevKit's onboard 3.3V LDO regulator handles all 3.3V peripherals. The DevKit's VIN pin exposes the raw 5V from USB, which is used to power the servos. This is important — **the servos must be powered from 5V, not 3.3V**.

Most USB power banks supply up to 2A at 5V. Two SG90 servos under load draw at most ~500mA combined. The ESP32 draws ~250mA at peak WiFi use. Total peak current ≈ 750mA, well within any power bank's capability.

---

### 4.4 LED Indicator — How It Works

An LED is wired to GPIO2. The firmware monitors the brightest of the four LDR readings every loop cycle. When it exceeds `LED_ON_THRESHOLD` (default: 120 on the 0–4095 ADC scale), the LED turns on.

This threshold is deliberately set **lower than the tracking threshold** (`DEADBAND_THRESHOLD`: 100 differential, vs `LED_ON_THRESHOLD`: 120 absolute). This means:

- **LED ON** = panel is receiving light — it is detecting the sun or a bright source
- **LED OFF** = panel sees darkness — night mode, servos park

The LED gives an immediate visual confirmation that the sensing system is working during outdoor testing, without needing a laptop or serial monitor open.

GPIO2 also drives the **onboard blue LED** on most ESP32 DevKits, so you get two indicators for the price of one — the onboard LED plus your external LED in series through the 330Ω resistor.

---

## 5. Circuit Assembly Guide

### 5.1 LDR Quadrant Array

Build one voltage divider per LDR:

```
3.3V ──── [LDR] ──┬──── ADC GPIO (34 / 35 / 32 / 33)
                  │
               [10 kΩ]
                  │
                 GND
```

Higher light → lower LDR resistance → higher voltage at ADC pin → higher ADC reading (0–4095).

**Physical arrangement:**

Mount the four LDRs in a 2×2 grid and place a cross-shaped cardboard baffle between them. The baffle must be tall enough (2–3 cm) to cast a shadow on the opposite LDR when the light source is off-axis. Without the baffle, all four LDRs see the same sky and the tracker has no directional information.

```
     ┌──────┬──────┐
     │  TL  │  TR  │   GPIO34    GPIO35
     │      │      │
     ├──────X──────┤   <- Cardboard cross (2-3 cm tall)
     │      │      │
     │  BL  │  BR  │   GPIO32    GPIO33
     └──────┴──────┘
```

**Wiring table:**

| LDR Position | Connect to | GPIO # |
|---|---|---|
| Top-Left | GPIO34 via 10kΩ pull-down | ADC1_CH6 (input-only) |
| Top-Right | GPIO35 via 10kΩ pull-down | ADC1_CH7 (input-only) |
| Bottom-Left | GPIO32 via 10kΩ pull-down | ADC1_CH4 |
| Bottom-Right | GPIO33 via 10kΩ pull-down | ADC1_CH5 |

> GPIO 34, 35, 36 are input-only pins on the ESP32 — they have no internal pull-up/pull-down and cannot be driven as outputs. This makes them ideal for ADC sensing.

---

### 5.2 LED Indicator

```
GPIO2 ──── [330 Ω] ──── LED (+) ──── LED (-) ──── GND
```

The 330Ω resistor limits current to about 8mA at 3.3V output — safe for both the GPIO and a standard LED. Use any colour; green or yellow works well visually for a "panel active" indicator.

> GPIO2 is also connected to the onboard LED on most ESP32 DevKit boards (active HIGH). Both LEDs will light simultaneously.

---

### 5.3 SG90 Servo Motors

Each SG90 has three wires:

| Wire colour | Function | Connect to |
|---|---|---|
| Orange or Yellow | Signal (PWM) | GPIO13 (Azimuth) or GPIO12 (Elevation) |
| Red | Power | 5V rail (ESP32 VIN pin) |
| Brown or Black | Ground | GND |

**Important:** Both servo red wires go to the ESP32's VIN pin (or a dedicated 5V breadboard rail connected to VIN). They must NOT be connected to the 3.3V pin — SG90 servos require 5V and will behave erratically at 3.3V. They also draw enough current to brownout the onboard 3.3V regulator if connected there.

**Mechanical arrangement:**

```
Solar Panel (flat face = sensing surface)
     |
  [Elevation servo] — tilts panel up/down (Y-axis)
     |
  [Azimuth servo]   — rotates whole assembly left/right (X-axis)
     |
  Fixed base
```

Mount the azimuth (horizontal) servo as the base. Attach the elevation (vertical) servo on the rotating arm of the azimuth servo. Mount the solar panel and LDR array together on the elevation servo's horn so they move as one unit.

---

### 5.4 INA219 — Connecting the Solar Panel

The INA219 must be wired **in series** with the solar panel's positive output lead. It measures current flowing from the panel to whatever load is connected.

```
Solar Panel (+) ──> [INA219 VIN+] ──[shunt]──[INA219 VIN-] ──> Load (+)
Solar Panel (−) ──────────────────────────────────────────────> Load (−)
```

For demonstration purposes you can leave VIN- open-circuit (no load). The INA219 will still report panel voltage accurately. Current will read near 0mA, which is expected — a real load (e.g. a small resistor or LED) needs to be connected across VIN- and GND to draw current.

**INA219 I2C connections:**

| INA219 Pin | Connect to |
|---|---|
| VCC | ESP32 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |
| VIN+ | Solar panel (+) |
| VIN- | Load (+) / or open for demo |

Default I2C address: **0x40** (A0 and A1 pins unconnected or grounded).

---

### 5.5 Complete Wiring Reference

| Signal | ESP32 GPIO | Notes |
|---|---|---|
| LDR Top-Left | GPIO 34 | Input-only; 10kΩ to GND |
| LDR Top-Right | GPIO 35 | Input-only; 10kΩ to GND |
| LDR Bottom-Left | GPIO 32 | 10kΩ to GND |
| LDR Bottom-Right | GPIO 33 | 10kΩ to GND |
| Servo Azimuth signal | GPIO 13 | PWM, 50 Hz |
| Servo Elevation signal | GPIO 12 | PWM, 50 Hz |
| INA219 SDA | GPIO 21 | I2C |
| INA219 SCL | GPIO 22 | I2C |
| LED indicator | GPIO 2 | 330Ω in series |
| Servo power (both) | VIN (5V) | From USB power bank via DevKit |
| INA219 VCC | 3.3V | |
| All GND | GND | Single common ground node |
| Solar panel (+) | INA219 VIN+ | In series |
| Solar panel (−) | GND / Load (−) | |

---

## 6. Software Setup (PlatformIO)

### Step 1 — Install Visual Studio Code
Download from: https://code.visualstudio.com/

### Step 2 — Install PlatformIO IDE
1. Open VS Code
2. Open Extensions panel (Ctrl+Shift+X)
3. Search **PlatformIO IDE**
4. Click Install
5. Restart VS Code when prompted

### Step 3 — Install Python 3.8+
Download from: https://www.python.org/downloads/
During install, check **"Add Python to PATH"**

### Step 4 — Open the Project
1. File → Open Folder
2. Select the `helios_project/` folder
3. PlatformIO auto-detects `platformio.ini` and configures everything

### Step 5 — Library Installation
Libraries are declared in `platformio.ini` and install automatically on first build:

```
ESP32Servo       v0.13.0    — Servo control for ESP32
Adafruit INA219  v1.2.3     — Current/Voltage sensor
Adafruit BusIO   v1.16.1    — I2C abstraction (INA219 dependency)
```

No manual installation needed.

---

## 7. Running the Project

### Step 1 — Connect ESP32 to PC
Use a micro-USB cable between the ESP32 DevKit and your computer. The power bank is **not** connected during flashing — use the PC's USB port for this step.

Drivers install automatically. If not:
- CP2102 chip: download from Silicon Labs
- CH340 chip: download from WCH

### Step 2 — Identify the COM Port
- Windows: Device Manager → Ports → look for COMx
- Linux: `ls /dev/ttyUSB*`
- macOS: `ls /dev/cu.*`

If the port is not auto-detected, uncomment and set it in `platformio.ini`:
```ini
upload_port = COM3          ; Windows example
; upload_port = /dev/ttyUSB0  ; Linux example
```

### Step 3 — Build

In VS Code:
- Press **Ctrl+Shift+P** → type `PlatformIO: Build` → Enter
- Or click the **✓** (checkmark) icon in the bottom toolbar

Wait for: `[SUCCESS] Took X.XX seconds`

### Step 4 — Upload

- Press **Ctrl+Shift+P** → type `PlatformIO: Upload` → Enter
- Or click the **→** (right arrow) icon in the bottom toolbar

If upload fails with "A fatal error occurred: Failed to connect":
1. Hold the **BOOT** button on the ESP32
2. Click Upload
3. Release BOOT once "Connecting........" appears in the terminal

### Step 5 — Open Serial Monitor

- Press **Ctrl+Shift+P** → type `PlatformIO: Monitor` → Enter
- Or click the **plug icon** in the bottom toolbar

Baud rate is set to **115200** in `platformio.ini` automatically.

Expected output when light is detected:

```
====================================
  HELIOS - Solar Yield Maximizer
  VIT Control Systems Project
====================================

[SERVO] Initialized at center position (90/90 deg).
[INA219] Sensor ready (calibrated for 16V / 400mA max).
[BOOT] System ready. Entering tracking loop.

------------- HELIOS Telemetry -------------
  LDR Raw  |  TL:1840  TR:1230  BL:1790  BR:1180
  Servos   |  Azimuth:  88 deg   Elevation:  93 deg
  LED      |  Panel light indicator: ON (light detected)
  Solar    |  V: 2.847 V   I: 18.30 mA   P: 52.10 mW
  Tracking |  Incidence ~2.8 deg   Cos-efficiency: 99.9%
--------------------------------------------
```

### Step 6 — Switch to Power Bank for Standalone Operation

Once flashed:
1. Disconnect from PC
2. Connect the USB power bank to the ESP32 micro-USB port
3. The system boots and runs autonomously — no serial monitor needed
4. Watch the LED: it turns on when the panel detects light, and the servos track the light source

---

## 8. Configuration & Tuning

All parameters live in `include/config.h`. Rebuild and re-flash after any change.

### Tuning Table

| Parameter | Default | When to change |
|---|---|---|
| `DEADBAND_THRESHOLD` | 100 | **Raise to 150–200** if servos jitter indoors or in shade. **Lower to 60–80** for sharper tracking in direct sunlight. |
| `LED_ON_THRESHOLD` | 120 | Lower if LED does not turn on under indoor lighting. Raise if LED flickers in shadow. |
| `STEP_DEG` | 1 | Raise to 2–3 for faster tracking. Keep at 1 for smooth demo movement. |
| `NIGHT_THRESHOLD` | 150 | Lower to 80 if system enters dark-mode incorrectly indoors. |
| `NIGHT_SLEEP_MS` | 5000 | 5 seconds is good for demos. Raise to 30000 for outdoor overnight deployment. |
| `SERVO_MIN_DEG` / `SERVO_MAX_DEG` | 10 / 170 | Narrow these if your mechanical mount binds before 10° or 170°. |
| `SERVO_NIGHT_PARK_AZ` | 60 | Set to your East-facing azimuth angle for outdoor deployment. |

### Calibration Procedure

1. Place the system in direct sunlight or under a strong desk lamp
2. Open the Serial Monitor (see Step 5 above)
3. Observe the LDR raw values — when the panel is aimed directly at the light, all four should be roughly equal
4. Cover one LDR quadrant with your hand and confirm the servo moves toward that side
5. Adjust `DEADBAND_THRESHOLD` if the servo is too twitchy (raise) or too sluggish (lower)

---

## 9. System Architecture

```
┌────────────────────────────────────────────────────────┐
│                  3V 250mA SOLAR PANEL                  │
│                  (mounted on pan-tilt)                  │
│                       ↓ light                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │     4x LDR Quadrant Array with cross baffle     │   │
│  └───────────────────┬─────────────────────────────┘   │
│                      │ ADC readings (0–4095, 8x avg)   │
│  ┌───────────────────▼───────────────────────────────┐ │
│  │                ESP32 DevKit v1                     │ │
│  │                                                    │ │
│  │  1. Sample 4 LDRs (8x averaged noise reduction)  │ │
│  │  2. Update LED (light detected indicator)         │ │
│  │  3. Check night condition -> park if dark         │ │
│  │  4. Compute H/V error differentials               │ │
│  │  5. Hybrid Logic deadband gate                    │ │
│  │     |err| > threshold? -> move servo 1 step       │ │
│  │     else               -> hold position           │ │
│  │  6. Clamp servo angles to mechanical limits       │ │
│  │  7. Read INA219 (V, I, P) via I2C                │ │
│  │  8. Print telemetry to Serial @ 115200 baud       │ │
│  └──────┬─────────────────┬───────────────────────────┘ │
│         │                 │                              │
│  [SG90 Azimuth]  [SG90 Elevation]  [LED]  [INA219]     │
│  GPIO13           GPIO12           GPIO2   I2C 21/22    │
│                                                          │
│  Power Bank (5V USB) ──> ESP32 USB ──> VIN ──> Servos  │
│  Solar Panel ──> INA219 VIN+ ──[shunt]──> VIN- ──> Load│
└────────────────────────────────────────────────────────┘
```

### Control Law Summary

```
topAvg   = (LDR_TL + LDR_TR) / 2
botAvg   = (LDR_BL + LDR_BR) / 2
leftAvg  = (LDR_TL + LDR_BL) / 2
rightAvg = (LDR_TR + LDR_BR) / 2

errElevation = topAvg  - botAvg     // + means sun is above panel
errAzimuth   = leftAvg - rightAvg   // + means sun is to the left

IF |errElevation| > DEADBAND_THRESHOLD:
    posElevation ± STEP_DEG          // tilt up or down

IF |errAzimuth| > DEADBAND_THRESHOLD:
    posAzimuth ± STEP_DEG            // rotate left or right

LED on  IF max(TL, TR, BL, BR) >= LED_ON_THRESHOLD
LED off IF max(TL, TR, BL, BR) <  LED_ON_THRESHOLD
```

Power output goal: `P_actual = P_max × cos(θ)` where θ → 0 as tracking improves.

---

## 10. Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| Upload fails: "A fatal error occurred" | ESP32 not in boot mode | Hold BOOT button, click Upload, release when "Connecting..." appears |
| Serial Monitor shows garbled text | Wrong baud rate | Confirm it is set to 115200 |
| Servos jitter constantly | DEADBAND_THRESHOLD too low | Raise to 150–200 in `config.h` |
| Servos do not move at all | Wrong GPIO or no 5V on servo rail | Check GPIO 12/13 signal; confirm servo red wires go to VIN (5V), not 3.3V |
| INA219 not found at boot | I2C wiring issue | Confirm SDA→GPIO21, SCL→GPIO22, VCC→3.3V. Check address is 0x40 |
| INA219 shows 0V / 0mA | No panel connected, or VIN+/VIN- wired backwards | Connect panel to VIN+, load to VIN-. Swap if reversed. |
| LED never turns on | LED wired backwards or GPIO2 issue | Check LED polarity (longer leg = anode = toward GPIO via 330Ω). Swap if needed. |
| LED stays on in complete darkness | LED_ON_THRESHOLD too low | Raise to 200–300 in `config.h` |
| System parks in daylight | NIGHT_THRESHOLD too high | Lower to 80–100 in `config.h` |
| Panel does not track East→West | Azimuth servo polarity inverted | In `hybridLogicControl()` swap the `+STEP_DEG` and `-STEP_DEG` for the Azimuth block |
| Low INA219 current readings | No load across panel output | Connect a 10–47Ω resistor between INA219 VIN- and GND to draw current |

---

*HELIOS Team – VIT School of Electronics Engineering – Control Systems Project Review*
