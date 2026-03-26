#include "key.h"
#include <Arduino.h>

static int cols[3];
static int rows[4];

static const char KEYMAP[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

static void set_all_rows(int v) {
    for (int i = 0; i < 4; i++)
        digitalWrite(rows[i], v);
}

void keyp_init(int c1, int c2, int c3, int r1, int r2, int r3, int r4) {
    cols[0] = c1; cols[1] = c2; cols[2] = c3;
    rows[0] = r1; rows[1] = r2; rows[2] = r3; rows[3] = r4;

    // Columns: inputs pulled LOW; a pressed key drives the column HIGH
    for (int i = 0; i < 3; i++) pinMode(cols[i], INPUT_PULLDOWN);

    // Rows: outputs, idle HIGH
    for (int i = 0; i < 4; i++) {
        pinMode(rows[i], OUTPUT);
        digitalWrite(rows[i], HIGH);
    }
}

char keyp_scan(void) {
    for (int r = 0; r < 4; r++) {
        set_all_rows(LOW);
        digitalWrite(rows[r], HIGH);
        delayMicroseconds(300);

        for (int c = 0; c < 3; c++) {
            if (digitalRead(cols[c]) == HIGH) {
                set_all_rows(HIGH);
                return KEYMAP[r][c];
            }
        }
    }
    set_all_rows(HIGH);
    return '\0';
}