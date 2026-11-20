#include "pwm.h"

/*
 * 参考标准例程实验9的 TIM14_PWM_Init，改成 TIM9 的通道1。
 * 改动要点：APB1 改为 APB2、GPIOF/PF9 改为 GPIOE/PE5、AF 改为 TIM9。
 * 本工程系统时钟为 168 MHz，APB2 分频为2，所以 PCLK2 为84 MHz，
 * TIM9 的定时器时钟为 2 * PCLK2 = 168 MHz，不能误用84 MHz计算 PSC。
 *
 * 计数频率 = 168 MHz / (PSC + 1)。本实验 PSC = 168 - 1，得到1 MHz。
 * 一个计数为1 us，ARR = 2000 - 1，周期为 (1999 + 1) * 1 us = 2 ms。
 * PWM1、向上计数、有效电平为高：CNT < CCR1 时输出高电平。
 * 因而 CCR1 = 500、15、1500 时，高电平分别为500、15、1500 us。
 * GPIO为3.3 V推挽输出，高电平的实际电压需用示波器确认。
 * 波形由定时器硬件产生，不需要在中断里翻转引脚，也不使用软件延时造波。
 */
void TIM9_PWM_Init(u16 arr, u16 psc, u16 pulse)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 1. 使能GPIOE和TIM9的时钟。TIM9挂在APB2总线上。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);

    /* 2. PE5选择AF3，即TIM9_CH1，配置为复用推挽输出。 */
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_TIM9);
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* 3. 初始化时基：向上计数，按给定PSC分频，计数到ARR后回到0。 */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler = psc;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM9, &TIM_TimeBaseStructure);

    /* 4. 初始化通道1。注意选高有效，才能直接用CCR1表示高电平宽度。 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = pulse;
    TIM_OC1Init(TIM9, &TIM_OCInitStructure);

    /* 5. 开启CCR1和ARR预装载，新比较值在下一个周期生效。 */
    TIM_OC1PreloadConfig(TIM9, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM9, ENABLE);
    TIM_GenerateEvent(TIM9, TIM_EventSource_Update);
    TIM_ClearFlag(TIM9, TIM_FLAG_Update);
    TIM_SetCounter(TIM9, 0);

    /* 6. 启动TIM9，此后即使主循环等待，PE5上的PWM也会连续输出。 */
    TIM_Cmd(TIM9, ENABLE);
}

void TIM9_PWM_SetPulse(u16 pulse)
{
    /* 只修改CCR1，ARR和PSC不变，所以周期一直保持2 ms。 */
    TIM_SetCompare1(TIM9, pulse);
}
