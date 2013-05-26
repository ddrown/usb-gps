#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "stm32f4xx_tim.h"

#include "mytimer.h"

__IO uint32_t TimingDelay;

void Timer_init() {
  TIM_TimeBaseInitTypeDef TIM_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_InitStructure.TIM_Prescaler = 8-1; // 84mhz clock / 8 = 10.5mhz
  TIM_InitStructure.TIM_Period = 0xFFFFFFFF; // 2^32 / 10.5mhz = ~409 seconds between wrapping
  TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  TIM_Cmd(TIM2, ENABLE);

  if (SysTick_Config(SystemCoreClock / 1000)) { 
    // error, give up and loop
    while (1);
  }
}

void TIM2_IRQHandler(void) {
  if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
    GPIO_ToggleBits(GPIOD, GPIO_Pin_13); // flash "counter wrap" LED
  }
}

void mainloop_timer() {
  if(TimingDelay == 0) {
    GPIO_ToggleBits(GPIOD, GPIO_Pin_14); // flash "alive" LED
    TimingDelay = 500;
  }
}

// conflicts with mainloop_timer
void Delay(__IO uint32_t msWait) { 
  TimingDelay = msWait;

  while(TimingDelay != 0);
}

// called by SysTick
void TimingDelay_Decrement(void) {
  if (TimingDelay != 0x00) { 
    TimingDelay--;
  }
}
