#include <gpiod.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHIP "/dev/gpiochip4"

// LCD pins (chip4 line offsets == GPIO numbers on your setup)
#define RS 26
#define E  19
#define D4 13
#define D5 6
#define D6 5
#define D7 16

// Buttons (pick free GPIOs)
#define BTN_NEXT 20
#define BTN_PREV 21

static struct gpiod_chip *chip;
static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;
static struct gpiod_line *btn_next, *btn_prev;

static void pulse_enable(void) {
    gpiod_line_set_value(e, 1);
    usleep(1);
    gpiod_line_set_value(e, 0);
    usleep(50);
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

    if (cmd == 0x01 || cmd == 0x02) usleep(2000);
    else usleep(50);
}

static void lcd_char(char c) {
    gpiod_line_set_value(rs, 1);
    write4((uint8_t)c >> 4);
    write4((uint8_t)c & 0x0F);
    usleep(50);
}

static void lcd_set_cursor(int row, int col) {
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    lcd_cmd(0x80 | (addr + (uint8_t)col));
}

static void lcd_clear(void) { lcd_cmd(0x01); }

static void lcd_print_padded(const char *s) {
    // Print exactly 16 chars: pad with spaces or truncate
    char buf[17];
    memset(buf, ' ', 16);
    buf[16] = '\0';

    size_t n = strlen(s);
    if (n > 16) n = 16;
    memcpy(buf, s, n);

    for (int i = 0; i < 16; i++) lcd_char(buf[i]);
}

static void lcd_init(void) {
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

// simple debounce: wait for stable press + wait for release
static int button_pressed(struct gpiod_line *btn) {
    // assumes button pulls line LOW when pressed
    int v1 = gpiod_line_get_value(btn);

    if (v1 < 0) return 0;
    if (v1 == 1) return 0; // not pressed (HIGH)

    usleep(20000); // 20ms debounce
    int v2 = gpiod_line_get_value(btn);
    if (v2 < 0 || v2 == 1) return 0;

    // wait for release
    while (gpiod_line_get_value(btn) == 0) usleep(5000);
    usleep(20000);
    return 1;
}

int main(void) {
    // 16+ messages (circular)
    const char *msgs[] = {
        "Message 01",
        "Message 02",
        "Message 03",
        "Message 04",
        "Message 05",
        "Message 06",
        "Message 07",
        "Message 08",
        "Message 09",
        "Message 10",
        "Message 11",
        "Message 12",
        "Message 13",
        "Message 14",
        "Message 15",
        "Message 16"
    };
    const int N = (int)(sizeof(msgs) / sizeof(msgs[0]));

    chip = gpiod_chip_open(CHIP);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    // LCD lines
    rs = gpiod_chip_get_line(chip, RS);
    e  = gpiod_chip_get_line(chip, E);
    d4 = gpiod_chip_get_line(chip, D4);
    d5 = gpiod_chip_get_line(chip, D5);
    d6 = gpiod_chip_get_line(chip, D6);
    d7 = gpiod_chip_get_line(chip, D7);

    if (!rs || !e || !d4 || !d5 || !d6 || !d7) {
        perror("gpiod_chip_get_line(lcd)");
        return 1;
    }

    if (gpiod_line_request_output(rs, "rs", 0) < 0) { perror("rs"); return 1; }
    if (gpiod_line_request_output(e,  "e",  0) < 0) { perror("e");  return 1; }
    if (gpiod_line_request_output(d4, "d4", 0) < 0) { perror("d4"); return 1; }
    if (gpiod_line_request_output(d5, "d5", 0) < 0) { perror("d5"); return 1; }
    if (gpiod_line_request_output(d6, "d6", 0) < 0) { perror("d6"); return 1; }
    if (gpiod_line_request_output(d7, "d7", 0) < 0) { perror("d7"); return 1; }

    // Button lines
    btn_next = gpiod_chip_get_line(chip, BTN_NEXT);
    btn_prev = gpiod_chip_get_line(chip, BTN_PREV);
    if (!btn_next || !btn_prev) {
        perror("gpiod_chip_get_line(btn)");
        return 1;
    }

    // inputs. If you have external pull-ups, this is fine.
    if (gpiod_line_request_input(btn_next, "btn_next") < 0) { perror("btn_next"); return 1; }
    if (gpiod_line_request_input(btn_prev, "btn_prev") < 0) { perror("btn_prev"); return 1; }

    lcd_init();

    int i = 0;
    int last_1 = -1;
    int last_2 = -1;

    while (1) {
        int i2 = (i + 1) % N;

        lcd_set_cursor(0, 0);
        lcd_print_padded(msgs[i]);

        lcd_set_cursor(1, 0);
        lcd_print_padded(msgs[i2]);

        int b_1 = 1 - gpiod_line_get_value(btn_next);
        if (b_1 < 0) { perror("get_value btn_next"); return 1; }

        int b_2 = gpiod_line_get_value(btn_prev);
        if (b_2 < 0) { perror("get_value btn_prev"); return 1; }

        if (b_1 != last_1) {
            printf("Button 1 value: %d\n", b_1);
            last_1 = b_1;
        }

        if (b_2 != last_2) {
            printf("Button 2 value: %d\n", b_2);
            last_2 = b_2;
        }

        // poll buttons
        if (b_1) {
            i = (i + 1) % N;
            usleep(300000);
        } else if (b_2) {
            i = (i - 1 + N) % N;
            usleep(300000);
        }

        usleep(30000); // small delay to reduce CPU
    }
}
