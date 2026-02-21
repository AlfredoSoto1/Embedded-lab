#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define CHIP "/dev/gpiochip4"
#define INPUT_PIN 21
#define MEASUREMENT_PERIOD_MS 1000  // Measure RPM over 1 second
#define PULSES_PER_REVOLUTION 6     // 6 pulses = 1 full rotation

int main(void) {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    // Set up GPIO 21 as input with rising edge detection
    struct gpiod_line *input = gpiod_chip_get_line(chip, INPUT_PIN);
    if (!input) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }
    
    if (gpiod_line_request_rising_edge_events(input, "input") < 0) {
        perror("gpiod_line_request_rising_edge_events");
        gpiod_chip_close(chip);
        return 1;
    }
    
    printf("RPM Tachometer: Measuring wheel speed on GPIO %d\n", INPUT_PIN);
    printf("Six pulses per revolution\n");
    printf("Press Ctrl+C to stop...\n\n");
    
    int pulse_count = 0;
    int total_pulses = 0;
    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (1) {
        // Wait for rising edge event with timeout
        struct timespec timeout = {.tv_sec = 0, .tv_nsec = 100000000}; // 100ms timeout
        int ret = gpiod_line_event_wait(input, &timeout);
        
        if (ret < 0) {
            perror("gpiod_line_event_wait");
            break;
        }
        
        if (ret > 0) {
            struct gpiod_line_event event;
            if (gpiod_line_event_read(input, &event) == 0) {
                if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
                    pulse_count++;
                    total_pulses++;
                }
            }
        }
        
        // Check if measurement period has elapsed
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000;
        
        if (elapsed_ms >= MEASUREMENT_PERIOD_MS) {
            // Calculate RPM: (pulses / pulses_per_rev) * (60000ms / period_ms)
            double revolutions = (double)pulse_count / PULSES_PER_REVOLUTION;
            double rpm = (revolutions * 60000.0) / elapsed_ms;
            double total_revolutions = (double)total_pulses / PULSES_PER_REVOLUTION;

            printf("\rRPM: %.1f | Total Revolutions: %.1f   ", rpm, total_revolutions);
            fflush(stdout);
            
            // Reset for next measurement period
            pulse_count = 0;
            start_time = current_time;
        }
    }

    printf("\n");
    gpiod_line_release(input);
    gpiod_chip_close(chip);
    return 0;
}
