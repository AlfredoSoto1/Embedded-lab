#include <Arduino.h>
#include "servo_motor.hpp"

// ── Pin definition ────────────────────────────────────────────────────────────
// GPIO 7 is PWM-capable on the ESP32-S3 (mapped to internal LEDC timer).
#define PIN_SERVO  7

// ── LEDC / PWM config ─────────────────────────────────────────────────────────
#define CH_SERVO       2       // LEDC channel (0-7, must not clash with other experiments)
#define PWM_FREQ_HZ    50      // Standard servo frequency: 50 Hz (T = 20 ms)
#define PWM_RESOLUTION 16      // 16-bit → 65535 counts; gives ~0.3 µs resolution per step

// ── Servo pulse-width limits (per lab spec) ───────────────────────────────────
#define PULSE_MIN_MS   0.375f  // 0°   – do not go below to avoid hardware damage
#define PULSE_MAX_MS   2.100f  // 180° – do not exceed to avoid excessive current
#define PERIOD_MS      20.0f   // 1 / 50 Hz

// Convert an angle [0°, 180°] to a 16-bit LEDC duty count.
// Linear interpolation between PULSE_MIN_MS and PULSE_MAX_MS.
static uint32_t angle_to_duty(float deg) {
    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 180.0f) deg = 180.0f;
    float pulse_ms = PULSE_MIN_MS + (deg / 180.0f) * (PULSE_MAX_MS - PULSE_MIN_MS);
    return (uint32_t)((pulse_ms / PERIOD_MS) * (float)((1u << PWM_RESOLUTION) - 1u));
}

// ── Angle sequence – Table 7.4 ────────────────────────────────────────────────
// Displacements are given relative to mechanical center (90°).
// "Left"  = counter-clockwise → absolute = 90° − displacement
// "Right" = clockwise         → absolute = 90° + displacement
// Values outside [0°, 180°] are clamped to the nearest limit.
//
//  Displacement      Absolute angle
//  ─────────────     ──────────────
//  Start             0.0°
//  22.5° left       67.5°
//  90.0° left        0.0°
//  22.5° right     112.5°
//  45.0° left       45.0°
//  90.0° right     180.0°
// 135.0° left        0.0°   (90° − 135° = −45° → clamped)
//  22.5° right     112.5°
//  90.0° right     180.0°
//  67.5° right     157.5°
static const float SEQUENCE[] = {
      0.0f,   // start position
     67.5f,   // 22.5° left  of center
      0.0f,   // 90.0° left  of center
    112.5f,   // 22.5° right of center
     45.0f,   // 45.0° left  of center
    180.0f,   // 90.0° right of center
      0.0f,   // 135.0° left of center (clamped to 0°)
    112.5f,   // 22.5° right of center
    180.0f,   // 90.0° right of center
    157.5f,   // 67.5° right of center
};

#define SEQUENCE_LEN      (sizeof(SEQUENCE) / sizeof(SEQUENCE[0]))
#define STEP_INTERVAL_MS  2000UL

static uint8_t       step_index = 0;
static unsigned long step_timer = 0;

// ── Public interface ──────────────────────────────────────────────────────────
void servo_motor_setup(void) {
    ledcSetup(CH_SERVO, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_SERVO, CH_SERVO);

    // Start at 0° as required by the lab spec
    step_index = 0;
    ledcWrite(CH_SERVO, angle_to_duty(SEQUENCE[0]));
    step_timer = millis();
}

void servo_motor_loop(void) {
    if ((millis() - step_timer) >= STEP_INTERVAL_MS) {
        // Advance timer by the fixed interval to prevent drift
        step_timer += STEP_INTERVAL_MS;

        step_index = (step_index + 1) % (uint8_t)SEQUENCE_LEN;
        ledcWrite(CH_SERVO, angle_to_duty(SEQUENCE[step_index]));
    }
}
