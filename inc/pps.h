#ifndef __PPS_H
#define __PPS_H

void mainloop_pps();
void PPS_init();

void before_usb_poll();
void after_usb_poll();

uint8_t clear_to_print();
uint32_t ms_since_last_pps();

void TIM2_CC2_IRQ();

#endif // __PPS_H
