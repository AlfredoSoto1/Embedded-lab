#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "lcd_api.h"

#define CHIP        "/dev/gpiochip4"
#define LED_TEST    21
#define RS          5
#define E           16
#define D4          6
#define D5          13
#define D6          19
#define D7          26
#define SCROLL_UP   14
#define SCROLL_DOWN 15

static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main(void)
{
    const int debounce_ms = 50;

    const char *messages[] = {
        "Message 01", "Message 02", "Message 03", "Message 04", "Message 05",
        "Message 06", "Message 07", "Message 08", "Message 09", "Message 10",
        "Message 11", "Message 12", "Message 13", "Message 14", "Message 15",
        "Message 16", "Message 17", "Message 18", "Message 19", "Message 20"};
    const int num_messages = sizeof(messages) / sizeof(messages[0]);

    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip)
    {
        perror("gpiod_chip_open");
        return 1;
    }

    /* LED */
    struct gpiod_line *led = gpiod_chip_get_line(chip, LED_TEST);
    if (!led || gpiod_line_request_output(led, "led", 0) < 0)
    {
        perror("led");
        return 1;
    }
    gpiod_line_set_value(led, 1);

    /* LCD */
    rs = gpiod_chip_get_line(chip, RS);
    e = gpiod_chip_get_line(chip, E);
    d4 = gpiod_chip_get_line(chip, D4);
    d5 = gpiod_chip_get_line(chip, D5);
    d6 = gpiod_chip_get_line(chip, D6);
    d7 = gpiod_chip_get_line(chip, D7);
    if (!rs || !e || !d4 || !d5 || !d6 || !d7)
    {
        perror("lcd lines");
        return 1;
    }
    if (gpiod_line_request_output(rs, "rs", 0) < 0 ||
        gpiod_line_request_output(e, "e", 0) < 0 ||
        gpiod_line_request_output(d4, "d4", 0) < 0 ||
        gpiod_line_request_output(d5, "d5", 0) < 0 ||
        gpiod_line_request_output(d6, "d6", 0) < 0 ||
        gpiod_line_request_output(d7, "d7", 0) < 0)
    {
        perror("lcd setup");
        return 1;
    }

    /* Scroll buttons — falling-edge interrupt (active-low) */
    struct gpiod_line *btn_up = gpiod_chip_get_line(chip, SCROLL_UP);
    struct gpiod_line *btn_down = gpiod_chip_get_line(chip, SCROLL_DOWN);
    if (!btn_up || !btn_down)
    {
        perror("button lines");
        return 1;
    }
    if (gpiod_line_request_falling_edge_events(btn_up, "scroll_up") < 0)
    {
        perror("scroll_up");
        return 1;
    }
    if (gpiod_line_request_falling_edge_events(btn_down, "scroll_down") < 0)
    {
        perror("scroll_down");
        return 1;
    }

    /* Group both buttons so we can block-wait on either simultaneously */
    struct gpiod_line_bulk btn_bulk;
    gpiod_line_bulk_init(&btn_bulk);
    gpiod_line_bulk_add(&btn_bulk, btn_up);
    gpiod_line_bulk_add(&btn_bulk, btn_down);

    lcd_init(chip, rs, e, d4, d5, d6, d7);
    lcd_clear();

    int current_index = 0;
    long long last_up_ms = 0;
    long long last_down_ms = 0;

    lcd_set_cursor(0, 0);
    lcd_print_padded(messages[current_index]);
    lcd_set_cursor(1, 0);
    lcd_print_padded(messages[(current_index + 1) % num_messages]);
    printf("Displaying: %s\n", messages[current_index]);

    while (1)
    {
        struct gpiod_line_bulk event_bulk;
        gpiod_line_bulk_init(&event_bulk);

        /* Block indefinitely until a button is pressed — CPU sleeps */
        int ret = gpiod_line_event_wait_bulk(&btn_bulk, NULL, &event_bulk);
        if (ret < 0)
        {
            perror("event_wait_bulk");
            break;
        }

        long long t = now_ms();
        int changed = 0;

        unsigned int n = gpiod_line_bulk_num_lines(&event_bulk);
        for (unsigned int i = 0; i < n; i++)
        {
            struct gpiod_line *line = gpiod_line_bulk_get_line(&event_bulk, i);
            struct gpiod_line_event ev;
            gpiod_line_event_read(line, &ev);

            if (line == btn_up && t - last_up_ms >= debounce_ms)
            {
                last_up_ms = t;
                current_index =
                    (current_index - 1 + num_messages) % num_messages;
                printf("Up: %s\n", messages[current_index]);
                changed = 1;
            }
            else if (line == btn_down && t - last_down_ms >= debounce_ms)
            {
                last_down_ms = t;
                current_index = (current_index + 1) % num_messages;
                printf("Down: %s\n", messages[current_index]);
                changed = 1;
            }
        }

        if (changed)
        {
            lcd_set_cursor(0, 0);
            lcd_print_padded(messages[current_index]);
            lcd_set_cursor(1, 0);
            lcd_print_padded(messages[(current_index + 1) % num_messages]);
            fflush(stdout);
        }
    }

    gpiod_line_release(led);
    gpiod_line_release(btn_up);
    gpiod_line_release(btn_down);
    gpiod_chip_close(chip);
    return 0;
}
