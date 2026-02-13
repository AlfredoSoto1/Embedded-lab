#include "lcd_api.h"
#include <gpiod.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct gpiod_chip *chip;
static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;

static void pulse_enable(void) {
    gpiod_line_set_value(e, 1);
    usleep(1);
    gpiod_line_set_value(e, 0);
    usleep(50);
}

void write4(uint8_t v) {
    gpiod_line_set_value(d4, (v >> 0) & 1);
    gpiod_line_set_value(d5, (v >> 1) & 1);
    gpiod_line_set_value(d6, (v >> 2) & 1);
    gpiod_line_set_value(d7, (v >> 3) & 1);
    pulse_enable();
}

void lcd_cmd(uint8_t cmd) {
    gpiod_line_set_value(rs, 0);
    write4(cmd >> 4);
    write4(cmd & 0x0F);

    if (cmd == 0x01 || cmd == 0x02) usleep(2000);
    else usleep(50);
}

void lcd_char(char c) {
    gpiod_line_set_value(rs, 1);
    write4((uint8_t)c >> 4);
    write4((uint8_t)c & 0x0F);
    usleep(50);
}

void lcd_set_cursor(int row, int col) {
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    lcd_cmd(0x80 | (addr + (uint8_t)col));
}

void lcd_clear(void) { lcd_cmd(0x01); }

void lcd_print_padded(const char *s) {
    // Print exactly 16 chars: pad with spaces or truncate
    char buf[17];
    memset(buf, ' ', 16);
    buf[16] = '\0';

    size_t n = strlen(s);
    if (n > 16) n = 16;
    memcpy(buf, s, n);

    for (int i = 0; i < 16; i++) lcd_char(buf[i]);
}

void lcd_init(struct gpiod_chip *chip_arg, 
              struct gpiod_line *rs_arg, 
              struct gpiod_line *e_arg,
              struct gpiod_line *d4_arg, 
              struct gpiod_line *d5_arg,
              struct gpiod_line *d6_arg, 
              struct gpiod_line *d7_arg
          ) {
    chip = chip_arg;
    rs = rs_arg;
    e = e_arg;
    d4 = d4_arg;
    d5 = d5_arg;
    d6 = d6_arg;
    d7 = d7_arg;

    usleep(50000);

    gpiod_line_set_value(rs, 0);
    gpiod_line_set_value(e, 0);

    write4(0x03); usleep(5000);
    write4(0x03); usleep(200);
    write4(0x03); usleep(200);
    write4(0x02); usleep(200);

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    usleep(2000);
}