/*
 * PWM Signal Generation — Lab 6.2.2
 *
 * Generates a 50% duty-cycle square wave at five frequencies using the
 * Raspberry Pi hardware PWM module through the Linux sysfs interface.
 *
 * Hardware setup:
 *   Add to /boot/firmware/config.txt:
 *       dtoverlay=pwm,pin=18,func=2      <- PWM0, GPIO 18
 *   Then reboot.  Connect oscilloscope probe to GPIO 18 (pin 12).
 *
 * Build: see CMakeLists.txt
 * Run:   sudo ./pwm_signal
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define PWM_CHIP    "/sys/class/pwm/pwmchip0"
#define PWM_CHANNEL 0

/* ------------------------------------------------------------------ helpers */

static int sysfs_write(const char *path, const char *val)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        perror(path);
        return -1;
    }
    int ret = write(fd, val, strlen(val));
    close(fd);
    return (ret < 0) ? -1 : 0;
}

static int sysfs_write_ul(const char *path, unsigned long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", val);
    return sysfs_write(path, buf);
}

/* ------------------------------------------------------------------ main */

int main(void)
{
    /* Frequencies required by the lab table */
    const unsigned int freqs[] = {500, 1000, 2000, 4000, 8000};
    const int num_freqs = (int)(sizeof(freqs) / sizeof(freqs[0]));

    char chan_dir[64], path[128];
    snprintf(chan_dir, sizeof(chan_dir), "%s/pwm%d", PWM_CHIP, PWM_CHANNEL);

    /* Export the PWM channel (skip if already exported) */
    if (access(chan_dir, F_OK) != 0)
    {
        char export_path[64], ch[4];
        snprintf(export_path, sizeof(export_path), "%s/export", PWM_CHIP);
        snprintf(ch, sizeof(ch), "%d", PWM_CHANNEL);
        if (sysfs_write(export_path, ch) < 0)
            return 1;
        usleep(100000); /* wait for sysfs entries to appear */
    }

    /* Disable PWM before changing period */
    snprintf(path, sizeof(path), "%s/enable", chan_dir);
    sysfs_write(path, "0");

    /* Print register-value table (Table 6.2) */
    printf("=== Table 6.2: Timer MCU values ===\n");
    printf("%-12s %-18s %-26s\n", "Freq (Hz)", "Period reg (ns)",
           "50%% DC reg (ns)");
    printf("%-12s %-18s %-26s\n", "----------", "---------------",
           "---------------");
    for (int i = 0; i < num_freqs; i++)
    {
        unsigned long period_ns = 1000000000UL / freqs[i];
        unsigned long dc_ns = period_ns / 2;
        printf("%-12u %-18lu %-26lu\n", freqs[i], period_ns, dc_ns);
    }

    printf("\nPress Enter to step through each frequency. Ctrl+C to quit.\n\n");

    for (int i = 0;; i = (i + 1) % num_freqs)
    {
        unsigned long period_ns = 1000000000UL / freqs[i];
        unsigned long dc_ns = period_ns / 2;

        /* Disable -> update period -> update duty cycle -> enable */
        snprintf(path, sizeof(path), "%s/enable", chan_dir);
        sysfs_write(path, "0");

        snprintf(path, sizeof(path), "%s/period", chan_dir);
        if (sysfs_write_ul(path, period_ns) < 0)
            return 1;

        snprintf(path, sizeof(path), "%s/duty_cycle", chan_dir);
        if (sysfs_write_ul(path, dc_ns) < 0)
            return 1;

        snprintf(path, sizeof(path), "%s/enable", chan_dir);
        sysfs_write(path, "1");

        printf("[%d/%d] %5u Hz | Period: %9lu ns | DC: %9lu ns (50%%)\n", i + 1,
               num_freqs, freqs[i], period_ns, dc_ns);
        printf("      Measure on oscilloscope, then press Enter...");
        fflush(stdout);

        getchar(); /* wait for user to record the measurement */
    }

    /* Cleanup */
    snprintf(path, sizeof(path), "%s/enable", chan_dir);
    sysfs_write(path, "0");

    char unexport_path[64], ch[4];
    snprintf(unexport_path, sizeof(unexport_path), "%s/unexport", PWM_CHIP);
    snprintf(ch, sizeof(ch), "%d", PWM_CHANNEL);
    sysfs_write(unexport_path, ch);

    return 0;
}
