#include <gpiod.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define CHIP "/dev/gpiochip4"

// LCD pins (BCM)
#define RS 26
#define E  19

#define D4 13
#define D5 6
#define D6 5
#define D7 16

static struct gpiod_chip *chip;
static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;

static void pulse_enable(void) {
    gpiod_line_set_value(e, 1);
    usleep(100);
    gpiod_line_set_value(e, 0);
    usleep(100);
}

static void write4(uint8_t v) {
    gpiod_line_set_value(d4, (v >> 0) & 1);
    gpiod_line_set_value(d5, (v >> 1) & 1);
    gpiod_line_set_value(d6, (v >> 2) & 1);
    gpiod_line_set_value(d7, (v >> 3) & 1);
    pulse_enable();
}

static void lcd_cmd(uint8_t cmd) {
    gpiod_line_set_value(rs, 0);
    write4(cmd >> 4);
    write4(cmd & 0x0F);

    // Clear/home need more time
    if (cmd == 0x01 || cmd == 0x02) usleep(2000);
    else usleep(100);
}

static void lcd_char(char c) {
    gpiod_line_set_value(rs, 1);
    write4((uint8_t)c >> 4);
    write4((uint8_t)c & 0x0F);
    usleep(100);
}

static void lcd_print(const char *s) {
    while (*s) lcd_char(*s++);
}

static void lcd_set_cursor(int row, int col) {
    // row 0 addr = 0x00, row 1 addr = 0x40
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    lcd_cmd(0x80 | (addr + (uint8_t)col));
}

static void lcd_init(void) {
     // wait after power-up
    usleep(50000);

    // 4-bit init sequence
    gpiod_line_set_value(rs, 0);
    gpiod_line_set_value(e, 0);

    write4(0x03); usleep(5000);
    write4(0x03); usleep(150);
    write4(0x03); usleep(150);
    write4(0x02); usleep(150); // 4-bit mode

    lcd_cmd(0x28); // 4-bit, 2 line, 5x8
    lcd_cmd(0x0C); // Display ON, cursor OFF, blink OFF (use 0x0F for blink)
    lcd_cmd(0x06); // Entry mode
    lcd_cmd(0x01); // Clear
}

int main(void) {
    chip = gpiod_chip_open(CHIP);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    rs = gpiod_chip_get_line(chip, RS);
    e  = gpiod_chip_get_line(chip, E);
    d4 = gpiod_chip_get_line(chip, D4);
    d5 = gpiod_chip_get_line(chip, D5);
    d6 = gpiod_chip_get_line(chip, D6);
    d7 = gpiod_chip_get_line(chip, D7);

    if (!rs || !e || !d4 || !d5 || !d6 || !d7) {
        perror("gpiod_chip_get_line");
        return 1;
    }

    if (gpiod_line_request_output(rs, "rs", 0) < 0) { perror("rs"); return 1; }
    if (gpiod_line_request_output(e,  "e",  0) < 0) { perror("e");  return 1; }
    if (gpiod_line_request_output(d4, "d4", 0) < 0) { perror("d4"); return 1; }
    if (gpiod_line_request_output(d5, "d5", 0) < 0) { perror("d5"); return 1; }
    if (gpiod_line_request_output(d6, "d6", 0) < 0) { perror("d6"); return 1; }
    if (gpiod_line_request_output(d7, "d7", 0) < 0) { perror("d7"); return 1; }

    lcd_init();

    lcd_set_cursor(0, 0);
    lcd_print("Hello!");

    // lcd_set_cursor(1, 0);
    // lcd_print("Raspberry Pi LCD");

    while (1) sleep(1);
}