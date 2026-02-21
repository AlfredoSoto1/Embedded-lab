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

#define ENCODER_A 14
#define ENCODER_B 15

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

    // Set up rotary encoder with interrupts on both edges
    struct gpiod_line *encoder_a = gpiod_chip_get_line(chip, ENCODER_A);
    struct gpiod_line *encoder_b = gpiod_chip_get_line(chip, ENCODER_B);
    
    if (!encoder_a || !encoder_b) {
        perror("gpiod_chip_get_line(encoder)");
        gpiod_chip_close(chip);
        return 1;
    }

    // Request both rising and falling edge events for encoder A
    if (gpiod_line_request_both_edges_events(encoder_a, "encoder_a") < 0) {
        perror("gpiod_line_request_both_edges_events(encoder_a)");
        gpiod_chip_close(chip);
        return 1;
    }

    // Request both rising and falling edge events for encoder B
    if (gpiod_line_request_both_edges_events(encoder_b, "encoder_b") < 0) {
        perror("gpiod_line_request_both_edges_events(encoder_b)");
        gpiod_chip_close(chip);
        return 1;
    }

    lcd_init(chip, rs, e, d4, d5, d6, d7);
    lcd_clear();
    
    // Display initial messages
    int current_index = 0;
    long long last_change_ms = 0;
    
    // Read initial state
    int last_a = gpiod_line_get_value(encoder_a);
    int last_b = gpiod_line_get_value(encoder_b);
    int last_state = (last_a << 1) | last_b;
    
    // Display first two messages
    lcd_set_cursor(0, 0);
    lcd_print_padded(messages[current_index]);
    lcd_set_cursor(1, 0);
    lcd_print_padded(messages[(current_index + 1) % num_messages]);
    
    printf("Displaying: %s\n", messages[current_index]);

    while (1) {
        // Wait for interrupt on encoder A
        int ret_a = gpiod_line_event_wait(encoder_a, &(struct timespec){ .tv_sec = 0, .tv_nsec = 10000000 });
        if (ret_a > 0) {
            struct gpiod_line_event ev;
            gpiod_line_event_read(encoder_a, &ev);
            
            // Read current state of both pins
            int a = gpiod_line_get_value(encoder_a);
            int b = gpiod_line_get_value(encoder_b);
            int state = (a << 1) | b;
            
            if (state != last_state) {
                long long t = now_ms();
                
                // Debounce
                if (t - last_change_ms >= debounce_ms) {
                    int direction = 0;
                    
                    // Decode Gray code transitions for forward direction
                    if (last_state == 0b00 && state == 0b10) direction = 1;
                    else if (last_state == 0b10 && state == 0b11) direction = 1;
                    else if (last_state == 0b11 && state == 0b01) direction = 1;
                    else if (last_state == 0b01 && state == 0b00) direction = 1;
                    
                    // Decode Gray code transitions for backward direction
                    else if (last_state == 0b00 && state == 0b01) direction = -1;
                    else if (last_state == 0b01 && state == 0b11) direction = -1;
                    else if (last_state == 0b11 && state == 0b10) direction = -1;
                    else if (last_state == 0b10 && state == 0b00) direction = -1;
                    
                    if (direction != 0) {
                        last_change_ms = t;
                        
                        if (direction == 1) {
                            current_index = (current_index + 1) % num_messages;
                            printf("Scrolled forward to: %s\n", messages[current_index]);
                        } else {
                            current_index = (current_index - 1 + num_messages) % num_messages;
                            printf("Scrolled backward to: %s\n", messages[current_index]);
                        }
                        
                        // Update LCD display
                        lcd_set_cursor(0, 0);
                        lcd_print_padded(messages[current_index]);
                        lcd_set_cursor(1, 0);
                        lcd_print_padded(messages[(current_index + 1) % num_messages]);
                        
                        fflush(stdout);
                    }
                }
                
                last_state = state;
            }
        }
        
        // Wait for interrupt on encoder B
        int ret_b = gpiod_line_event_wait(encoder_b, &(struct timespec){ .tv_sec = 0, .tv_nsec = 10000000 });
        if (ret_b > 0) {
            struct gpiod_line_event ev;
            gpiod_line_event_read(encoder_b, &ev);
            
            // Read current state of both pins
            int a = gpiod_line_get_value(encoder_a);
            int b = gpiod_line_get_value(encoder_b);
            int state = (a << 1) | b;
            
            if (state != last_state) {
                long long t = now_ms();
                
                // Debounce
                if (t - last_change_ms >= debounce_ms) {
                    int direction = 0;
                    
                    // Decode Gray code transitions for forward direction
                    if (last_state == 0b00 && state == 0b10) direction = 1;
                    else if (last_state == 0b10 && state == 0b11) direction = 1;
                    else if (last_state == 0b11 && state == 0b01) direction = 1;
                    else if (last_state == 0b01 && state == 0b00) direction = 1;
                    
                    // Decode Gray code transitions for backward direction
                    else if (last_state == 0b00 && state == 0b01) direction = -1;
                    else if (last_state == 0b01 && state == 0b11) direction = -1;
                    else if (last_state == 0b11 && state == 0b10) direction = -1;
                    else if (last_state == 0b10 && state == 0b00) direction = -1;
                    
                    if (direction != 0) {
                        last_change_ms = t;
                        
                        if (direction == 1) {
                            current_index = (current_index + 1) % num_messages;
                            printf("Scrolled forward to: %s\n", messages[current_index]);
                        } else {
                            current_index = (current_index - 1 + num_messages) % num_messages;
                            printf("Scrolled backward to: %s\n", messages[current_index]);
                        }
                        
                        // Update LCD display
                        lcd_set_cursor(0, 0);
                        lcd_print_padded(messages[current_index]);
                        lcd_set_cursor(1, 0);
                        lcd_print_padded(messages[(current_index + 1) % num_messages]);
                        
                        fflush(stdout);
                    }
                }
                
                last_state = state;
            }
        }
    }

    gpiod_line_release(led);
    gpiod_line_release(encoder_a);
    gpiod_line_release(encoder_b);
    gpiod_chip_close(chip);
    return 0;
}
