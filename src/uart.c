#include <stdio.h>
#include <string.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"

#include "uart.h"
#include "mytimer.h"

#define LINE_MAX 80
#define LINES_COUNT 2

static char nmea_line[LINES_COUNT][LINE_MAX];
static uint8_t nmea_line_len[LINES_COUNT] = {0,0};
__IO static uint8_t nmea_line_active = 0;
__IO static uint8_t at_newline = 0;

static struct gps_status {
  uint8_t fix_type;
  uint8_t fix_sat_count;
  uint8_t sat_count;
  uint8_t avg_snr;
  uint8_t sats_above_10_dbm;
  uint16_t dbm_total;
  uint8_t day;
  uint8_t month;
  uint8_t year;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t valid;
  uint32_t time_timestamp;
  uint8_t done;
} gps_state;

void gps_data() {
  if(gps_state.done == 1) {
    printf("G %u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%lu\n", gps_state.fix_type, gps_state.fix_sat_count, gps_state.sat_count, gps_state.avg_snr, 
        gps_state.day, gps_state.month, gps_state.year, gps_state.hour, gps_state.minute, gps_state.second, gps_state.valid,
        gps_state.time_timestamp);
    gps_state.done = 0;
  }
}

// $GPGSA,A,3,23,19,13,03,,,,,,,,,3.83,3.70,0.99
static void handle_gpgsa(const char *line) {
  uint8_t seen_commas = 0;
  uint8_t last_comma_position = 0;

  gps_state.fix_type = 0;
  gps_state.fix_sat_count = 0;

  for(uint8_t i = 0; i < strlen(line); i++) {
    if(line[i] == ',') {
      seen_commas++;
      last_comma_position = i;
    } else if(line[i] == '*') {
      break;
    } else if(seen_commas == 2) {
      if(line[i] >= '0' && line[i] <= '9') {
        gps_state.fix_type = line[i] - '0' + gps_state.fix_type * 10;
      }
    } else if(seen_commas > 2 && seen_commas < 15) {
      if(line[i] >= '0' && line[i] <= '9' && last_comma_position == (i-1)) {
        gps_state.fix_sat_count++;
      }
    }
  }
}

// $GPGSV,1,1,02,32,,,37,31,,,29*77
static void handle_gpgsv(const char *line) {
  uint8_t seen_commas = 0;
  uint8_t last_comma_position = 0;
  uint8_t total_lines = 0;
  uint8_t this_line = 0;
  uint8_t dbm = 0;

  gps_state.sat_count = 0;

  for(uint8_t i = 0; i < strlen(line); i++) {
    if(line[i] == ',') {
      seen_commas++;
      last_comma_position = i;
    } else if(line[i] == '*') {
      break;
    } else if(seen_commas == 1) {
      if(line[i] >= '0' && line[i] <= '9') {
        total_lines = line[i] - '0' + total_lines*10;
      }
    } else if(seen_commas == 2) {
      if(line[i] >= '0' && line[i] <= '9') {
        this_line = line[i] - '0' + this_line*10;
      }
    } else if(seen_commas == 3) {
      if(this_line == 1) {
        gps_state.avg_snr = 0;
        gps_state.dbm_total = 0;
        gps_state.sats_above_10_dbm = 0;
      }
      if(line[i] >= '0' && line[i] <= '9') {
        gps_state.sat_count = line[i] - '0' + gps_state.sat_count*10;
      }
    } else if(seen_commas > 3 && (seen_commas - 4) % 4 == 3) {
      if(last_comma_position == i-1) {
        if(dbm > 10) {
          gps_state.dbm_total += dbm;
          gps_state.sats_above_10_dbm++;
        }
        dbm = 0;
      }
      if(line[i] >= '0' && line[i] <= '9') {
        dbm = line[i] - '0' + dbm*10;
      }
    }
  }

  if(dbm > 10) {
    gps_state.dbm_total += dbm;
    gps_state.sats_above_10_dbm++;
  }
 
  if(this_line == total_lines && gps_state.sats_above_10_dbm > 0) {
    gps_state.avg_snr = gps_state.dbm_total / gps_state.sats_above_10_dbm;
  }
}

// $GPRMC,164013.006,V,,,,,0.00,162.21,230513,,,N*4A
static void handle_gprmc(const char *line) {
  uint8_t seen_commas = 0;
  uint8_t last_comma_position = 0;

  gps_state.day = gps_state.month = gps_state.year = 0;
  gps_state.hour = gps_state.minute = gps_state.second = 0;
  gps_state.valid = 0;
  gps_state.time_timestamp = TIM_GetCounter(TIM2);

  for(uint8_t i = 0; i < strlen(line); i++) {
    if(line[i] == ',') {
      seen_commas++;
      last_comma_position = i;
    } else if(line[i] == '*') {
      break;
    } else if(seen_commas == 1) {
      uint8_t *this_counter = NULL;
      if(i - last_comma_position < 3) {
        this_counter = &gps_state.day;
      } else if(i - last_comma_position < 5) {
        this_counter = &gps_state.month;
      } else if(i - last_comma_position < 7) {
        this_counter = &gps_state.year;
      }
      if(this_counter && line[i] >= '0' && line[i] <= '9') {
        *this_counter = line[i] - '0' + *this_counter * 10;
      }
    } else if(seen_commas == 2) {
      if(line[i] != 'V') {
        gps_state.valid = 1;
      }
    } else if(seen_commas == 9) {
      uint8_t *this_counter = NULL;
      if(i - last_comma_position < 3) {
        this_counter = &gps_state.hour;
      } else if(i - last_comma_position < 5) {
        this_counter = &gps_state.minute;
      } else if(i - last_comma_position < 7) {
        this_counter = &gps_state.second;
      }
      if(this_counter && line[i] >= '0' && line[i] <= '9') {
        *this_counter = line[i] - '0' + *this_counter * 10;
      }
    }
  }

  gps_state.done = 1;
}

// $GPGGA,164013.006,,,,,0,0,,,M,,M,,*4F
static void handle_gpgga(const char *line) {
}

void mainloop_uart() {
  if(at_newline) {
    int8_t line_completed = nmea_line_active - 1;
    if(line_completed < 0)
      line_completed = LINES_COUNT-1;
    at_newline = 0;
    if(strncmp("$GPGSA,", nmea_line[line_completed], 7) == 0) {
      handle_gpgsa(nmea_line[line_completed]);
    } else if(strncmp("$GPGSV,", nmea_line[line_completed], 7) == 0) {
      handle_gpgsv(nmea_line[line_completed]);
    } else if(strncmp("$GPRMC,", nmea_line[line_completed], 7) == 0) {
      handle_gprmc(nmea_line[line_completed]);
    } else if(strncmp("$GPGGA,", nmea_line[line_completed], 7) == 0) {
      handle_gpgga(nmea_line[line_completed]);
    }
  }
}

// configures USART2 for PA2=TX,PA3=RX 8N1 and interrupt on RX
void UART_init() {
  GPIO_InitTypeDef  GPIO_InitStructure;
  USART_InitTypeDef USART_InitStruct;  // usart
  NVIC_InitTypeDef NVIC_InitStructure; // interrupt controller

  memset(&gps_state, '\0', sizeof(gps_state));

  // turn on clocks
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // GPIOA enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // USART2 enable

  // physical pins configuration
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // alternate function mode
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // alternate function mode
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // map physical pins to the usart1 periph
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); // TX
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); // RX

  // configure USART2
  USART_InitStruct.USART_BaudRate = 9600;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_Parity = USART_Parity_No;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(USART2, &USART_InitStruct);

  // enable receive interrupt -- USART2_IRQHandler() function
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  // turn on USART2 peripheral
  USART_Cmd(USART2, ENABLE);
}

// read(interrupt)
void USART2_IRQHandler() {
  if(USART_GetITStatus(USART2, USART_IT_RXNE)) {
    int t = USART_ReceiveData(USART2);
    GPIO_ToggleBits(GPIOD, GPIO_Pin_13); // toggle "serial data" LED
    if(nmea_line_len[nmea_line_active] < LINE_MAX-2) {
      uint8_t len = nmea_line_len[nmea_line_active];
      nmea_line[nmea_line_active][len] = t;
      nmea_line[nmea_line_active][len + 1] = '\0';
      nmea_line_len[nmea_line_active] = len + 1;
    }
    if(t == '\n') {
      nmea_line_active = (nmea_line_active + 1) % LINES_COUNT;
      at_newline = 1;
      nmea_line_len[nmea_line_active] = 0;
      nmea_line[nmea_line_active][0] = '\0';
    }
  }
}

// write - blocking
void putchar_usart2(int c) {
  // wait till data register is empty
  while(!USART_GetFlagStatus(USART2, USART_FLAG_TXE));
  USART_SendData(USART2, c);
}
