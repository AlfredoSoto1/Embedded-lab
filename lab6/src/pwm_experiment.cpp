#include "pwm_experiment.h"
#include <Arduino.h>

#define PWM_PIN        7    // GPIO pin for PWM output (RED on RGB LED)
#define PWM_CHANNEL    0    // LEDC channel
#define PWM_RES_BITS   8    // 8-bit resolution: duty range 0–255
#define PWM_DUTY_50    128  // 50% duty cycle (128 / 255 ≈ 50%)

#define CHANGE_PIN     15   // Button to cycle through frequencies (active-low)
#define DEBOUNCE_MS    50

// Table 6.2 frequencies (Hz)
static const uint32_t FREQUENCIES[] = { 500, 1000, 2000, 4000, 8000 };
static const int NUM_FREQS = sizeof(FREQUENCIES) / sizeof(FREQUENCIES[0]);

static int freqIndex = 0;
static unsigned long lastChangeMs = 0;
static int prevBtn;

static void applyFrequency(void) {
    uint32_t freq = FREQUENCIES[freqIndex];
    ledcSetup(PWM_CHANNEL, freq, PWM_RES_BITS);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, PWM_DUTY_50);

    // Period in microseconds = 1,000,000 / freq
    uint32_t period_us = 1000000UL / freq;
    Serial.print("Frequency: "); Serial.print(freq);    Serial.print(" Hz  |  ");
    Serial.print("Period: ");    Serial.print(period_us); Serial.print(" us  |  ");
    Serial.println("Duty: 50%");
}

void pwm_setup(void) {
    Serial.begin(115200);

    pinMode(CHANGE_PIN, INPUT_PULLUP);
    prevBtn = digitalRead(CHANGE_PIN);

    applyFrequency();
}

void pwm_loop(void) {
    int btn = digitalRead(CHANGE_PIN);
    unsigned long t = millis();

    // Falling edge = button pressed (active-low with pull-up)
    if (prevBtn == HIGH && btn == LOW && t - lastChangeMs >= DEBOUNCE_MS) {
        lastChangeMs = t;
        freqIndex = (freqIndex + 1) % NUM_FREQS;
        applyFrequency();
    }

    prevBtn = btn;
}
