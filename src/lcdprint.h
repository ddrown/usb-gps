#ifndef __LCDPRINT_H__
#define __LCDPRINT_H__

uint8_t LCD_print_string(const char *str);
uint8_t LCD_print_uint(uint32_t number);
uint8_t LCD_print_int(int32_t number);
uint8_t LCD_spacepad(uint8_t thislen, uint8_t lastlen);

#endif
