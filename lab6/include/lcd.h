#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void lcd_init(int rs, int e, int d4, int d5, int d6, int d7);
void lcd_cmd(uint8_t cmd);
void lcd_char(char c);
void lcd_set_cursor(int row, int col);
void lcd_clear(void);
void lcd_print_padded(const char *s);

#endif // LCD_H