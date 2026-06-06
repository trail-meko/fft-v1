#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "mydefine.h"
void scheduler_init(void);
void scheduler_run(void);
void TIM2_Init(void);
void TIM3_Init(uint32_t frequency);
void TIM4_Init_Freq(uint32_t freq_hz);
#endif
