#include <stdlib.h>
#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"

#include "main.h"
#include "pps.h"
#include "mytimer.h"

// Private variables
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

// Private function prototypes
void init();

int main(void) {
  init();

  /*
   * Disable STDOUT buffering. Otherwise nothing will be printed
   * before a newline character or when the buffer is flushed.
   */
  setbuf(stdout, NULL);

  while(1) {
    mainloop_pps();
    mainloop_timer();
  }

  return 0;
}

void LED_init() {
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void init() {
  LED_init();

  PPS_init();

  Timer_init();

  USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
}

/*
 * Dummy function to avoid compiler error
 */
void _init() {

}
