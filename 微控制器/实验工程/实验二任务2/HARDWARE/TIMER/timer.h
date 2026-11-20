#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

/* 中断和主程序共同使用的变量加volatile，保证每次都从内存读取。 */
extern volatile u8 TIM3_CaptureReady;  /* 1：本次测量结束，可读取结果 */
extern volatile u8 TIM3_CaptureError;  /* 1：捕获覆盖，丢弃这次数据 */
extern volatile u32 TIM3_HighWidthUs;  /* 最近一次实测高电平宽度，单位us */
extern volatile u32 TIM3_OverflowCount; /* 定时器累计溢出次数 */

void TIM3_CH2_Capture_Init(void);      /* PC7 / TIM3_CH2输入捕获初始化 */
void TIM3_Capture_Start(void);        /* 开始测量下一次完整的高电平 */
#endif
