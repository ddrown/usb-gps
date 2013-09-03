#ifndef __LCD_H__
#define __LCD_H__

#define LCD_MAX_X 20
#define LCD_MAX_Y 4

void I2C_init();
void setup_LCD();

void LCD_home();
void LCD_clear();
void LCD_moveTo(uint8_t col, uint8_t row);
uint8_t LCD_print_char(uint8_t value);

#endif
