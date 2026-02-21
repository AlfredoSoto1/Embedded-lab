#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "lcd_api.h"

#define CHIP "/dev/gpiochip4"
#define LED_TEST 21

#define RS 5
#define E  16
#define D4 6
#define D5 13
#define D6 19
#define D7 26

static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;

int main(void) {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    // Set up the LED line as output test
    struct gpiod_line *led = gpiod_chip_get_line(chip, LED_TEST);
    if (!led) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(led, "led", 0) < 0) { 
        perror("led"); return 1; 
    }
    gpiod_line_set_value(led, 1); // turn on the LED

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

    struct gpiod_line *btn = gpiod_chip_get_line(chip, 20);

    if (gpiod_line_request_both_edges_events(btn, "btn") < 0) {
        perror("gpiod_line_request_both_edges_events");
        gpiod_chip_close(chip);
        return 1;
    }

    lcd_init(chip, rs, e, d4, d5, d6, d7);
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print_padded("Counter: 0");
    
    int counter = 0;

    while (1) {
        // Wait up to 5 seconds; -1 means wait forever
        int ret = gpiod_line_event_wait(btn, &(struct timespec){ .tv_sec = 5, .tv_nsec = 0 });
        if (ret < 0) {
            perror("gpiod_line_event_wait");
            break;
        }
        if (ret == 0) {
            // timeout (optional)
            continue;
        }

        struct gpiod_line_event ev;
        if (gpiod_line_event_read(btn, &ev) < 0) {
            perror("gpiod_line_event_read");
            break;
        }

        if (ev.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            // printf("Pressed %d\n", counter++);
            counter++;
            
            // Display the counter on LCD
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Counter: %d", counter);
            lcd_set_cursor(0, 0);
            lcd_print_padded(buffer);
            printf("Counter: %d\n", counter);
        } else if (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            // printf("Released\n");
        }
        fflush(stdout);
    }

    gpiod_line_release(led);
    gpiod_line_release(btn);
    gpiod_chip_close(chip);
    return 0;
}
