#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

#define CHIP "/dev/gpiochip4"
#define LED_TEST 21
#define BUTTON_PIN 14

// Available frequencies to cycle through
static const int FREQUENCIES[] = {500, 1000, 1500, 2000, 3000};
static const int NUM_FREQUENCIES = 5;

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
        perror("led"); 
        return 1; 
    }
    
    // Set up button as input with edge detection
    struct gpiod_line *button = gpiod_chip_get_line(chip, BUTTON_PIN);
    if (!button) {
        perror("gpiod_chip_get_line button");
        gpiod_chip_close(chip);
        return 1;
    }
    
    if (gpiod_line_request_falling_edge_events(button, "button") < 0) {
        perror("gpiod_line_request_falling_edge_events");
        return 1;
    }
    
    int freq_index = 0;
    int current_freq = FREQUENCIES[freq_index];
    long interval_us = 1000000L / (2 * current_freq);
    
    printf("Polling mode: Pin %d buzzer, Pin %d button\n", LED_TEST, BUTTON_PIN);
    printf("Current frequency: %dHz\n", current_freq);
    printf("Press button to cycle frequencies. Press Ctrl+C to stop...\n");
    
    int state = 0;
    struct timespec timeout = {.tv_sec = 0, .tv_nsec = 0};

    while (1) {
        // Check for button press (non-blocking)
        int ret = gpiod_line_event_wait(button, &timeout);
        if (ret > 0) {
            struct gpiod_line_event event;
            if (gpiod_line_event_read(button, &event) == 0) {
                if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
                    // Cycle to next frequency
                    freq_index = (freq_index + 1) % NUM_FREQUENCIES;
                    current_freq = FREQUENCIES[freq_index];
                    interval_us = 1000000L / (2 * current_freq);
                    printf("Frequency changed to: %dHz\n", current_freq);
                    usleep(50000); // Debounce delay
                }
            }
        }
        
        // Toggle pin to generate square wave
        gpiod_line_set_value(led, state);
        state = !state;
        
        // Poll: sleep for the calculated interval
        usleep(interval_us);
    }

    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}
