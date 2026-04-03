#include <Arduino.h>
#include "lcd.h"

// ── Pin definitions ──────────────────────────────────────────────────────────
#define PIN_S1          8    // H-Bridge IN1 (motor direction output)
#define PIN_S2          9    // H-Bridge IN2 (motor direction output)

#define PIN_BTN_STOP    10   // Push button – STOP  (active LOW, internal pull-up)
#define PIN_BTN_LEFT    11   // Push button – LEFT  (active LOW, internal pull-up)
#define PIN_BTN_RIGHT   12   // Push button – RIGHT (active LOW, internal pull-up)

// LCD 4-bit interface pins
#define LCD_RS  13
#define LCD_E   14
#define LCD_D4  15
#define LCD_D5  16
#define LCD_D6  17
#define LCD_D7  18

// ── Debounce config ───────────────────────────────────────────────────────────
#define DEBOUNCE_MS  50
#define BTN_COUNT    3

static const uint8_t BTN_PINS[BTN_COUNT] = {
    PIN_BTN_STOP, PIN_BTN_LEFT, PIN_BTN_RIGHT
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

// True on falling edge: button just pressed this cycle
static bool btn_just_pressed(int idx) {
    return (stable_state[idx] == LOW && prev_stable[idx] == HIGH);
}

// True while button is physically held (for simultaneous detection)
static bool btn_held(int idx) {
    return stable_state[idx] == LOW;
}

// ── Motor state ───────────────────────────────────────────────────────────────
typedef enum { MOTOR_STOP = 0, MOTOR_LEFT, MOTOR_RIGHT } MotorState;

static MotorState  motor_state   = MOTOR_STOP;
static const char *last_btn_name = "STOP";
static bool        lcd_dirty     = true;

static void motor_apply(MotorState s) {
    switch (s) {
        case MOTOR_STOP:  digitalWrite(PIN_S1, LOW);  digitalWrite(PIN_S2, LOW);  break;
        // LEFT:  S1=HIGH, S2=LOW
        case MOTOR_LEFT:  digitalWrite(PIN_S1, HIGH); digitalWrite(PIN_S2, LOW);  break;
        // RIGHT: S1=LOW,  S2=HIGH
        case MOTOR_RIGHT: digitalWrite(PIN_S1, LOW);  digitalWrite(PIN_S2, HIGH); break;
    }
    motor_state = s;
    lcd_dirty   = true;
}

// ── LCD output ────────────────────────────────────────────────────────────────
static void lcd_update(void) {
    if (!lcd_dirty) return;
    lcd_dirty = false;

    const char *state_str;
    switch (motor_state) {
        case MOTOR_STOP:  state_str = "STOP";  break;
        case MOTOR_LEFT:  state_str = "LEFT";  break;
        case MOTOR_RIGHT: state_str = "RIGHT"; break;
        default:          state_str = "?";     break;
    }

    char line[17];
    lcd_set_cursor(0, 0);
    snprintf(line, sizeof(line), "State: %-5s", state_str);
    lcd_print_padded(line);

    lcd_set_cursor(1, 0);
    snprintf(line, sizeof(line), "Btn:   %-5s", last_btn_name);
    lcd_print_padded(line);
}

// ── Public interface ──────────────────────────────────────────────────────────
void dc_motor_setup(void) {
    pinMode(PIN_S1, OUTPUT);
    pinMode(PIN_S2, OUTPUT);
    motor_apply(MOTOR_STOP);

    buttons_init();

    lcd_init(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
    lcd_update();
}

void dc_motor_loop(void) {
    buttons_update();

    // Safety: LEFT + RIGHT simultaneously → ignore to protect H-bridge transistors
    if (btn_held(1) && btn_held(2)) return;

    if (btn_just_pressed(0)) {          // BTN_STOP
        last_btn_name = "STOP";
        motor_apply(MOTOR_STOP);
    } else if (btn_just_pressed(1)) {   // BTN_LEFT
        last_btn_name = "LEFT";
        motor_apply(MOTOR_LEFT);
    } else if (btn_just_pressed(2)) {   // BTN_RIGHT
        last_btn_name = "RIGHT";
        motor_apply(MOTOR_RIGHT);
    }

    lcd_update();
}
