#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define CHIP "/dev/gpiochip4"

// Digit selector pins (0 = ON, 1 = OFF)
#define SEL_7S1 14
#define SEL_7S2 15

// Segment pins (0 = ON, 1 = OFF)
#define SEG_A 26
#define SEG_B 19
#define SEG_C 13
#define SEG_D 6
#define SEG_E 5
#define SEG_F 21
#define SEG_G 20

#define MULTIPLEX_INTERVAL_US 2000  // 2ms per digit (500Hz refresh rate)
#define COUNTER_INTERVAL_MS 500     // Increment counter every 500ms

// 7-segment patterns for hex digits 0-F
// Bit order: GFEDCBA (bit 0 = A, bit 6 = G)
// Inverted: 0 = ON, 1 = OFF
static const unsigned char HEX_PATTERNS[16] = {
    0b1000000,  // 0
    0b1111001,  // 1
    0b0100100,  // 2
    0b0110000,  // 3
    0b0011001,  // 4
    0b0010010,  // 5
    0b0000010,  // 6
    0b1111000,  // 7
    0b0000000,  // 8
    0b0010000,  // 9
    0b0001000,  // A
    0b0000011,  // B
    0b1000110,  // C
    0b0100001,  // D
    0b0000110,  // E
    0b0001110   // F
};

static struct gpiod_line *seg_a, *seg_b, *seg_c, *seg_d, *seg_e, *seg_f, *seg_g;
static struct gpiod_line *sel_7s1, *sel_7s2;

// Global state for interrupt handlers
static volatile unsigned char current_counter = 0;
static volatile int current_digit = 0;  // 0 = first digit, 1 = second digit
static volatile unsigned long multiplex_count = 0;

void setup_gpio(struct gpiod_chip *chip) {
    // Get segment lines
    seg_a = gpiod_chip_get_line(chip, SEG_A);
    seg_b = gpiod_chip_get_line(chip, SEG_B);
    seg_c = gpiod_chip_get_line(chip, SEG_C);
    seg_d = gpiod_chip_get_line(chip, SEG_D);
    seg_e = gpiod_chip_get_line(chip, SEG_E);
    seg_f = gpiod_chip_get_line(chip, SEG_F);
    seg_g = gpiod_chip_get_line(chip, SEG_G);
    
    // Get selector lines
    sel_7s1 = gpiod_chip_get_line(chip, SEL_7S1);
    sel_7s2 = gpiod_chip_get_line(chip, SEL_7S2);
    
    // Request all as outputs (start with segments OFF = 1)
    gpiod_line_request_output(seg_a, "seg_a", 1);
    gpiod_line_request_output(seg_b, "seg_b", 1);
    gpiod_line_request_output(seg_c, "seg_c", 1);
    gpiod_line_request_output(seg_d, "seg_d", 1);
    gpiod_line_request_output(seg_e, "seg_e", 1);
    gpiod_line_request_output(seg_f, "seg_f", 1);
    gpiod_line_request_output(seg_g, "seg_g", 1);
    
    gpiod_line_request_output(sel_7s1, "sel_7s1", 1); // Start disabled (OFF = 1)
    gpiod_line_request_output(sel_7s2, "sel_7s2", 1); // Start disabled (OFF = 1)
}

void set_segments(unsigned char pattern) {
    gpiod_line_set_value(seg_a, ((pattern >> 0) & 1));
    gpiod_line_set_value(seg_b, ((pattern >> 1) & 1));
    gpiod_line_set_value(seg_c, ((pattern >> 2) & 1));
    gpiod_line_set_value(seg_d, ((pattern >> 3) & 1));
    gpiod_line_set_value(seg_e, ((pattern >> 4) & 1));
    gpiod_line_set_value(seg_f, ((pattern >> 5) & 1));
    gpiod_line_set_value(seg_g, ((pattern >> 6) & 1));
}

// Timer interrupt handler for multiplexing
void multiplex_timer_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)si;
    (void)uc;
    
    unsigned char high_nibble = (current_counter >> 4) & 0x0F;
    unsigned char low_nibble = current_counter & 0x0F;
    
    // Turn off BOTH digits first to prevent ghosting
    gpiod_line_set_value(sel_7s1, 1);
    gpiod_line_set_value(sel_7s2, 1);
    
    // Clear all segments (turn them all OFF)
    // set_segments(0b1111111);
    // set_segments(0b0000000);
    
    if (current_digit == 0) {
        // Display high nibble on first digit (7S1)
        set_segments(HEX_PATTERNS[high_nibble]);
        gpiod_line_set_value(sel_7s1, 0); // Turn on first digit
        current_digit = 1;
    } else {
        // Display low nibble on second digit (7S2)
        set_segments(HEX_PATTERNS[low_nibble]);
        gpiod_line_set_value(sel_7s2, 0); // Turn on second digit
        current_digit = 0;
    }
    
    // Increment counter every COUNTER_INTERVAL_MS
    multiplex_count++;
    if (multiplex_count >= (COUNTER_INTERVAL_MS * 1000 / MULTIPLEX_INTERVAL_US)) {
        current_counter++;
        multiplex_count = 0;
    }
}

int main(void) {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }
    
    setup_gpio(chip);

    // while(1){
    //     usleep(1000000); // Sleep for 1 second
    // }
    
    printf("7-Segment Counter: 00 to FF\n");
    printf("Press Ctrl+C to stop...\n\n");
    
    // Setup signal handler for multiplex timer
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = multiplex_timer_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        gpiod_chip_close(chip);
        return 1;
    }
    
    // Create multiplex timer
    timer_t multiplex_timer_id;
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &multiplex_timer_id;
    
    if (timer_create(CLOCK_REALTIME, &sev, &multiplex_timer_id) == -1) {
        perror("timer_create");
        gpiod_chip_close(chip);
        return 1;
    }
    
    // Start multiplex timer (500Hz = 2ms per interrupt)
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = MULTIPLEX_INTERVAL_US * 1000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = MULTIPLEX_INTERVAL_US * 1000;
    
    if (timer_settime(multiplex_timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        gpiod_chip_close(chip);
        return 1;
    }
    
    // Main loop - just wait for interrupts
    while (1) {
        pause();
    }
    
    gpiod_chip_close(chip);
    return 0;
}
