#ifndef __LCD_H__
#define __LCD_H__

void I2C_init();
void setup_LCD();

void LCD_home();
void LCD_clear();
void LCD_moveTo(uint8_t col, uint8_t row);
uint8_t LCD_print_char(uint8_t value);

#endif
