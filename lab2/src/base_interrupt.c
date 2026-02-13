#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define CHIP "/dev/gpiochip4"

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main(void) {
    const int debounce_ms = 50;

    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    struct gpiod_line *led = gpiod_chip_get_line(chip, 21);
    if (!led) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(led, "led", 0) < 0) { perror("led"); return 1; }


    while (1) {
        gpiod_line_set_value(led, 1);
        usleep(500000); // 500ms

        gpiod_line_set_value(led, 0);
        usleep(500000); // 500ms
    }

    // if (gpiod_line_request_both_edges_events(led, "btn") < 0) {
    //     perror("gpiod_line_request_both_edges_events");
    //     gpiod_chip_close(chip);
    //     return 1;
    // }
    
    //  if (gpiod_line_request_falling_edge_events(led, "btn") < 0) {
    //     perror("gpiod_line_request_falling_edge_events");
    //     gpiod_chip_close(chip);
    //     return 1;
    // }

    // if (gpiod_line_request_rising_edge_events(led, "btn") < 0) {
    //     perror("gpiod_line_request_rising_edge_events");
    //     gpiod_chip_close(chip);
    //     return 1;
    // }

    // long long last_ms = 0;

    // while (1) {
    //     // Wait up to 5 seconds; -1 means wait forever
    //     int ret = gpiod_line_event_wait(led, &(struct timespec){ .tv_sec = 5, .tv_nsec = 0 });
    //     if (ret < 0) {
    //         perror("gpiod_line_event_wait");
    //         break;
    //     }
    //     if (ret == 0) {
    //         // timeout (optional)
    //         continue;
    //     }

    //     struct gpiod_line_event ev;
    //     if (gpiod_line_event_read(led, &ev) < 0) {
    //         perror("gpiod_line_event_read");
    //         break;
    //     }

    //     long long t = now_ms();
    //     if (t - last_ms < debounce_ms) {
    //         continue; // debounce
    //     }
    //     last_ms = t;

    //     if (ev.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
    //         printf("Pressed\n");
    //     } else if (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
    //         printf("Released\n");
    //     }
    //     fflush(stdout);
    // }

    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}
