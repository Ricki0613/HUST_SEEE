#include "sys.h"
#include "pwm.h"

/*
 * 实验二任务1：TIM9_CH1在PE5输出周期2 ms、高电平500 us的矩形波。
 * 示波器探头接PE5，地夹接开发板GND，采用直流耦合。
 * 预期：频率500 Hz，周期2 ms，高电平500 us，占空比25%，高电平约3.3 V。
 */
int main(void)
{
    SystemCoreClockUpdate();  /* 启动文件已调用SystemInit建立168 MHz时钟 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* TIM9时钟168 MHz，分频168，计数2000次，比较值500。 */
    TIM9_PWM_Init(2000 - 1, 168 - 1, 500);

    while (1)
    {
        /* PWM由硬件自动输出，主程序无需不断翻转PE5。 */
    }
}
