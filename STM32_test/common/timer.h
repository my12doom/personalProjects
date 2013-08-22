#ifndef __TIMER_H__
#define __TIMER_H__

#include "stm32f10x.h"

// this library use TIM2 as its timer
// the total time of a single delay call is limited by 2^31-1 ticks (~29.82s on 72Mhz chip)

int init_timer(void);
int64_t gettick(void);
int64_t getus(void);
int delayms(int count);
int delayus(int count);
void delaytick(int count);

#endif
