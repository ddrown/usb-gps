#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "main.h"
#include "stm32f4xx_tim.h"

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"

// Private variables
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;
static __IO uint32_t TimingDelay;
static __IO uint32_t LastPPSTime;
static __IO uint8_t PendingPPSTime = 0;

// Private function prototypes
void init();

void Delay(__IO uint32_t nTime) { 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

void TimingDelay_Decrement(void) {
  if (TimingDelay != 0x00) { 
    TimingDelay--;
  }
}

int main(void) {
  uint32_t last;
  init();

  /*
   * Disable STDOUT buffering. Otherwise nothing will be printed
   * before a newline character or when the buffer is flushed.
   */
  setbuf(stdout, NULL);

  last = 0;
  while(1) {
    if(PendingPPSTime) {
      printf("PPS at %lu (%ld)\n", LastPPSTime, (LastPPSTime - last));
      last = LastPPSTime;
      PendingPPSTime = 0;
    }
    if(TimingDelay == 0) { // "alive" LED
      GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
      TimingDelay = 500;
    }
  }

  return 0;
}

void LED_init() {
  GPIO_InitTypeDef GPIO_InitStructure;

  // ---------- LEDS -------- //
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void PPS_init() {
  EXTI_InitTypeDef EXTI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // syscfg for EXTI
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  // connect PA1 to EXTI line1
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource1);
  // Configure EXTI Line1 
  EXTI_InitStructure.EXTI_Line = EXTI_Line1;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void Timer_init() {
  TIM_TimeBaseInitTypeDef TIM_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_InitStructure.TIM_Prescaler = 8-1; // 84mhz clock / 8 = 10.5mhz
  TIM_InitStructure.TIM_Period = 0xFFFFFFFF;
  TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  TIM_Cmd(TIM2, ENABLE);
}

void init() {
  LED_init();

  PPS_init();

  Timer_init();

  USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

  if (SysTick_Config(SystemCoreClock / 1000)) { 
    // error, give up and loop
    while (1);
  }
}

void TIM2_IRQHandler(void) {
  if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
    GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
  }
}

void EXTI1_IRQHandler(void) {
  if(EXTI_GetITStatus(EXTI_Line1) != RESET) {
    LastPPSTime = TIM_GetCounter(TIM2);
    PendingPPSTime = 1;
    // flash LED
    GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
    // Clear the EXTI line 1 pending bit
    EXTI_ClearITPendingBit(EXTI_Line1);
  }
}


/*
 * Dummy function to avoid compiler error
 */
void _init() {

}
