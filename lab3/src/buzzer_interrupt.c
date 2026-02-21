#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define CHIP "/dev/gpiochip4"
#define LED_TEST 21
#define BUTTON_PIN 14

// Available frequencies to cycle through
static const int FREQUENCIES[] = {500, 1000, 1500, 2000, 3000};
static const int NUM_FREQUENCIES = 5;

// Global variables for signal handler
static struct gpiod_line *led = NULL;
static int state = 0;
static timer_t timerid;
static volatile int current_freq_index = 0;

// Timer interrupt handler
void timer_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)si;
    (void)uc;
    
    if (led) {
        // Toggle pin to generate square wave at FREQUENCY_HZ
        gpiod_line_set_value(led, state);
        state = !state;
    }
}

int main(void) {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    // Set up the LED line as output test
    led = gpiod_chip_get_line(chip, LED_TEST);
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
    
    // Set up signal handler for timer
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
    
    // Create a POSIX timer
    struct sigevent sev;
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;
    
    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1) {
        perror("timer_create");
        return 1;
    }
    
    // Configure timer for the initial frequency
    int current_freq = FREQUENCIES[current_freq_index];
    long interval_ns = 1000000000L / (2 * current_freq);
    
    struct itimerspec timer_spec;
    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = interval_ns;
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = interval_ns;
    
    if (timer_settime(timerid, 0, &timer_spec, NULL) == -1) {
        perror("timer_settime");
        return 1;
    }
    
    printf("Timer interrupt mode: Pin %d buzzer, Pin %d button\n", LED_TEST, BUTTON_PIN);
    printf("Current frequency: %dHz\n", current_freq);
    printf("Press button to cycle frequencies. Press Ctrl+C to stop...\n");

    // Monitor button presses
    while (1) {
        struct timespec timeout = {.tv_sec = 1, .tv_nsec = 0};
        int ret = gpiod_line_event_wait(button, &timeout);
        
        if (ret > 0) {
            struct gpiod_line_event event;
            if (gpiod_line_event_read(button, &event) == 0) {
                if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
                    // Cycle to next frequency
                    current_freq_index = (current_freq_index + 1) % NUM_FREQUENCIES;
                    current_freq = FREQUENCIES[current_freq_index];
                    
                    // Update timer with new frequency
                    interval_ns = 1000000000L / (2 * current_freq);
                    timer_spec.it_value.tv_sec = 0;
                    timer_spec.it_value.tv_nsec = interval_ns;
                    timer_spec.it_interval.tv_sec = 0;
                    timer_spec.it_interval.tv_nsec = interval_ns;
                    
                    if (timer_settime(timerid, 0, &timer_spec, NULL) == -1) {
                        perror("timer_settime");
                        break;
                    }
                    
                    printf("Frequency changed to: %dHz\n", current_freq);
                    usleep(50000); // Debounce delay
                }
            }
        }
    }

    timer_delete(timerid);
    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}
