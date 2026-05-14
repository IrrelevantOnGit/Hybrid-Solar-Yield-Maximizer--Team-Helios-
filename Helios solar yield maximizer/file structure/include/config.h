#pragma once

// ============================================================
//  config.h  –  HELIOS Solar Yield Maximizer
//  All hardware pin assignments and tuning constants
//
//  Hardware revision: 3V 250mA solar panel, power bank supply
// ============================================================

// ─────────────────────────────────────────────
//  GPIO PIN ASSIGNMENTS  (ESP32 DevKit v1)
// ─────────────────────────────────────────────

// LDR Quadrant Array (connect via 10kΩ pull-down resistor to GND)
//  Physical layout when looking at the sensor face:
//  ┌──────┬──────┐
//  │  TL  │  TR  │   (Top-Left, Top-Right)
//  ├──────┼──────┤
//  │  BL  │  BR  │   (Bottom-Left, Bottom-Right)
//  └──────┴──────┘
#define PIN_LDR_TL      34    // GPIO34 – ADC1_CH6  (input only)
#define PIN_LDR_TR      35    // GPIO35 – ADC1_CH7  (input only)
#define PIN_LDR_BL      32    // GPIO32 – ADC1_CH4
#define PIN_LDR_BR      33    // GPIO33 – ADC1_CH5

// Servo Motors
#define PIN_SERVO_AZ    13    // GPIO13 – Azimuth servo  (horizontal / X-axis)
#define PIN_SERVO_EL    12    // GPIO12 – Elevation servo (vertical  / Y-axis)

// INA219 Current/Voltage Sensor (I2C)
#define I2C_SDA         21    // GPIO21 – I2C Data
#define I2C_SCL         22    // GPIO22 – I2C Clock

// Solar Panel Light Detection Indicator LED
//  This LED lights up when the solar panel is detecting usable
//  light (i.e. not in night/dark condition). It acts as a live
//  visual indicator that the panel is receiving illumination.
//  Wire: GPIO2 → 330Ω resistor → LED anode → LED cathode → GND
#define PIN_LED_INDICATOR  2  // GPIO2 – onboard LED also lights up on most DevKits

// NOTE: Voltage divider pin REMOVED.
//  The old 12V panel needed a 100kΩ/10kΩ divider to scale
//  voltage down to ADC-safe levels. The 3V panel output is
//  already within the ESP32's 3.3V ADC range, so you CAN
//  read it directly on GPIO36 if you want raw panel voltage.
//  However since the INA219 already measures V, I and P
//  via I2C, a separate ADC voltage pin adds nothing useful
//  at this scale. It is omitted entirely in this build.

// ─────────────────────────────────────────────
//  SERVO CONFIGURATION
// ─────────────────────────────────────────────
#define SERVO_MIN_US    500   // Pulse width for 0°   (microseconds)
#define SERVO_MAX_US    2400  // Pulse width for 180° (microseconds)

#define SERVO_MIN_DEG   10    // Mechanical travel limit (avoid binding)
#define SERVO_MAX_DEG   170   // Mechanical travel limit (avoid binding)
#define SERVO_CENTER    90    // Default center / home position

// Night park position – face East to catch morning sun
#define SERVO_NIGHT_PARK_AZ   60    // ~East-facing azimuth
#define SERVO_NIGHT_PARK_EL   45    // Tilted down slightly for morning rise

// ─────────────────────────────────────────────
//  HYBRID LOGIC TUNING
// ─────────────────────────────────────────────

// DEADBAND / THRESHOLD (0–4095 ADC scale on ESP32 12-bit ADC)
//  Increase to reduce hunting in cloudy conditions.
//  Decrease for more aggressive / precise tracking in bright sun.
//  Recommended starting value: 80–120
#define DEADBAND_THRESHOLD    100

// Step size (degrees) per control loop iteration
//  Smaller = smoother but slower tracking
//  Recommended: 1–3°
#define STEP_DEG              1

// Night / no-light detection threshold (ADC units, 0–4095)
//  If ALL four LDRs read below this value, the system treats it
//  as darkness: servos park, LED turns off, loop waits.
//  Indoors under artificial light the LDRs typically read
//  300–800; direct sunlight pushes them above 2000.
//  Default 150 works well for outdoor darkness.
#define NIGHT_THRESHOLD       150

// LED brightness threshold – separate from night detection.
//  The LED turns ON whenever the brightest LDR reads above this
//  value, giving a smooth "panel seeing light" indication
//  even before full tracking threshold is crossed.
//  Set equal to or slightly below NIGHT_THRESHOLD.
#define LED_ON_THRESHOLD      120

// ─────────────────────────────────────────────
//  TIMING
// ─────────────────────────────────────────────

// Main loop delay (milliseconds)
//  Controls how often the tracker re-checks LDR values.
//  500 ms is a good balance between responsiveness and motor wear.
#define LOOP_DELAY_MS         500

// Duration to wait (ms) when night/dark is detected before re-checking
//  5 seconds keeps the system responsive indoors during demos.
//  Increase to 30000 for real outdoor overnight deployment.
#define NIGHT_SLEEP_MS        5000
