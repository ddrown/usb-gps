// Many parts of this code were ported from Arduino's LiquidCrystal_I2C lib

#include "stm32f4xx_conf.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"

#include "lcd.h"
#include "mytimer.h"

#define I2C_LCD_ADDR 0x3f

#define LCD_FUNCTIONSET 0x20
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// cursor/display commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_SETDDRAMADDR 0x80

// text layout
#define LCD_ENTRYMODESET 0x04
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00 

// dedicated pins
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define LCD_SEND_En 1 << 2  // Enable bit
#define LCD_SEND_Rw 1 << 1  // Read/Write bit
#define LCD_SEND_Rs 1  // Register select bit

#define LCD_MAX_X 20
#define LCD_MAX_Y 4
static char current_lcd_state[LCD_MAX_X][LCD_MAX_Y];
// printing = where we want to print
// current = location of LCD's pointer
static uint8_t printing_lcd_x, printing_lcd_y, current_lcd_x, current_lcd_y;

// estimation: 168mhz and ~30 instructions per loop
#define ONE_MS 5600
static int8_t send_i2c_data(uint8_t data) {
  __IO uint32_t I2CTimeout;

  // wait for I2C3 to be ready
  I2CTimeout = ONE_MS;
  while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BUSY)) {
    if((I2CTimeout--) == 0) {
      return -1;
    }
  }

  // Send I2C3 start condition
  I2C_GenerateSTART(I2C3, ENABLE);

  // wait for ack of start condition
  I2CTimeout = ONE_MS;
  while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT)) {
    if((I2CTimeout--) == 0) {
      I2C_GenerateSTOP(I2C3, ENABLE);
      return -2;
    }
  }

  // send slave address for write
  I2C_Send7bitAddress(I2C3, I2C_LCD_ADDR << 1, I2C_Direction_Transmitter);

  // wait for address send
  I2CTimeout = ONE_MS;
  while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
    if((I2CTimeout--) == 0) {
      I2C_GenerateSTOP(I2C3, ENABLE);
      return -3;
    }
  }

  // -- write to i2c --
  I2C_SendData(I2C3, data);

  // wait for byte to finish
  I2CTimeout = ONE_MS;
  while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BTF) == RESET) {
    if((I2CTimeout--) == 0) {
      I2C_GenerateSTOP(I2C3, ENABLE);
      return -4;
    }
  }

  // -- stop i2c transaction
  I2C_GenerateSTOP(I2C3, ENABLE);

  return 1;
}

static int8_t send_LCD(uint8_t data) {
  int8_t error = send_i2c_data(data);
  if(error < 0) {
    //printf("E: %d\n", error);
    GPIO_SetBits(GPIOD, GPIO_Pin_15);
  }
  return error;
}

static int8_t send_halfbyte(uint8_t data) {
  if(send_LCD(data | LCD_SEND_En) < 0) {
    return -1;
  }
  DelayUS(1); // must be >450ns
  if(send_LCD(data & ~LCD_SEND_En) < 0) {
    return -1;
  }
  DelayUS(37); // commands need >37us to settle
  return 0;
}

static void send_byte(uint8_t data, uint8_t mode) {
  if(send_halfbyte( (data & 0xf0) | mode | LCD_BACKLIGHT) < 0) {
    return;
  }
  if(send_halfbyte( ((data<<4) & 0xf0) | mode | LCD_BACKLIGHT) < 0) {
    return;
  }
}

void I2C_init() {
  GPIO_InitTypeDef  GPIO_InitStructure;
  I2C_InitTypeDef  I2C_InitStructure;

  // turn on clocks
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); // GPIO C enable
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // GPIO A enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE); // I2C3 enable

  // physical pins configuration
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; // PA8=scl
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // alternate function mode
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 50mhz
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;   // output type: open drain
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // PC9=sda
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // alternate function mode
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 50mhz
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;   // output type: open drain
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // map physical pins to the i2c3 periph
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_I2C3); // PA8=scl
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_I2C3); // PC9=sda

  // configure I2C3
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;         // I2C mode (vs smbus)
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2; // duty cycle 2
  I2C_InitStructure.I2C_OwnAddress1 = 0x0;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 100000; // 100khz
  I2C_Init(I2C3, &I2C_InitStructure);

  // enable I2C3
  I2C_Cmd(I2C3, ENABLE);
}

void setup_LCD() {
  uint8_t i;

  // wait 50ms for LCD to come up
  Delay(50);

  // reset pins to all 0
  send_LCD(0);
  DelayUS(4500);

  for(i = 0; i < 3; i++) { // try three times to set LCD into a known state (re-sync half-byte stream)
    // start by setting 8 bit mode
    send_halfbyte(LCD_FUNCTIONSET|LCD_8BITMODE);
    DelayUS(4500); // wait a minimum of 4.1ms
  }

  // set in 4 bit mode
  send_halfbyte(LCD_FUNCTIONSET|LCD_4BITMODE);

  // set # of lines, font size, etc
  send_byte(LCD_FUNCTIONSET|LCD_2LINE|LCD_4BITMODE|LCD_5x8DOTS, 0);

  // TODO: does this need a modifier?
  // turn on display
  send_byte(LCD_BACKLIGHT|LCD_DISPLAYON|LCD_CURSOROFF|LCD_BLINKOFF, 0);

  LCD_clear();

  // left to right
  send_byte(LCD_ENTRYMODESET|LCD_ENTRYLEFT|LCD_ENTRYSHIFTDECREMENT, 0);

  LCD_home();

  LCD_print_char('B');
}

void LCD_home() {
  // top left corner
  send_byte(LCD_RETURNHOME, 0);
  current_lcd_x = current_lcd_y = printing_lcd_x = printing_lcd_y = 0;
  DelayUS(500); // moving to home takes awhile
}

void LCD_clear() {
  uint8_t x,y;
  // clear anything that might be on it
  send_byte(LCD_CLEARDISPLAY, 0);
  current_lcd_x = current_lcd_y = printing_lcd_x = printing_lcd_y = 0;
  for(x = 0; x < LCD_MAX_X; x++) {
   for(y = 0; y < LCD_MAX_Y; y++) {
     current_lcd_state[x][y] = ' ';
   }
  }
  DelayUS(1500); // clearing the display takes awhile
}

void LCD_moveTo(uint8_t col, uint8_t row) {
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  send_byte(LCD_SETDDRAMADDR | (col + row_offsets[row]), 0);
  current_lcd_x = printing_lcd_x = col;
  current_lcd_y = printing_lcd_y = row;
}

static void advance_cursor(uint8_t *x, uint8_t *y) {
  (*x)++;
  if(*x == LCD_MAX_X) {
    (*y)++;
    (*x) = 0;
  }
  if(*y == LCD_MAX_Y) {
    (*y) = 0;
  }
}

// around 700us - 800us per character
uint8_t LCD_print_char(uint8_t value) {
  if(current_lcd_state[printing_lcd_x][printing_lcd_y] != value) {
    if(current_lcd_x != printing_lcd_x || current_lcd_y != printing_lcd_y) {
      LCD_moveTo(printing_lcd_x, printing_lcd_y);
    }
    send_byte(value, LCD_SEND_Rs);
    current_lcd_state[current_lcd_x][current_lcd_y] = value;
    advance_cursor(&current_lcd_x, &current_lcd_y);
  }
  advance_cursor(&printing_lcd_x, &printing_lcd_y);
  return 1;
}
