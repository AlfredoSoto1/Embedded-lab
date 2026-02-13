#include <gpiod.h>
#include <unistd.h>
#include <stdio.h>

#define GPIO_CHIP "/dev/gpiochip4"
#define LED_GPIO 4       
#define BUTTON_GPIO 17    

int main(void)
{
    struct gpiod_chip *chip;
    struct gpiod_line *led;
    struct gpiod_line *btn;

    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    led = gpiod_chip_get_line(chip, LED_GPIO);
    if (!led) { perror("get_line led"); return 1; }

    btn = gpiod_chip_get_line(chip, BUTTON_GPIO);
    if (!btn) { perror("get_line btn"); return 1; }

    if (gpiod_line_request_output(led, "led", 0) < 0) {
        perror("request_output led");
        return 1;
    }

    if (gpiod_line_request_input(btn, "button") < 0) {
        perror("request_input btn");
        return 1;
    }

    int last = -1;

    while (1) {
        int v = gpiod_line_get_value(btn);
        if (v < 0) { perror("get_value btn"); return 1; }

        if (v != last) {
            printf("Button value: %d\n", v);
            last = v;
        }

        gpiod_line_set_value(led, v);

        usleep(10000); // 10ms polling
    }
}
