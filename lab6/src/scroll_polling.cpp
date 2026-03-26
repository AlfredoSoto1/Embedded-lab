#include "scroll_polling.h"
#include "lcd.h"
#include <Arduino.h>

#define SP_RS          42
#define SP_E           40
#define SP_D4          39
#define SP_D5          38
#define SP_D6          37
#define SP_D7          36
#define SP_SCROLL_UP   48
#define SP_SCROLL_DOWN 47
#define SP_DEBOUNCE_MS 50

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
static int prevUp, prevDown;

static void displayMessages(void) {
    lcd_set_cursor(0, 0);
    lcd_print_padded(messages[currentIndex]);
    lcd_set_cursor(1, 0);
    lcd_print_padded(messages[(currentIndex + 1) % NUM_MESSAGES]);
}

void scroll_polling_setup(void) {
    Serial.begin(115200);

    lcd_init(SP_RS, SP_E, SP_D4, SP_D5, SP_D6, SP_D7);
    lcd_clear();

    pinMode(SP_SCROLL_UP,   INPUT_PULLUP);
    pinMode(SP_SCROLL_DOWN, INPUT_PULLUP);

    prevUp   = digitalRead(SP_SCROLL_UP);
    prevDown = digitalRead(SP_SCROLL_DOWN);

    displayMessages();
    Serial.println(messages[currentIndex]);
}

void scroll_polling_loop(void) {
    int up   = digitalRead(SP_SCROLL_UP);
    int down = digitalRead(SP_SCROLL_DOWN);
    unsigned long t = millis();
    bool changed = false;

    // Falling edge = button pressed (active-low with pull-up)
    if (prevUp == HIGH && up == LOW && t - lastUpMs >= SP_DEBOUNCE_MS) {
        lastUpMs = t;
        currentIndex = (currentIndex - 1 + NUM_MESSAGES) % NUM_MESSAGES;
        Serial.print("Up: "); Serial.println(messages[currentIndex]);
        changed = true;
    }

    if (prevDown == HIGH && down == LOW && t - lastDownMs >= SP_DEBOUNCE_MS) {
        lastDownMs = t;
        currentIndex = (currentIndex + 1) % NUM_MESSAGES;
        Serial.print("Down: "); Serial.println(messages[currentIndex]);
        changed = true;
    }

    if (changed) displayMessages();

    prevUp   = up;
    prevDown = down;
    delay(1);
}

