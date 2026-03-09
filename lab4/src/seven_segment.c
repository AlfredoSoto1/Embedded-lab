#define _GNU_SOURCE
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define CHIP "/dev/gpiochip4"

// Digit selector pins (0 = ON, 1 = OFF)
#define SEL_7S1 21
#define SEL_7S2 20

// BCD output pins to 74LS47 decoder
// Bit 0 (LSB) to Bit 3 (MSB)
#define OUT_1 26  // Bit 0 (A)
#define OUT_2 19  // Bit 1 (B)
#define OUT_3 13  // Bit 2 (C)
#define OUT_4 6   // Bit 3 (D)

#define MULTIPLEX_INTERVAL_US 2000  // 2ms per digit (500Hz refresh rate)
#define COUNTER_INTERVAL_MS 500     // Increment counter every 500ms

static struct gpiod_line *bcd_bit0, *bcd_bit1, *bcd_bit2, *bcd_bit3;
static struct gpiod_line *sel_7s1, *sel_7s2;

// Global state for interrupt handlers
static volatile unsigned char current_counter = 0;
static volatile int current_digit = 0;  // 0 = first digit, 1 = second digit
static volatile unsigned long multiplex_count = 0;

void setup_gpio(struct gpiod_chip *chip) {
    // Get BCD output lines
    bcd_bit0 = gpiod_chip_get_line(chip, OUT_1);
    bcd_bit1 = gpiod_chip_get_line(chip, OUT_2);
    bcd_bit2 = gpiod_chip_get_line(chip, OUT_3);
    bcd_bit3 = gpiod_chip_get_line(chip, OUT_4);
    
    // Get selector lines
    sel_7s1 = gpiod_chip_get_line(chip, SEL_7S1);
    sel_7s2 = gpiod_chip_get_line(chip, SEL_7S2);
    
    // Request all as outputs (start with BCD = 0, inverted for active low = all HIGH)
    gpiod_line_request_output(bcd_bit0, "bcd_bit0", 1);
    gpiod_line_request_output(bcd_bit1, "bcd_bit1", 1);
    gpiod_line_request_output(bcd_bit2, "bcd_bit2", 1);
    gpiod_line_request_output(bcd_bit3, "bcd_bit3", 1);
    
    gpiod_line_request_output(sel_7s1, "sel_7s1", 1); // Start disabled (OFF = 1)
    gpiod_line_request_output(sel_7s2, "sel_7s2", 1); // Start disabled (OFF = 1)
}

void set_bcd_output(unsigned char digit) {
    // Output 4-bit BCD value (0-9) to 74LS47 decoder
    // Active low: invert the bits (0 = HIGH, 1 = LOW)
    gpiod_line_set_value(bcd_bit0, ((digit >> 0) & 1));
    gpiod_line_set_value(bcd_bit1, ((digit >> 1) & 1));
    gpiod_line_set_value(bcd_bit2, ((digit >> 2) & 1));
    gpiod_line_set_value(bcd_bit3, ((digit >> 3) & 1));

    // printf("Counter: %02x \n", digit);
}

// Timer interrupt handler for multiplexing
void multiplex_timer_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)si;
    (void)uc;
    
    // current_counter = 88;

    // Extract tens and ones digits for decimal display (00-99)
    unsigned char tens_digit = current_counter / 10;
    unsigned char ones_digit = current_counter % 10;
    
    // set_bcd_output(tens_digit);
    // gpiod_line_set_value(sel_7s1, 0); // Turn on first digit (tens)
    // gpiod_line_set_value(sel_7s2, 1); // Turn on first digit (tens)
    // Turn off BOTH digits first to prevent ghosting
    gpiod_line_set_value(sel_7s1, 1);
    gpiod_line_set_value(sel_7s2, 1);
    
    if (current_digit == 0) {
        set_bcd_output(tens_digit);
        gpiod_line_set_value(sel_7s1, 0); // Turn on first digit (tens)
        current_digit = 1;
    } else {
        set_bcd_output(ones_digit);
        gpiod_line_set_value(sel_7s2, 0); // Turn on second digit (ones)
        current_digit = 0;
    }
    
    // Increment counter every COUNTER_INTERVAL_MS
    multiplex_count++;
    if (multiplex_count >= (COUNTER_INTERVAL_MS * 1000) / MULTIPLEX_INTERVAL_US) {
        current_counter++;
        if (current_counter > 99) {
            current_counter = 0;  // Wrap at 99
        }
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
    
    printf("7-Segment Counter: 00 to 99 (Decimal)\n");
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
