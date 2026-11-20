#ifndef __ADC_H
#define __ADC_H
#include "sys.h"

/*
 * 实验三任务1：8 位与 12 位分辨率比较。
 * 接线：直流稳压电源 1.00 V 接 PA5，即 ADC1 的通道 5（板上丝印 STM_ADC）。
 * PA5 同时连 TPAD，需把 P12 跳线帽拔掉，使该引脚只接外部电压。
 * 单极性输入，电压必须在 0~3.3 V 之间，且与开发板共地。
 */
#define ADC_DC_PIN      GPIO_Pin_5
#define ADC_DC_CHANNEL  ADC_Channel_5

void ADC1_Init(u32 resolution);  /* 分辨率：ADC_Resolution_8b 或 ADC_Resolution_12b */
u16  ADC1_Read(void);            /* 软件触发一次单次转换，返回右对齐码值 */

#endif
