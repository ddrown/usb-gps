#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"

#include "pps.h"
#include "usbd_cdc_vcp.h"
#include "uart.h"
#include "lcd.h"
#include "lcdprint.h"

static __IO uint32_t LastPPSTime;
static __IO uint8_t PendingPPSTime = 0;
static __IO uint8_t gps_lcd_print_go = 0;

void mainloop_pps() {
  if(gps_lcd_print_go) {
    gps_lcd_print();
    gps_lcd_print_go = 0;
  }
}

static uint32_t start_usb;
static uint16_t pending_usb_time = 0;
static uint16_t last_usb_time = 0;
void before_usb_poll() {

  if(PendingPPSTime) {
    start_usb = TIM_GetCounter(TIM2);
    VCP_send_buffer((uint8_t *)"P", 1);
    last_usb_time = pending_usb_time;
    pending_usb_time = 1;
    PendingPPSTime = 0;
  } else if(pending_usb_time == 250) {
    start_usb = TIM_GetCounter(TIM2);
    VCP_send_buffer((uint8_t *)"P", 1);
    pending_usb_time++;
  } else {
    if(pending_usb_time < 60000) { // avoid wrapping
      pending_usb_time++;
    }
  }
}

static uint32_t hz;
void after_usb_poll() {
  static uint32_t pps_before_that = 0;
  if(pending_usb_time == 1) {
    int32_t diff = TIM_GetCounter(TIM2) - start_usb;
    hz = LastPPSTime-pps_before_that;
    printf(" %lu %lu %ld %ld %u\n", LastPPSTime, start_usb, hz, diff, last_usb_time);
    pps_before_that = LastPPSTime;

    gps_lcd_print_go = 1;
  } else if(pending_usb_time == 251) {
    int32_t diff = TIM_GetCounter(TIM2) - start_usb;
    printf(" %lu %ld\n",start_usb, diff);
    GPIO_ResetBits(GPIOD, GPIO_Pin_12);
  }
}

uint32_t ms_since_last_pps() {
  uint32_t diff = TIM_GetCounter(TIM2) - LastPPSTime;
  return diff / (hz/1000);
}

// reserve a 25 SOF (~50ms) window for the PPS messages
uint8_t clear_to_print() {
  return pending_usb_time < 225 || (pending_usb_time > 275 && pending_usb_time < 525) || pending_usb_time > 510;
}

void TIM2_CC2_IRQ() {
  LastPPSTime = TIM_GetCapture2(TIM2);
  PendingPPSTime = 1;
  // flash LED
  GPIO_SetBits(GPIOD, GPIO_Pin_12);
}

void PPS_init() {
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2); // TIM2_CH2

  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x00;

  TIM_ICInit(TIM2, &TIM_ICInitStructure);

  TIM_ITConfig(TIM2, TIM_IT_CC2, ENABLE);
}
