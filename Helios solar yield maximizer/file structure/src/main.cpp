/*
 * ============================================================
 *  HYBRID LOGIC SOLAR YIELD MAXIMIZER
 *  Closed-Loop Dual-Axis Solar Tracking System
 *  Team HELIOS – VIT, School of Electronics Engineering
 *  Control Systems Project
 * ============================================================
 *
 *  Hardware:
 *    - ESP32 DevKit v1 (Dual-Core 240 MHz)
 *    - 2x SG90 Servo Motors  (Azimuth / Elevation axes)
 *    - 4x LDR (Light Dependent Resistors) – quadrant array
 *    - INA219 Current/Voltage Sensor (I2C)
 *    - 3V 250mA Solar Panel (demonstration / sensing element)
 *    - USB Power Bank (system power supply via ESP32 USB)
 *    - 1x LED + 330 ohm resistor (panel light detection indicator)
 *
 *  Power architecture:
 *    Power bank → USB → ESP32 → 3.3V rail (LDRs, INA219, LED)
 *                             → 5V  pin   (SG90 servos via VIN)
 *    Solar panel → INA219 shunt → monitored output
 *    No external battery. No voltage divider needed.
 *
 *  Pin Map (see include/config.h for full list)
 * ============================================================
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>
#include "config.h"

// ─────────────────────────────────────────────
//  Objects
// ─────────────────────────────────────────────
Servo   servoAzimuth;      // Horizontal (X-axis)  <- SG90
Servo   servoElevation;    // Vertical   (Y-axis)  <- SG90
Adafruit_INA219 ina219;

// ─────────────────────────────────────────────
//  State
// ─────────────────────────────────────────────
int     posAzimuth   = SERVO_CENTER;
int     posElevation = SERVO_CENTER;

float   panelVoltage = 0.0f;
float   panelCurrent = 0.0f;
float   panelPower   = 0.0f;

bool    ina219Ready  = false;

// ─────────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────────
void    initServos();
void    initSensors();
void    sampleLDRs(int &topLeft, int &topRight, int &botLeft, int &botRight);
void    hybridLogicControl(int tl, int tr, int bl, int br);
void    readPowerSensor();
void    updateLED(int tl, int tr, int bl, int br);
void    printTelemetry(int tl, int tr, int bl, int br);
void    clampServo(int &angle, int minVal, int maxVal);
bool    isNightCondition(int tl, int tr, int bl, int br);
void    returnToNightPark();

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n===================================="));
    Serial.println(F("  HELIOS - Solar Yield Maximizer   "));
    Serial.println(F("  VIT Control Systems Project       "));
    Serial.println(F("====================================\n"));

    // LED indicator pin
    pinMode(PIN_LED_INDICATOR, OUTPUT);
    digitalWrite(PIN_LED_INDICATOR, LOW);

    Wire.begin(I2C_SDA, I2C_SCL);
    initServos();
    initSensors();

    Serial.println(F("[BOOT] System ready. Entering tracking loop.\n"));
}

// ─────────────────────────────────────────────
//  MAIN LOOP
// ─────────────────────────────────────────────
void loop() {
    int tl, tr, bl, br;

    // 1. Read all four LDRs
    sampleLDRs(tl, tr, bl, br);

    // 2. Update LED indicator (independent of tracking threshold)
    updateLED(tl, tr, bl, br);

    // 3. Night/darkness detection -> park servos and wait
    if (isNightCondition(tl, tr, bl, br)) {
        Serial.println(F("[DARK] Low light detected - parking servos."));
        returnToNightPark();
        delay(NIGHT_SLEEP_MS);
        return;
    }

    // 4. Hybrid logic tracking (threshold-damped)
    hybridLogicControl(tl, tr, bl, br);

    // 5. Read power sensor (INA219)
    readPowerSensor();

    // 6. Serial telemetry
    printTelemetry(tl, tr, bl, br);

    delay(LOOP_DELAY_MS);
}

// ─────────────────────────────────────────────
//  INIT SERVOS
// ─────────────────────────────────────────────
void initServos() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);

    servoAzimuth.setPeriodHertz(50);
    servoElevation.setPeriodHertz(50);

    servoAzimuth.attach(PIN_SERVO_AZ, SERVO_MIN_US, SERVO_MAX_US);
    servoElevation.attach(PIN_SERVO_EL, SERVO_MIN_US, SERVO_MAX_US);

    servoAzimuth.write(SERVO_CENTER);
    servoElevation.write(SERVO_CENTER);
    delay(800);

    Serial.println(F("[SERVO] Initialized at center position (90/90 deg)."));
}

// ─────────────────────────────────────────────
//  INIT SENSORS
// ─────────────────────────────────────────────
void initSensors() {
    // INA219 calibration for 3V 250mA panel:
    //   setCalibration_16V_400mA gives 1mA resolution up to 400mA
    //   which comfortably covers our 250mA max panel current.
    if (ina219.begin()) {
        ina219.setCalibration_16V_400mA();
        ina219Ready = true;
        Serial.println(F("[INA219] Sensor ready (calibrated for 16V / 400mA max)."));
    } else {
        Serial.println(F("[INA219] WARNING - Sensor not found. Power monitoring disabled."));
    }
}

// ─────────────────────────────────────────────
//  SAMPLE LDRs
//  Circuit: 3.3V -- LDR -- ADC pin -- 10kOhm -- GND
//  Higher ADC reading = brighter light hitting that LDR
//  8x averaging reduces ESP32 ADC noise significantly
// ─────────────────────────────────────────────
void sampleLDRs(int &tl, int &tr, int &bl, int &br) {
    const int N = 8;
    long sumTL = 0, sumTR = 0, sumBL = 0, sumBR = 0;

    for (int i = 0; i < N; i++) {
        sumTL += analogRead(PIN_LDR_TL);
        sumTR += analogRead(PIN_LDR_TR);
        sumBL += analogRead(PIN_LDR_BL);
        sumBR += analogRead(PIN_LDR_BR);
        delayMicroseconds(200);
    }

    tl = (int)(sumTL / N);
    tr = (int)(sumTR / N);
    bl = (int)(sumBL / N);
    br = (int)(sumBR / N);
}

// ─────────────────────────────────────────────
//  LED INDICATOR
//  Turns on whenever the brightest LDR quadrant
//  reads above LED_ON_THRESHOLD, indicating the
//  solar panel is receiving usable light.
//  This is independent of the tracking deadband.
// ─────────────────────────────────────────────
void updateLED(int tl, int tr, int bl, int br) {
    int maxReading = max(max(tl, tr), max(bl, br));
    if (maxReading >= LED_ON_THRESHOLD) {
        digitalWrite(PIN_LED_INDICATOR, HIGH);
    } else {
        digitalWrite(PIN_LED_INDICATOR, LOW);
    }
}

// ─────────────────────────────────────────────
//  HYBRID LOGIC CONTROL
//
//  Hunting Effect mitigation:
//    Movement only occurs when the differential
//    between opposing LDR pairs exceeds DEADBAND_THRESHOLD.
//    This prevents jitter in diffuse / overcast conditions.
//
//  Control law:
//    errElevation = topAvg  - botAvg   (+ = sun above, tilt up)
//    errAzimuth   = leftAvg - rightAvg (+ = sun left, rotate left)
//    if |err| > DEADBAND_THRESHOLD -> step servo by STEP_DEG
// ─────────────────────────────────────────────
void hybridLogicControl(int tl, int tr, int bl, int br) {
    int topAvg   = (tl + tr) / 2;
    int botAvg   = (bl + br) / 2;
    int leftAvg  = (tl + bl) / 2;
    int rightAvg = (tr + br) / 2;

    int errElevation = topAvg  - botAvg;
    int errAzimuth   = leftAvg - rightAvg;

    // --- Elevation (Y-axis) ---
    if (abs(errElevation) > DEADBAND_THRESHOLD) {
        posElevation += (errElevation > 0) ? STEP_DEG : -STEP_DEG;
        clampServo(posElevation, SERVO_MIN_DEG, SERVO_MAX_DEG);
        servoElevation.write(posElevation);
    }

    // --- Azimuth (X-axis) ---
    if (abs(errAzimuth) > DEADBAND_THRESHOLD) {
        posAzimuth += (errAzimuth > 0) ? -STEP_DEG : STEP_DEG;
        clampServo(posAzimuth, SERVO_MIN_DEG, SERVO_MAX_DEG);
        servoAzimuth.write(posAzimuth);
    }
}

// ─────────────────────────────────────────────
//  READ INA219 POWER SENSOR
// ─────────────────────────────────────────────
void readPowerSensor() {
    if (!ina219Ready) return;
    panelVoltage = ina219.getBusVoltage_V();
    panelCurrent = ina219.getCurrent_mA();
    panelPower   = ina219.getPower_mW();
}

// ─────────────────────────────────────────────
//  NIGHT / DARKNESS DETECTION
//  All four LDRs below NIGHT_THRESHOLD -> dark
// ─────────────────────────────────────────────
bool isNightCondition(int tl, int tr, int bl, int br) {
    int maxReading = max(max(tl, tr), max(bl, br));
    return (maxReading < NIGHT_THRESHOLD);
}

// ─────────────────────────────────────────────
//  NIGHT PARK
//  Face East to be ready for morning sun
// ─────────────────────────────────────────────
void returnToNightPark() {
    servoAzimuth.write(SERVO_NIGHT_PARK_AZ);
    servoElevation.write(SERVO_NIGHT_PARK_EL);
    posAzimuth   = SERVO_NIGHT_PARK_AZ;
    posElevation = SERVO_NIGHT_PARK_EL;
    digitalWrite(PIN_LED_INDICATOR, LOW);
}

// ─────────────────────────────────────────────
//  CLAMP SERVO ANGLE
// ─────────────────────────────────────────────
void clampServo(int &angle, int minVal, int maxVal) {
    if (angle < minVal) angle = minVal;
    if (angle > maxVal) angle = maxVal;
}

// ─────────────────────────────────────────────
//  SERIAL TELEMETRY
// ─────────────────────────────────────────────
void printTelemetry(int tl, int tr, int bl, int br) {
    int maxLDR = max(max(tl, tr), max(bl, br));
    bool ledState = (maxLDR >= LED_ON_THRESHOLD);

    Serial.println(F("------------- HELIOS Telemetry -------------"));
    Serial.printf("  LDR Raw  |  TL:%4d  TR:%4d  BL:%4d  BR:%4d\n", tl, tr, bl, br);
    Serial.printf("  Servos   |  Azimuth: %3d deg   Elevation: %3d deg\n", posAzimuth, posElevation);
    Serial.printf("  LED      |  Panel light indicator: %s\n", ledState ? "ON (light detected)" : "OFF");

    if (ina219Ready) {
        Serial.printf("  Solar    |  V: %.3f V   I: %.2f mA   P: %.2f mW\n",
                      panelVoltage, panelCurrent, panelPower);
        float devAz = (posAzimuth - SERVO_CENTER) * (float)M_PI / 180.0f;
        float devEl = (posElevation - SERVO_CENTER) * (float)M_PI / 180.0f;
        float incidenceAngle = sqrtf(devAz * devAz + devEl * devEl);
        float cosEff = cosf(incidenceAngle) * 100.0f;
        Serial.printf("  Tracking |  Incidence ~%.1f deg   Cos-efficiency: %.1f%%\n",
                      incidenceAngle * 180.0f / (float)M_PI, cosEff);
    } else {
        Serial.println(F("  Solar    |  INA219 not connected - check wiring."));
    }
    Serial.println(F("--------------------------------------------\n"));
}
