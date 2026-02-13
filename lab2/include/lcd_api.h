#ifndef LCD_API_H
#define LCD_API_H

/**
 * @file lcd_api.h
 * @brief LCD display driver API for character LCD displays
 * 
 * This header provides functions to control and write to a character LCD
 * display using GPIO interface. Supports basic operations like initialization,
 * cursor positioning, character/string output, and display clearing.
 */

#include <gpiod.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Writes 4 bits of data to the LCD
 * 
 * Sends a nibble (4 bits) to the LCD data pins in 4-bit mode operation.
 * This is a low-level function used for LCD communication protocol.
 * 
 * @param v The 4-bit value to write (uses lower 4 bits)
 */
void write4(uint8_t v);

/**
 * @brief Sends a command to the LCD
 * 
 * Writes a command byte to the LCD controller (e.g., clear display,
 * cursor control, display mode). Commands are sent with RS pin low.
 * 
 * @param cmd The command byte to send to the LCD
 */
void lcd_cmd(uint8_t cmd);

/**
 * @brief Displays a single character on the LCD
 * 
 * Writes one character at the current cursor position. The cursor
 * automatically advances after writing.
 * 
 * @param c The character to display
 */
void lcd_char(char c);

/**
 * @brief Sets the cursor position on the LCD
 * 
 * Moves the cursor to the specified row and column. Subsequent writes
 * will appear at this position.
 * 
 * @param row The row number (typically 0 or 1 for 2-line displays)
 * @param col The column number (0-15 for 16-character displays)
 */
void lcd_set_cursor(int row, int col);

/**
 * @brief Clears the LCD display
 * 
 * Erases all characters from the display and returns the cursor to
 * the home position (0, 0).
 */
void lcd_clear(void);

/**
 * @brief Prints a string with padding to fill the line
 * 
 * Displays a string on the LCD and pads the remainder of the line with
 * spaces to clear any previous content.
 * 
 * @param s The null-terminated string to display
 */
void lcd_print_padded(const char *s);

/**
 * @brief Initializes the LCD display
 * 
 * Performs the initialization sequence for the LCD including setting
 * 4-bit mode, display configuration, and clearing the screen. Must be
 * called before any other LCD functions.
 * 
 * @param chip The gpiod_chip containing the LCD GPIO lines
 * @param rs The GPIO line for the Register Select (RS) pin
 * @param e The GPIO line for the Enable (E) pin
 * @param d4 The GPIO line for the D4 data pin
 * @param d5 The GPIO line for the D5 data pin
 * @param d6 The GPIO line for the D6 data pin  
 * @param d7 The GPIO line for the D7 data pin
 */
void lcd_init(struct gpiod_chip *chip, struct gpiod_line *rs, struct gpiod_line *e,
              struct gpiod_line *d4, struct gpiod_line *d5,
              struct gpiod_line *d6, struct gpiod_line *d7);

#endif // LCD_API_H