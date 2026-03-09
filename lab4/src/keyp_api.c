#include "keyp_api.h"
#include <gpiod.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct gpiod_chip *chip;
static struct gpiod_line *cols[3];
static struct gpiod_line *rows[4];

static const char KEYMAP[4][3] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};

static void set_all_rows(int v) {
    for (int i = 0; i < 4; i++) {
        gpiod_line_set_value(rows[i], v);
    }
}

char keyp_scan(void) {
    // Scan each row by setting it high one at a time
    for (int r = 0; r < 4; r++) {
        // Set all rows low first
        set_all_rows(0);
        // Set current row high
        gpiod_line_set_value(rows[r], 1);

        // Small settle time
        usleep(300);

        // Check each column
        for (int c = 0; c < 3; c++) {
            int v = gpiod_line_get_value(cols[c]);
            if (v == 1) {
                set_all_rows(1);  // Reset rows
                return KEYMAP[r][c];
            }
        }
    }
    set_all_rows(1);  // Reset all rows
    return '\0';
}

void keyp_init(struct gpiod_chip *chip_arg, 
              struct gpiod_line *c1_line, 
              struct gpiod_line *c2_line,
              struct gpiod_line *c3_line, 
              struct gpiod_line *r1_line,
              struct gpiod_line *r2_line, 
              struct gpiod_line *r3_line,
              struct gpiod_line *r4_line) {
    chip = chip_arg;
    cols[0] = c1_line;
    cols[1] = c2_line;
    cols[2] = c3_line;
    rows[0] = r1_line;
    rows[1] = r2_line;
    rows[2] = r3_line;
    rows[3] = r4_line;

    set_all_rows(1);
}

struct gpiod_line** keyp_get_cols(void) {
    return cols;
}