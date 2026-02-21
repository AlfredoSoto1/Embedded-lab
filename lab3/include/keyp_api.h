#ifndef KEYP_API_H
#define KEYP_API_H

/**
 * @file keyp_api.h
 * @brief 4x3 Matrix Keypad driver API
 * 
 * This header provides functions to control and read from a 4x3 matrix keypad
 * using GPIO interface with interrupt support.
 */

#include <gpiod.h>

/**
 * @brief Initializes the keypad
 * 
 * Sets up the GPIO lines for the keypad matrix. Columns are configured as
 * inputs with interrupts, rows are configured as outputs.
 * 
 * @param chip_arg The gpiod_chip containing the keypad GPIO lines
 * @param c1_line The GPIO line for column 1
 * @param c2_line The GPIO line for column 2
 * @param c3_line The GPIO line for column 3
 * @param r1_line The GPIO line for row 1
 * @param r2_line The GPIO line for row 2
 * @param r3_line The GPIO line for row 3
 * @param r4_line The GPIO line for row 4
 */
void keyp_init(struct gpiod_chip *chip_arg, 
              struct gpiod_line *c1_line, 
              struct gpiod_line *c2_line,
              struct gpiod_line *c3_line, 
              struct gpiod_line *r1_line,
              struct gpiod_line *r2_line, 
              struct gpiod_line *r3_line,
              struct gpiod_line *r4_line);

/**
 * @brief Scans the keypad to detect which key is pressed
 * 
 * Performs a scan of the keypad matrix to determine which key (if any)
 * is currently pressed.
 * 
 * @return The character of the pressed key, or '\0' if no key is pressed
 */
char keyp_scan(void);

/**
 * @brief Gets the column GPIO lines array
 * 
 * @return Pointer to the array of column GPIO lines
 */
struct gpiod_line** keyp_get_cols(void);

#endif // KEYP_API_H
