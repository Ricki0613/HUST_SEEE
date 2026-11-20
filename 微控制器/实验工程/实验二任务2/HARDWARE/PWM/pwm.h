#ifndef __PWM_H
#define __PWM_H
#include "sys.h"

/* TIM9_CH1 从 PE5 输出，arr、psc 分别是写入 ARR、PSC 的实际数值。 */
void TIM9_PWM_Init(u16 arr, u16 psc, u16 pulse);
/* pulse 的单位为计数次数；本实验每次计数为 1 us。 */
void TIM9_PWM_SetPulse(u16 pulse);
#endif
