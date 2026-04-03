#include "lcd.h"
#include <Arduino.h>
#include <string.h>

static int _rs, _e, _d4, _d5, _d6, _d7;

static void pulse_enable(void) {
    digitalWrite(_e, HIGH);
    delayMicroseconds(1);
    digitalWrite(_e, LOW);
    delayMicroseconds(50);
}

static void write4(uint8_t v) {
    digitalWrite(_d4, (v >> 0) & 1);
    digitalWrite(_d5, (v >> 1) & 1);
    digitalWrite(_d6, (v >> 2) & 1);
    digitalWrite(_d7, (v >> 3) & 1);
    pulse_enable();
}

void lcd_cmd(uint8_t cmd) {
    digitalWrite(_rs, LOW);
    write4(cmd >> 4);
    write4(cmd & 0x0F);

    if (cmd == 0x01 || cmd == 0x02) delayMicroseconds(2000);
    else delayMicroseconds(50);
}

void lcd_char(char c) {
    digitalWrite(_rs, HIGH);
    write4((uint8_t)c >> 4);
    write4((uint8_t)c & 0x0F);
    delayMicroseconds(50);
}

void lcd_set_cursor(int row, int col) {
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    lcd_cmd(0x80 | (addr + (uint8_t)col));
}

void lcd_clear(void) { lcd_cmd(0x01); }

void lcd_print_padded(const char *s) {
    char buf[17];
    memset(buf, ' ', 16);
    buf[16] = '\0';

    size_t n = strlen(s);
    if (n > 16) n = 16;
    memcpy(buf, s, n);

    for (int i = 0; i < 16; i++) lcd_char(buf[i]);
}

void lcd_init(int rs, int e, int d4, int d5, int d6, int d7) {
    _rs = rs; _e = e;
    _d4 = d4; _d5 = d5; _d6 = d6; _d7 = d7;

    pinMode(_rs, OUTPUT); pinMode(_e,  OUTPUT);
    pinMode(_d4, OUTPUT); pinMode(_d5, OUTPUT);
    pinMode(_d6, OUTPUT); pinMode(_d7, OUTPUT);

    delay(50);

    digitalWrite(_rs, LOW);
    digitalWrite(_e,  LOW);

    write4(0x03); delay(5);
    write4(0x03); delayMicroseconds(200);
    write4(0x03); delayMicroseconds(200);
    write4(0x02); delayMicroseconds(200);

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    delay(2);
}
