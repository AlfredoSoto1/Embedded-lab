#include "rgb_pwm.h"
#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/gpio.h"

// ── Pin definitions ──────────────────────────────────────────────────────────
#define RED_PIN    9
#define GREEN_PIN  8
#define BLUE_PIN   7
#define BTN_PIN    45   // Push-button (active-low with internal pull-up)

// ── LEDC channels (avoid channel 0, used by pwm_experiment) ─────────────────
#define CH_RED     1
#define CH_GREEN   2
#define CH_BLUE    3

#define PWM_FREQ   1000  // 1000 Hz
#define PWM_BITS   8     // 8-bit resolution → 0–255

// ── Color look-up table (Table 6.4) ─────────────────────────────────────────
//   Each row: { R, G, B }
//   255 → 100% duty cycle, 0 → 0% duty cycle
static const uint8_t COLOR_TABLE[][3] = {
    {   0,   0, 255 },   // 1 – Blue
    {   0, 255,   0 },   // 2 – Green
    { 255,   0,   0 },   // 3 – Red
    { 255,  30, 217 },   // 4 – Pink / Magenta
    {  30, 222, 252 },   // 5 – Cyan
    { 240, 200,  40 },   // 6 – Yellow
    { 255, 123,  33 },   // 7 – Orange
    { 255, 255, 255 },   // 8 – White
};
static const int NUM_COLORS = sizeof(COLOR_TABLE) / sizeof(COLOR_TABLE[0]);

static int colorIndex = 0;

// Volatile flag set inside ISR, consumed in the loop
static volatile bool colorChangeFlag = false;

// ── ISR: fires on falling edge (button press) ────────────────────────────────
void IRAM_ATTR isr_btn(void) {
    colorChangeFlag = true;
}

// ── Apply the current color to the three PWM channels ───────────────────────
static void applyColor(void) {
    ledcWrite(CH_RED,   COLOR_TABLE[colorIndex][0]);
    ledcWrite(CH_GREEN, COLOR_TABLE[colorIndex][1]);
    ledcWrite(CH_BLUE,  COLOR_TABLE[colorIndex][2]);

    Serial.printf("Color %d → R:%3d  G:%3d  B:%3d\n",
                  colorIndex + 1,
                  COLOR_TABLE[colorIndex][0],
                  COLOR_TABLE[colorIndex][1],
                  COLOR_TABLE[colorIndex][2]);
}

// ── Setup ────────────────────────────────────────────────────────────────────
void rgb_pwm_setup(void) {
    Serial.begin(115200);

    // Configure three PWM channels at 1000 Hz, 8-bit resolution
    ledcSetup(CH_RED,   PWM_FREQ, PWM_BITS);
    ledcSetup(CH_GREEN, PWM_FREQ, PWM_BITS);
    ledcSetup(CH_BLUE,  PWM_FREQ, PWM_BITS);

    ledcAttachPin(RED_PIN,   CH_RED);
    ledcAttachPin(GREEN_PIN, CH_GREEN);
    ledcAttachPin(BLUE_PIN,  CH_BLUE);

    // Button: internal pull-up, ISR on falling edge
    pinMode(BTN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN_PIN), isr_btn, FALLING);

    // Enable GPIO as a light-sleep wakeup source (level LOW = button held)
    gpio_wakeup_enable((gpio_num_t)BTN_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // Show the first color and enter low-power mode from the loop
    applyColor();
    Serial.println("Setup done. Entering light sleep between button presses.");
}

// ── Loop: low-power wait → wake → handle color change ────────────────────────
void rgb_pwm_loop(void) {
    // Enter light sleep; CPU halts here until the button pulls BTN_PIN LOW
    esp_light_sleep_start();

    // CPU resumes here after wakeup — check if it was a real button press
    if (colorChangeFlag) {
        colorChangeFlag = false;
        colorIndex = (colorIndex + 1) % NUM_COLORS;
        applyColor();

        // Wait for the button to be released before sleeping again
        // to prevent immediate re-wake on level trigger
        while (digitalRead(BTN_PIN) == LOW) { /* busy-wait */ }
        delay(50); // debounce
    }
}
