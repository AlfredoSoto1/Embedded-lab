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

#define BTN_SCROLL 14  // Button to scroll through messages

static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main(void) {
    const int debounce_ms = 50;
    
    // Array of messages to scroll through
    const char *messages[] = {
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
        "Message 16",
        "Message 17",
        "Message 18",
        "Message 19",
        "Message 20"
    };
    const int num_messages = sizeof(messages) / sizeof(messages[0]);

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

    // Set up scroll button with interrupt
    struct gpiod_line *btn = gpiod_chip_get_line(chip, BTN_SCROLL);
    if (!btn) {
        perror("gpiod_chip_get_line(btn)");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_rising_edge_events(btn, "btn_scroll") < 0) {
        perror("gpiod_line_request_rising_edge_events");
        gpiod_chip_close(chip);
        return 1;
    }

    lcd_init(chip, rs, e, d4, d5, d6, d7);
    lcd_clear();
    
    // Display initial messages
    int current_index = 0;
    long long last_ms = 0;
    
    // Display first two messages
    lcd_set_cursor(0, 0);
    lcd_print_padded(messages[current_index]);
    lcd_set_cursor(1, 0);
    lcd_print_padded(messages[(current_index + 1) % num_messages]);
    
    printf("Displaying: %s\n", messages[current_index]);

    while (1) {
        // Wait for button press event
        int ret = gpiod_line_event_wait(btn, &(struct timespec){ .tv_sec = 1, .tv_nsec = 0 });
        if (ret < 0) {
            perror("gpiod_line_event_wait");
            break;
        }
        if (ret == 0) {
            // timeout - continue waiting
            continue;
        }

        struct gpiod_line_event ev;
        if (gpiod_line_event_read(btn, &ev) < 0) {
            perror("gpiod_line_event_read");
            break;
        }

        printf("Sensor active\n");

        // Software debounce
        long long t = now_ms();
        if (t - last_ms < debounce_ms) {
            continue; // debounce
        }
        last_ms = t;

        if (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            // Scroll to next message
            current_index = (current_index + 1) % num_messages;
            
            // Update LCD display
            lcd_set_cursor(0, 0);
            lcd_print_padded(messages[current_index]);
            lcd_set_cursor(1, 0);
            lcd_print_padded(messages[(current_index + 1) % num_messages]);
            
            printf("Scrolled to: %s\n", messages[current_index]);
        }
        
        fflush(stdout);
    }

    gpiod_line_release(led);
    gpiod_line_release(btn);
    gpiod_chip_close(chip);
    return 0;
}
