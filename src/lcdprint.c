#include "stm32f4xx_conf.h"

#include "lcd.h"
#include "lcdprint.h"

uint8_t LCD_print_string(const char *str) {
  uint8_t len = 0;
  for(;str && *str; str++) {
    LCD_print_char(*str);
    len++;
  }
  return len;
}

#define MAX_INT_CHRS 12
uint8_t LCD_print_uint(uint32_t number) {
  char intstr[MAX_INT_CHRS];
  uint8_t p = MAX_INT_CHRS;

  intstr[p-1] = '\0';
  p--;

  if(number == 0) {
    intstr[p-1] = '0';
    p--;
  }

  while(number != 0 && p > 0) {
    intstr[p-1] = '0'+(number % 10);
    p--;
    number = number / 10;
  }
  return LCD_print_string(intstr+p);
}

uint8_t LCD_print_int(int32_t number) {
  char intstr[MAX_INT_CHRS];
  uint8_t negative = 0;
  uint8_t p = MAX_INT_CHRS;

  intstr[p-1] = '\0';
  p--;

  if(number == 0) {
    intstr[p-1] = '0';
    p--;
  }
  if(number < 0) {
    negative = 1;
  }

  while(number != 0 && p > 0) {
    if(number < 0) {
      intstr[p-1] = '0'-(number % 10);
    } else {
      intstr[p-1] = '0'+(number % 10);
    }
    p--;
    number = number / 10;
  }
  if(negative && p > 0) {
    intstr[p-1] = '-';
    p--;
  }
  return LCD_print_string(intstr+p);
}

uint8_t LCD_spacepad(uint8_t thislen, uint8_t lastlen) {
  if(thislen < lastlen) {
    lastlen = lastlen - thislen;
    while(lastlen > 0) {
      LCD_print_char(' ');
      lastlen--;
    }
  }
  return thislen;
}
