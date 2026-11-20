#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "pwm.h"
#include "timer.h"

/*
 * 实验二任务2：本工程同时输出500 us脉冲并用TIM3测量。
 * 接线：用杜邦线把PE5(TIM9_CH1输出)接到PC7(TIM3_CH2输入)。
 * 示波器探头接PE5或PC7，地夹接GND；串口助手设115200、8位、无校验、1停止位。
 * USART1：PA9为TX，PA10为RX；可使用开发板原有USB转串口接口。
 * 程序不把设定值当成测量值，打印的数据来自TIM3的两次硬件捕获。
 */
volatile u32 measured_high_us = 0; /* 也可在Keil Watch窗口观察，打开连续刷新 */

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init((u8)(SystemCoreClock / 1000000));
    uart_init(115200);

    TIM9_PWM_Init(2000 - 1, 168 - 1, 500);
    TIM3_CH2_Capture_Init();

    printf("Lab2 Task2: PE5 -> PC7, period=2000 us, target=500 us\r\n");

    while (1)
    {
        TIM3_Capture_Start();       /* 从下一个上升沿开始测一次完整高电平 */
        while (TIM3_CaptureReady == 0)
        {
            /* 等待中断完成。若一直不输出结果，先检查PE5到PC7的连线。 */
        }

        if (TIM3_CaptureError == 0)
        {
            measured_high_us = TIM3_HighWidthUs;
            printf("HIGH: %lu us\r\n", (unsigned long)measured_high_us);
        }
        else
        {
            printf("Capture overrun, retry.\r\n");
        }

        /* 降低打印速度，便于阅读；延时不会改变硬件输出的PWM周期。 */
        delay_ms(500);
    }
}
