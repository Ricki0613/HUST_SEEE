#include "timer.h"

/*
 * TIM2 挂在 APB1：PCLK1=42 MHz，定时器时钟为 84 MHz。
 * 预分频固定为 84-1，计数频率 1 MHz，即每个计数 1 us。
 * 采样率只改 ARR：ARR+1 = 1 MHz / fs。
 *   1 kHz  -> ARR+1 = 1000 -> ARR = 999
 *   10 kHz -> ARR+1 = 100  -> ARR = 99
 */

#define TIM2_TIMER_CLOCK  84000000UL   /* TIM2 计数时钟 84 MHz */
#define TIM2_PRESCALER    (84 - 1)     /* 分频后 1 MHz */

void TIM2_SampleTimer_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;      /* 默认 1 kHz */
    TIM_TimeBaseStructure.TIM_Prescaler = TIM2_PRESCALER;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* 更新事件作为触发输出 TRGO，ADC 用它作为外部触发源。 */
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);

    TIM_Cmd(TIM2, DISABLE);   /* 先不跑，等 ADC 准备好再启动 */
}

void TIM2_SetSampleRate(u32 fs)
{
    /* ARR 无预装载，定时器停止时直接写立即生效 */
    TIM2->ARR = (TIM2_TIMER_CLOCK / (TIM2_PRESCALER + 1)) / fs - 1;
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
}

void TIM2_Start(void)
{
    TIM_SetCounter(TIM2, 0);
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_Stop(void)
{
    TIM_Cmd(TIM2, DISABLE);
}
