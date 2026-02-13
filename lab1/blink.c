#include <gpiod.h>
#include <unistd.h>
#include <stdio.h>

#define GPIO_CHIP "/dev/gpiochip4"
#define LED_GPIO 4

int main(void)
{
    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    line = gpiod_chip_get_line(chip, LED_GPIO);
    if (!line) {
        perror("gpiod_chip_get_line");
        return 1;
    }

    if (gpiod_line_request_output(line, "led-blink", 0) < 0) {
        perror("gpiod_line_request_output");
        return 1;
    }

    while (1) {
        gpiod_line_set_value(line, 1);
        usleep(100000);               
        gpiod_line_set_value(line, 0); 
        usleep(100000);               
    }
}
