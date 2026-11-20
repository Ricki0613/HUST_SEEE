#ifndef __ADC_H
#define __ADC_H
#include "sys.h"

/*
 * 实验三任务2：50 Hz 正弦波在不同采样率下的峰峰值测量。
 * 接线：信号发生器输出 Vp-p=2 V、直流偏置 1.0 V 的 50 Hz 正弦波，
 *       输出端接 PA4（ADC1_IN4，板上 STM_DAC 多功能端口），地接开发板 GND。
 * 信号范围 0~2 V，满足 0~3.3 V 的单极性输入要求。
 * 采样由 TIM2 的更新事件（TRGO）硬件触发，实现严格等间隔采样。
 */
#define ADC_AC_PIN        GPIO_Pin_4
#define ADC_AC_CHANNEL    ADC_Channel_4
#define ADC_SAMPLE_POINTS 400    /* 采集点数，要求 >= 400 点 */

extern volatile u16 ADC_Buf[ADC_SAMPLE_POINTS];  /* 采样结果，右对齐码值 */
extern volatile u16 ADC_Count;                   /* 已采集点数 */
extern volatile u8  ADC_Done;                    /* 采满 400 点置 1 */

void ADC1_Init(void);          /* 12 位、右对齐、TIM2 TRGO 上升沿触发 */
void ADC1_StartCapture(void);  /* 清零计数并开 EOC 中断，等待定时器触发 */

#endif
