#include "dimmer.h"
#include "key.h"
#include "lcd.h"
#include <Arduino.h>

// ── Pin definitions ──────────────────────────────────────────────────────────
#define LED_PIN    21   // PWM output to LED + 220 Ω resistor

#define LCD_RS     42
#define LCD_E      40
#define LCD_D4     39
#define LCD_D5     38
#define LCD_D6     37
#define LCD_D7     36

// Keypad: rows are outputs, columns are inputs (INPUT_PULLDOWN)
#define KP_C1      16
#define KP_C2      17
#define KP_C3      18
#define KP_R1       4
#define KP_R2       5
#define KP_R3       6
#define KP_R4      15

// ── LEDC (PWM) config ────────────────────────────────────────────────────────
#define LED_CHANNEL  4    // LEDC channel (0-3 used by other experiments)
#define PWM_FREQ     1000 // 1000 Hz
#define PWM_BITS     8    // 8-bit: 0–255

// ── Brightness levels ────────────────────────────────────────────────────────
// 11 levels: key '0'→0%, '1'→10%, ..., '9'→90%, '#'→100%
// duty = round(percent / 100.0 * 255)
static const uint8_t DUTY_TABLE[11] = {
      0,   // level  0 →   0%
     26,   // level  1 →  10%
     51,   // level  2 →  20%
     77,   // level  3 →  30%
    102,   // level  4 →  40%
    128,   // level  5 →  50%
    153,   // level  6 →  60%
    179,   // level  7 →  70%
    204,   // level  8 →  80%
    230,   // level  9 →  90%
    255,   // level 10 → 100%
};

static int currentLevel = 0;

static void applyLevel(int level) {
    currentLevel = level;
    ledcWrite(LED_CHANNEL, DUTY_TABLE[level]);

    // Row 0: level label
    char buf[17];
    snprintf(buf, sizeof(buf), "Level:  %2d / 10", level);
    lcd_set_cursor(0, 0);
    lcd_print_padded(buf);

    // Row 1: percentage
    snprintf(buf, sizeof(buf), "Brightness: %3d%%", level * 10);
    lcd_set_cursor(1, 0);
    lcd_print_padded(buf);

    Serial.printf("Level %d → %d%%\n", level, level * 10);
}

void dimmer_setup(void) {
    Serial.begin(115200);

    lcd_init(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
    lcd_clear();

    keyp_init(KP_C1, KP_C2, KP_C3, KP_R1, KP_R2, KP_R3, KP_R4);

    ledcSetup(LED_CHANNEL, PWM_FREQ, PWM_BITS);
    ledcAttachPin(LED_PIN, LED_CHANNEL);

    applyLevel(0);
}

void dimmer_loop(void) {
    char key = keyp_scan();
    if (key == '\0') {
        delay(20);
        return;
    }

    int level = -1;
    if (key >= '0' && key <= '9') level = key - '0';
    else if (key == '#')          level = 10;

    if (level >= 0) applyLevel(level);

    // Wait for key release, then debounce
    while (keyp_scan() != '\0') delay(10);
    delay(50);
}
