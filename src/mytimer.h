#ifndef __MYTIMER_H
#define __MYTIMER_H

extern __IO uint32_t TimingDelay;

void Timer_init();
void mainloop_timer();
void Delay(__IO uint32_t msWait);
void DelayUS(__IO uint32_t usWait);
void TimingDelay_Decrement(void);

#endif // __MYTIMER_H
