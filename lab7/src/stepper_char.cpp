#include <Arduino.h>
#include "stepper_char.hpp"

// ── Stepper pin definitions ───────────────────────────────────────────────────
// Same 28YBJ-48 wiring via ULN2803 as EXPERIMENT_STEPPER_MOTOR
#define PIN_A   4    // Orange (A)
#define PIN_B   5    // Yellow (B)
#define PIN_Ap  6    // Pink   (A')
#define PIN_Bp  7    // Blue   (B')

// ── Button pin definitions ────────────────────────────────────────────────────
#define PIN_BTN_UP        35   // UP        – increase motor speed (shorter period)
#define PIN_BTN_DOWN      36   // DOWN      – decrease motor speed (longer period)
#define PIN_BTN_STARTSTOP 37   // START/STOP – begin or interrupt 720° run

// ── Half-step sequence (Table 7.6) ───────────────────────────────────────────
// ULN2803 is inverting: MCU HIGH energises the coil.
// Columns: {A, B, A', B'}
static const uint8_t HALF_STEP[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1},
};

// ── 28YBJ-48 constants ────────────────────────────────────────────────────────
// Half-step mode: 4096 steps per output shaft revolution
// 720° = 2 full revolutions → 8192 half-steps
#define STEPS_PER_REV 4096
#define STEPS_720     (STEPS_PER_REV * 2)   // 8192 steps

// ── Speed table – Table 7.7 ───────────────────────────────────────────────────
// Each entry is the inter-step period (signal period) in microseconds.
// Index 0 = slowest (100 ms), index 9 = fastest (0.1 ms).
//
//  Idx  Period    Frequency   RPM (output shaft)
//   0   100.0 ms    10 Hz     10   / (4096 steps/rev / 60) ≈ 0.147 rpm
//   1    50.0 ms    20 Hz     ≈  0.293 rpm
//   2    20.0 ms    50 Hz     ≈  0.732 rpm
//   3    10.0 ms   100 Hz     ≈  1.465 rpm
//   4     5.0 ms   200 Hz     ≈  2.930 rpm
//   5     2.0 ms   500 Hz     ≈  7.324 rpm
//   6     1.0 ms  1000 Hz     ≈ 14.648 rpm
//   7     0.5 ms  2000 Hz     ≈ 29.297 rpm
//   8     0.2 ms  5000 Hz     ≈ 73.242 rpm
//   9     0.1 ms 10000 Hz     ≈146.484 rpm
static const uint32_t PERIODS_US[] = {
    100000,   // 100.0 ms →   10 Hz
     50000,   //  50.0 ms →   20 Hz
     20000,   //  20.0 ms →   50 Hz
     10000,   //  10.0 ms →  100 Hz
      5000,   //   5.0 ms →  200 Hz
      2000,   //   2.0 ms →  500 Hz
      1000,   //   1.0 ms → 1000 Hz
       500,   //   0.5 ms → 2000 Hz
       200,   //   0.2 ms → 5000 Hz
       100,   //   0.1 ms → 10000 Hz
};

#define SPEED_LEVELS  (sizeof(PERIODS_US) / sizeof(PERIODS_US[0]))

// ── Debounce ──────────────────────────────────────────────────────────────────
#define DEBOUNCE_MS 50
#define BTN_COUNT   3

static const uint8_t BTN_PINS[BTN_COUNT] = {
    PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_STARTSTOP
};

static unsigned long debounce_timer[BTN_COUNT];
static bool          raw_state[BTN_COUNT];
static bool          stable_state[BTN_COUNT];
static bool          prev_stable[BTN_COUNT];

static void buttons_init(void) {
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(BTN_PINS[i], INPUT_PULLUP);
        raw_state[i]      = HIGH;
        stable_state[i]   = HIGH;
        prev_stable[i]    = HIGH;
        debounce_timer[i] = 0;
    }
}

static void buttons_update(void) {
    unsigned long now = millis();
    for (int i = 0; i < BTN_COUNT; i++) {
        bool raw = (bool)digitalRead(BTN_PINS[i]);
        if (raw != raw_state[i]) {
            raw_state[i]      = raw;
            debounce_timer[i] = now;
        }
        prev_stable[i] = stable_state[i];
        if ((now - debounce_timer[i]) >= DEBOUNCE_MS) {
            stable_state[i] = raw_state[i];
        }
    }
}

static bool btn_just_pressed(int idx) {
    return (stable_state[idx] == LOW && prev_stable[idx] == HIGH);
}

// ── Coil output helpers ───────────────────────────────────────────────────────
static uint8_t seq_idx = 0;

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

static void advance_seq(void) {
    write_coils(HALF_STEP[seq_idx]);
    seq_idx = (uint8_t)((seq_idx + 1) % 8);
}

// ── Module state ──────────────────────────────────────────────────────────────
typedef enum { STATE_IDLE, STATE_RUNNING } ExpState;

static ExpState  state        = STATE_IDLE;
static uint8_t   speed_idx    = 0;       // start at 1st entry (100 ms) per Table 7.7
static int32_t   steps_left   = 0;
static uint32_t  last_step_us = 0;       // micros() timestamp of last step

// ── Public interface ──────────────────────────────────────────────────────────
void stepper_char_setup(void) {
    pinMode(PIN_A,  OUTPUT);
    pinMode(PIN_B,  OUTPUT);
    pinMode(PIN_Ap, OUTPUT);
    pinMode(PIN_Bp, OUTPUT);
    deenergize();

    buttons_init();
}

void stepper_char_loop(void) {
    buttons_update();

    if (state == STATE_IDLE) {
        // UP key: increase speed (advance to next shorter period)
        if (btn_just_pressed(0)) {
            if (speed_idx < (uint8_t)(SPEED_LEVELS - 1)) speed_idx++;
        }
        // DOWN key: decrease speed (go back to longer period)
        else if (btn_just_pressed(1)) {
            if (speed_idx > 0) speed_idx--;
        }
        // START: begin 720° (8192 half-steps) at the current period
        else if (btn_just_pressed(2)) {
            steps_left   = STEPS_720;
            last_step_us = micros();
            state        = STATE_RUNNING;
        }

    } else {   // STATE_RUNNING
        // STOP: emergency interrupt – de-energize and return to idle
        if (btn_just_pressed(2)) {
            deenergize();
            state = STATE_IDLE;
            return;
        }

        // Non-blocking step: fire when one period has elapsed.
        // Accumulate last_step_us to avoid drift across many steps.
        // uint32_t subtraction handles micros() rollover correctly.
        uint32_t now = micros();
        if ((now - last_step_us) >= PERIODS_US[speed_idx]) {
            last_step_us += PERIODS_US[speed_idx];
            advance_seq();
            steps_left--;
            if (steps_left <= 0) {
                deenergize();
                state = STATE_IDLE;
            }
        }
    }
}
