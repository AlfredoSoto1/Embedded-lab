#include <Arduino.h>
#include "stepper_motor.hpp"

// ── Pin definitions ───────────────────────────────────────────────────────────
// Connect MCU pins → ULN2803 inputs (IN1–IN4); ULN2803 outputs drive motor coils.
// ULN2803 is an inverting current-sink: MCU HIGH → output LOW → coil energised.
// The sequences below represent MCU output values (1 = energise coil).
#define PIN_A   4    // Orange (A)
#define PIN_B   5    // Yellow (B)
#define PIN_Ap  6    // Pink   (A')
#define PIN_Bp  7    // Blue   (B')

// ── Select sequence: comment out to use FULL-STEP ────────────────────────────
#define USE_HALF_STEP

// ── Timing ────────────────────────────────────────────────────────────────────
#define STEP_DELAY_MS  10UL

// ── 28YBJ-48 motor constants ──────────────────────────────────────────────────
// Stride angle: 5.625° per motor step (4-phase, before gear reduction)
// Gear ratio  : 64:1
// Full-step   → 11.25°/step  (motor), output shaft: 360°/(512 steps) = 0.703125°... 
//   Actually: 360° / (32 steps/rev × 64 gear) = 360/2048 = 0.17578°/full-step
// Half-step   → 0.08789°/half-step → 4096 half-steps per output revolution
#ifdef USE_HALF_STEP
  #define STEPS_PER_REV 4096
#else
  #define STEPS_PER_REV 2048
#endif

// ── Full-step sequence – Table 7.5 ───────────────────────────────────────────
// One-phase (wave drive): a single coil energised per step.
// Columns: {A, B, A', B'}
static const uint8_t FULL_STEP[4][4] = {
    {1, 0, 0, 0},   // Step 1
    {0, 1, 0, 0},   // Step 2
    {0, 0, 1, 0},   // Step 3
    {0, 0, 0, 1},   // Step 4
};

// ── Half-step sequence – Table 7.6 ───────────────────────────────────────────
// Alternates single-coil and two-coil energisation; doubles angular resolution.
// Columns: {A, B, A', B'}
static const uint8_t HALF_STEP[8][4] = {
    {1, 0, 0, 0},   // Step 1
    {1, 1, 0, 0},   // Step 2
    {0, 1, 0, 0},   // Step 3
    {0, 1, 1, 0},   // Step 4
    {0, 0, 1, 0},   // Step 5
    {0, 0, 1, 1},   // Step 6
    {0, 0, 0, 1},   // Step 7
    {1, 0, 0, 1},   // Step 8
};

// ── Module state ──────────────────────────────────────────────────────────────
static uint8_t seq_idx = 0;   // Current position in the active sequence

static void write_coils(const uint8_t *row) {
    digitalWrite(PIN_A,  row[0]);
    digitalWrite(PIN_B,  row[1]);
    digitalWrite(PIN_Ap, row[2]);
    digitalWrite(PIN_Bp, row[3]);
}

static void deenergize(void) {
    const uint8_t off[4] = {0, 0, 0, 0};
    write_coils(off);
}

// Advance one step; dir = +1 (left / CW) or -1 (right / CCW).
// seq_idx update uses modular wrap in the correct direction.
static void step_once(int8_t dir) {
#ifdef USE_HALF_STEP
    write_coils(HALF_STEP[seq_idx]);
    seq_idx = (uint8_t)((seq_idx + (dir > 0 ? 1 : 7)) % 8);
#else
    write_coils(FULL_STEP[seq_idx]);
    seq_idx = (uint8_t)((seq_idx + (dir > 0 ? 1 : 3)) % 4);
#endif
    delay(STEP_DELAY_MS);
}

// Rotate 'steps' steps; positive = left, negative = right.
static void rotate_steps(int32_t steps) {
    int8_t  dir   = (steps >= 0) ? 1 : -1;
    int32_t total = (steps >= 0) ? steps : -steps;
    for (int32_t s = 0; s < total; s++) {
        step_once(dir);
    }
}

// Convert degrees to step count for the active sequence mode.
static int32_t degrees_to_steps(float deg) {
    return (int32_t)((deg / 360.0f) * (float)STEPS_PER_REV);
}

// ── Public interface ──────────────────────────────────────────────────────────
void stepper_motor_setup(void) {
    pinMode(PIN_A,  OUTPUT);
    pinMode(PIN_B,  OUTPUT);
    pinMode(PIN_Ap, OUTPUT);
    pinMode(PIN_Bp, OUTPUT);
    deenergize();
}

void stepper_motor_loop(void) {
    // ── Rotation routine (runs once, then idles) ──────────────────────────────
    // Step 12: 270° left → 180° left → 90° right

    // 270° to the left
    rotate_steps(degrees_to_steps(270.0f));
    deenergize();
    delay(500);

    // 180° to the left
    rotate_steps(degrees_to_steps(180.0f));
    deenergize();
    delay(500);

    // 90° to the right
    rotate_steps(-degrees_to_steps(90.0f));
    deenergize();

    // Routine complete – idle (yield to FreeRTOS watchdog via delay)
    while (true) {
        delay(1000);
    }
}
