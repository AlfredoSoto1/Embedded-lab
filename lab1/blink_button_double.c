#include <gpiod.h>
#include <unistd.h>
#include <stdio.h>

#define GPIO_CHIP "/dev/gpiochip4"
#define LED_GPIO_1 4        // BCM GPIO4  (pin 7)
#define LED_GPIO_2 27        // BCM GPIO4  (pin 7)
#define BUTTON_GPIO_1 17    // BCM GPIO17 (pin 11)
#define BUTTON_GPIO_2 22    // BCM GPIO17 (pin 11)

int main(void)
{
    struct gpiod_chip *chip;
    struct gpiod_line *led_1;
    struct gpiod_line *led_2;
    struct gpiod_line *btn_1;
    struct gpiod_line *btn_2;

    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    led_1 = gpiod_chip_get_line(chip, LED_GPIO_1);
    led_2 = gpiod_chip_get_line(chip, LED_GPIO_2);
    if (!led_1 || !led_2) { perror("get_line led"); return 1; }

    btn_1 = gpiod_chip_get_line(chip, BUTTON_GPIO_1);
    btn_2 = gpiod_chip_get_line(chip, BUTTON_GPIO_2);
    if (!btn_1 || !btn_2) { perror("get_line btn"); return 1; }

    if (gpiod_line_request_output(led_1, "led_1", 0) < 0) {
        perror("request_output led_1");
        return 1;
    }
    if (gpiod_line_request_output(led_2, "led_2", 0) < 0) {
        perror("request_output led_2");
        return 1;
    }

    if (gpiod_line_request_input(btn_1, "button_1") < 0) {
        perror("request_input btn_1");
        return 1;
    }
    if (gpiod_line_request_input(btn_2, "button_2") < 0) {
        perror("request_input btn_2");
        return 1;
    }

    int last_1 = -1;
    int last_2 = -1;

    gpiod_line_set_value(led_1, 0);
    gpiod_line_set_value(led_2, 0);

    while (1) {
        int v_1 = gpiod_line_get_value(btn_1);
        if (v_1 < 0) { perror("get_value btn_1"); return 1; }

        if (v_1 != last_1) {
            printf("Button 1 value: %d\n", v_1);
            last_1 = v_1;
        }

        gpiod_line_set_value(led_1, v_1);

        int v_2 = gpiod_line_get_value(btn_2);
        if (v_2 < 0) { perror("get_value btn_2"); return 1; }

        if (v_2 != last_2) {
            printf("Button 2 value: %d\n", v_2);
            last_2 = v_2;
        }

        gpiod_line_set_value(led_2, 1 - v_2);

        usleep(10000);
    }
}
