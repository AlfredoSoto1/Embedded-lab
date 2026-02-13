#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main(void) {
    const char *chipname = "gpiochip0"; // usually correct on Pi 5
    const unsigned int line_num = 17;   // BCM GPIO
    const int debounce_ms = 50;

    struct gpiod_chip *chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        return 1;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }

    // Request BOTH edges so we can print press/release
    // (Pressed = FALLING edge when using pull-up + button to GND)
    if (gpiod_line_request_both_edges_events(line, "button-irq") < 0) {
        perror("gpiod_line_request_both_edges_events");
        gpiod_chip_close(chip);
        return 1;
    }

    printf("Listening on %s line %u (GPIO%u). Ctrl+C to exit.\n",
           chipname, line_num, line_num);

    long long last_ms = 0;

    while (1) {
        // Wait up to 5 seconds; -1 means wait forever
        int ret = gpiod_line_event_wait(line, &(struct timespec){ .tv_sec = 5, .tv_nsec = 0 });
        if (ret < 0) {
            perror("gpiod_line_event_wait");
            break;
        }
        if (ret == 0) {
            // timeout (optional)
            continue;
        }

        struct gpiod_line_event ev;
        if (gpiod_line_event_read(line, &ev) < 0) {
            perror("gpiod_line_event_read");
            break;
        }

        long long t = now_ms();
        if (t - last_ms < debounce_ms) {
            continue; // debounce
        }
        last_ms = t;

        if (ev.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            printf("Pressed\n");
        } else if (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            printf("Released\n");
        }
        fflush(stdout);
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}
