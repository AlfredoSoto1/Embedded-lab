#include <Arduino.h>
#include "dc_motor_driver.hpp"

// ── Pin definitions ──────────────────────────────────────────────────────────
#define PIN_S1           8    // PWM output → L293D IN1
#define PIN_S2           9    // PWM output → L293D IN2

#define PIN_BTN_LEFT     10   // Increment LEFT  speed (active LOW, internal pull-up)
#define PIN_BTN_RIGHT    11   // Increment RIGHT speed (active LOW, internal pull-up)
#define PIN_BTN_ACTIVATE 12   // Activate / stop motor  (active LOW, internal pull-up)

// ── PWM config ────────────────────────────────────────────────────────────────
// L293D max recommended: 600 mA – 1 kHz PWM is well within its switching range
#define PWM_FREQ_HZ    1000
#define PWM_RESOLUTION 8          // 8-bit → duty range 0–255
#define SPEED_LEVELS   5          // 20 % increments: 20 / 40 / 60 / 80 / 100 %

#define CH_S1 0                   // LEDC channel for PIN_S1
#define CH_S2 1                   // LEDC channel for PIN_S2

// Map speed level (1-5) to 8-bit duty cycle
static inline uint8_t level_to_duty(int level) {
    return (uint8_t)((level * 255) / SPEED_LEVELS);
}

// ── Debounce ──────────────────────────────────────────────────────────────────
#define DEBOUNCE_MS 50
#define BTN_COUNT   3

static const uint8_t BTN_PINS[BTN_COUNT] = {
    PIN_BTN_LEFT, PIN_BTN_RIGHT, PIN_BTN_ACTIVATE
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

static bool btn_held(int idx) {
    return stable_state[idx] == LOW;
}

// ── Motor state ───────────────────────────────────────────────────────────────
typedef enum { MOTOR_STOP = 0, MOTOR_LEFT, MOTOR_RIGHT } MotorState;

// Each direction keeps its own independent speed level
static int        left_level   = 1;          // 1–5 (20 %–100 %)
static int        right_level  = 1;          // 1–5 (20 %–100 %)
static MotorState motor_state  = MOTOR_STOP; // cycles STOP→LEFT→RIGHT→STOP

// Write PWM duties to both outputs to match current motor state
static void motor_apply(void) {
    switch (motor_state) {
        case MOTOR_STOP:
            ledcWrite(CH_S1, 0);
            ledcWrite(CH_S2, 0);
            break;
        case MOTOR_LEFT:
            ledcWrite(CH_S1, level_to_duty(left_level));
            ledcWrite(CH_S2, 0);
            break;
        case MOTOR_RIGHT:
            ledcWrite(CH_S1, 0);
            ledcWrite(CH_S2, level_to_duty(right_level));
            break;
    }
}

// ── Public interface ──────────────────────────────────────────────────────────
void dc_motor_driver_setup(void) {
    ledcSetup(CH_S1, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_S1, CH_S1);
    ledcSetup(CH_S2, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_S2, CH_S2);
    ledcWrite(CH_S1, 0);
    ledcWrite(CH_S2, 0);

    buttons_init();
}

void dc_motor_driver_loop(void) {
    buttons_update();

    // Safety guard: BTN_LEFT + BTN_RIGHT held simultaneously → do nothing
    // Prevents shoot-through current in the L293D H-bridge
    if (btn_held(0) && btn_held(1)) return;

    if (btn_just_pressed(0)) {
        // Cycle LEFT speed: 1→2→3→4→5→1 (20 % steps)
        left_level = (left_level % SPEED_LEVELS) + 1;
        // Live-update PWM if already spinning left
        if (motor_state == MOTOR_LEFT) motor_apply();

    } else if (btn_just_pressed(1)) {
        // Cycle RIGHT speed: 1→2→3→4→5→1
        right_level = (right_level % SPEED_LEVELS) + 1;
        // Live-update PWM if already spinning right
        if (motor_state == MOTOR_RIGHT) motor_apply();

    } else if (btn_just_pressed(2)) {
        // Cycle motor state: STOP → LEFT → RIGHT → STOP → …
        switch (motor_state) {
            case MOTOR_STOP:  motor_state = MOTOR_LEFT;  break;
            case MOTOR_LEFT:  motor_state = MOTOR_RIGHT; break;
            case MOTOR_RIGHT: motor_state = MOTOR_STOP;  break;
        }
        motor_apply();
    }
}
