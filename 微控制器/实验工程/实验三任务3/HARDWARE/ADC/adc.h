#ifndef __ADC_H
#define __ADC_H
#include "sys.h"

/*
 * 实验三任务3：单 ADC 扫描与双独立 ADC 采样（二选一）
 * 本工程采用“方式B”：ADC1、ADC2 独立模式，不使用双重 ADC 同步。
 *   ADC1 的通道5（PA5，板上 STM_ADC）测 1.00 V 直流；
 *   ADC2 的通道4（PA4，板上 STM_DAC 多功能端口）测 50 Hz 正弦波。
 * 要求：12 位、右对齐、采样率 10 kHz、每通道采集点数 >= 400。
 * 两个 ADC 都由 TIM2 的 TRGO 触发，各自转换自己的通道，互不等待。
 * 说明：本板 PA1(CH1) 已接 LAN8720 的 50 MHz REFCLK，PA0(CH0) 接 WK_UP 按键，
 *       不能直接做模拟输入，所以改用多功能端口上的 PA5、PA4 两个引脚。
 */

#define ADC_DC_PIN        GPIO_Pin_5
#define ADC_DC_CHANNEL    ADC_Channel_5     /* ADC1_IN5 */
#define ADC_AC_PIN        GPIO_Pin_4
#define ADC_AC_CHANNEL    ADC_Channel_4     /* ADC2_IN4 */
#define ADC_SAMPLE_POINTS 400               /* 每通道采集点数，要求 >= 400 */

extern volatile u16 ADC_DC_Buf[ADC_SAMPLE_POINTS];  /* ADC1：直流通道数据 */
extern volatile u16 ADC_AC_Buf[ADC_SAMPLE_POINTS];  /* ADC2：交流通道数据 */
extern volatile u16 ADC_DC_Count;
extern volatile u16 ADC_AC_Count;
extern volatile u8  ADC_AllDone;                    /* 两路都采满置 1 */

void ADC1_ADC2_Init(void);   /* 两个 ADC 独立模式、12 位、右对齐、T2_TRGO 触发 */
void ADC_StartCapture(void); /* 清零计数，开两路 EOC 中断，等待定时器触发 */

#endif
