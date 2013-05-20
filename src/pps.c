#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"

#include "pps.h"

static __IO uint32_t LastPPSTime;
static __IO uint8_t PendingPPSTime = 0;

void mainloop_pps() {
  static uint32_t last = 0;

  if(PendingPPSTime) {
    printf("PPS at %lu (%ld)\n", LastPPSTime, (LastPPSTime - last));
    last = LastPPSTime;
    PendingPPSTime = 0;
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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
