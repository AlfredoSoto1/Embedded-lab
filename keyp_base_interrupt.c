#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "lcd_api.h"
#include "keyp_api.h"

#define CHIP "/dev/gpiochip4"
#define LED_TEST 21

#define RS 5
#define E  16
#define D4 6
#define D5 13
#define D6 19
#define D7 26

// Keypad pins
#define COL1 14
#define COL2 15
#define COL3 18
#define ROW1 27
#define ROW2 22
#define ROW3 23
#define ROW4 24

static struct gpiod_line *rs, *e, *d4, *d5, *d6, *d7;

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main(void) {
    const int debounce_ms = 30;

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

    // Keypad setup - 3 columns (inputs with pull-down) and 4 rows (outputs)
    struct gpiod_line *col1 = gpiod_chip_get_line(chip, COL1);
    struct gpiod_line *col2 = gpiod_chip_get_line(chip, COL2);
    struct gpiod_line *col3 = gpiod_chip_get_line(chip, COL3);
    struct gpiod_line *row1 = gpiod_chip_get_line(chip, ROW1);
    struct gpiod_line *row2 = gpiod_chip_get_line(chip, ROW2);
    struct gpiod_line *row3 = gpiod_chip_get_line(chip, ROW3);
    struct gpiod_line *row4 = gpiod_chip_get_line(chip, ROW4);

    if (!col1 || !col2 || !col3 || !row1 || !row2 || !row3 || !row4) {
        perror("gpiod_chip_get_line(keypad)");
        return 1;
    }

    // Configure columns as inputs with pull-down and rising edge events
    if (gpiod_line_request_both_edges_events(col1, "col1") < 0) { perror("col1"); return 1; }
    if (gpiod_line_request_both_edges_events(col2, "col2") < 0) { perror("col2"); return 1; }
    if (gpiod_line_request_both_edges_events(col3, "col3") < 0) { perror("col3"); return 1; }

    // Configure rows as outputs (start low)
    if (gpiod_line_request_output(row1, "row1", 0) < 0) { perror("row1"); return 1; }
    if (gpiod_line_request_output(row2, "row2", 0) < 0) { perror("row2"); return 1; }
    if (gpiod_line_request_output(row3, "row3", 0) < 0) { perror("row3"); return 1; }
    if (gpiod_line_request_output(row4, "row4", 0) < 0) { perror("row4"); return 1; }

    // Initialize keypad
    keyp_init(chip, col1, col2, col3, row1, row2, row3, row4);

    lcd_init(chip, rs, e, d4, d5, d6, d7);
    lcd_clear();
    
    // State variables for displaying keys
    char line0_buffer[17] = "";  // 16 chars + null terminator
    char line1_buffer[17] = "";
    int current_line = 0;        // 0 or 1
    int line0_pos = 0;           // Current position on line 0
    int line1_pos = 0;           // Current position on line 1
    
    long long last_key_ms = 0;

    printf("Waiting for keypad input...\n");

    while (1) {
        // Check keypad column events (check each column)
        struct gpiod_line **cols = keyp_get_cols();
        for (int i = 0; i < 3; i++) {
            int ret = gpiod_line_event_wait(cols[i], &(struct timespec){ .tv_sec = 0, .tv_nsec = 10000000L }); // 10ms
            if (ret < 0) {
                perror("gpiod_line_event_wait(col)");
                return 1;
            }
            if (ret == 0) {
                continue; // no event on this column
            }

            struct gpiod_line_event ev;
            if (gpiod_line_event_read(cols[i], &ev) < 0) {
                perror("gpiod_line_event_read");
                break;
            }

            if (ev.event_type != GPIOD_LINE_EVENT_RISING_EDGE) {
                continue;
            } 

            long long t = now_ms();
            if (t - last_key_ms >= debounce_ms) {
                last_key_ms = t;
                
                // Scan keypad to determine which key
                char key = keyp_scan();
                if (key != '\0') {
                    printf("Key pressed: %c\n", key);
                    
                    // Drain all pending events from all columns caused by scanning
                    for (int j = 0; j < 3; j++) {
                        struct gpiod_line_event drain_ev;
                        while (gpiod_line_event_wait(cols[j], &(struct timespec){0, 0}) > 0) {
                            gpiod_line_event_read(cols[j], &drain_ev);
                        }
                    }
                    
                    if (key == '*') {
                        // Clear LCD and reset buffers
                        lcd_clear();
                        line0_buffer[0] = '\0';
                        line1_buffer[0] = '\0';
                        line0_pos = 0;
                        line1_pos = 0;
                        printf("LCD cleared\n");
                        
                    } else if (key == '#') {
                        // Switch to the other line
                        current_line = (current_line == 0) ? 1 : 0;
                        printf("Switched to line %d\n", current_line);
                        
                    } else {
                        // Display the key on the current line
                        if (current_line == 0 && line0_pos < 16) {
                            line0_buffer[line0_pos] = key;
                            line0_pos++;
                            line0_buffer[line0_pos] = '\0';
                            
                            lcd_set_cursor(0, 0);
                            lcd_print_padded(line0_buffer);
                            
                        } else if (current_line == 1 && line1_pos < 16) {
                            line1_buffer[line1_pos] = key;
                            line1_pos++;
                            line1_buffer[line1_pos] = '\0';
                            
                            lcd_set_cursor(1, 0);
                            lcd_print_padded(line1_buffer);
                        }
                    }
                }
            }
        }
        fflush(stdout);
    }

    gpiod_line_release(led);
    gpiod_line_release(col1);
    gpiod_line_release(col2);
    gpiod_line_release(col3);
    gpiod_line_release(row1);
    gpiod_line_release(row2);
    gpiod_line_release(row3);
    gpiod_line_release(row4);
    gpiod_chip_close(chip);
    return 0;
}
