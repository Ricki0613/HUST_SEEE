#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

/*
 * 实验三任务2/3：TIM2 只做采样定时器，不产生中断。
 * 更新事件作为 TRGO 送给 ADC，实现等间隔硬件触发采样。
 * 采样率 fs = TimerCLK / ((PSC+1)(ARR+1)) = 84 MHz / ((83+1)(ARR+1))
 */
void TIM2_SampleTimer_Init(void);
void TIM2_SetSampleRate(u32 fs);   /* fs 单位 Hz，例如 1000、10000 */
void TIM2_Start(void);
void TIM2_Stop(void);

#endif
