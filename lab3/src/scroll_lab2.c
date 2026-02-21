#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHIP "/dev/gpiochip4"
#define INPUT_PIN 21

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
    
    printf("Tachometer: Counting rising edges on GPIO %d\n", INPUT_PIN);
    printf("Press Ctrl+C to stop...\n\n");
    
    int counter = 0;

    while (1) {
        // Wait for rising edge event
        int ret = gpiod_line_event_wait(input, NULL);
        if (ret < 0) {
            perror("gpiod_line_event_wait");
            break;
        }
        
        if (ret > 0) {
            struct gpiod_line_event event;
            if (gpiod_line_event_read(input, &event) == 0) {
                if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
                    counter++;
                    printf("\rCount: %d", counter);
                    fflush(stdout);
                }
            }
        }
    }

    printf("\n");
    gpiod_line_release(input);
    gpiod_chip_close(chip);
    return 0;
}
