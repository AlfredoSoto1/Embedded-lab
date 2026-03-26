#include "scroll_interrupt.h"
#include "lcd.h"
#include <Arduino.h>

#define SI_RS          42
#define SI_E           40
#define SI_D4          39
#define SI_D5          38
#define SI_D6          37
#define SI_D7          36
#define SI_SCROLL_UP   48
#define SI_SCROLL_DOWN 47
#define SI_DEBOUNCE_MS 50

static const char *messages[] = {
    "Message 01", "Message 02", "Message 03", "Message 04", "Message 05",
    "Message 06", "Message 07", "Message 08", "Message 09", "Message 10",
    "Message 11", "Message 12", "Message 13", "Message 14", "Message 15",
    "Message 16", "Message 17", "Message 18", "Message 19", "Message 20"
};
static const int NUM_MESSAGES = sizeof(messages) / sizeof(messages[0]);

static int currentIndex = 0;
static unsigned long lastUpMs = 0;
static unsigned long lastDownMs = 0;

// Volatile flags set inside ISRs, consumed in the loop
static volatile bool upPressed   = false;
static volatile bool downPressed = false;

void IRAM_ATTR isr_up(void)   { upPressed   = true; }
void IRAM_ATTR isr_down(void) { downPressed = true; }

static void displayMessages(void) {
    lcd_set_cursor(0, 0);
    lcd_print_padded(messages[currentIndex]);
    lcd_set_cursor(1, 0);
    lcd_print_padded(messages[(currentIndex + 1) % NUM_MESSAGES]);
}

void scroll_interrupt_setup(void) {
    Serial.begin(115200);

    lcd_init(SI_RS, SI_E, SI_D4, SI_D5, SI_D6, SI_D7);
    lcd_clear();

    pinMode(SI_SCROLL_UP,   INPUT_PULLUP);
    pinMode(SI_SCROLL_DOWN, INPUT_PULLUP);

    // Trigger ISR on falling edge (button press, active-low)
    attachInterrupt(digitalPinToInterrupt(SI_SCROLL_UP),   isr_up,   FALLING);
    attachInterrupt(digitalPinToInterrupt(SI_SCROLL_DOWN), isr_down, FALLING);

    displayMessages();
    Serial.println(messages[currentIndex]);
}

void scroll_interrupt_loop(void) {
    unsigned long t = millis();
    bool changed = false;

    if (upPressed) {
        upPressed = false;
        if (t - lastUpMs >= SI_DEBOUNCE_MS) {
            lastUpMs = t;
            currentIndex = (currentIndex - 1 + NUM_MESSAGES) % NUM_MESSAGES;
            Serial.print("Up: "); Serial.println(messages[currentIndex]);
            changed = true;
        }
    }

    if (downPressed) {
        downPressed = false;
        if (t - lastDownMs >= SI_DEBOUNCE_MS) {
            lastDownMs = t;
            currentIndex = (currentIndex + 1) % NUM_MESSAGES;
            Serial.print("Down: "); Serial.println(messages[currentIndex]);
            changed = true;
        }
    }

    if (changed) displayMessages();
}
