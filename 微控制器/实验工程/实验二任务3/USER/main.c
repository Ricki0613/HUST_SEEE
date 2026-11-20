#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "pwm.h"
#include "timer.h"

/*
 * 实验二任务3：周期仍为2 ms，高电平改为15 us和1500 us，各保存3组实测数据。
 * 接线沿用任务2：PE5输出，通过杜邦线连到PC7；示波器与开发板共地。
 * 操作：上电先测15 us三组，保持该波形等待；按KEY0(PE4)切换到1500 us三组。
 * 再按KEY0可回到15 us重新测量。每个阶段都会等待，便于手动记录示波器读数。
 * 数组只保存TIM3实测值，不预先填入15或1500；示波器读数需要自己实测记录。
 */
volatile u32 measured_15us[3] = {0, 0, 0};
volatile u32 measured_1500us[3] = {0, 0, 0};
volatile u16 target_high_us = 15;  /* 当前设定宽度，区别于上面两个实测数组 */
volatile u8 sample_count = 0;     /* 当前阶段已经保存的有效数据组数 */

static void KEY0_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &GPIO_InitStructure); /* 沿用实验一：PE4按下为低 */
}

static void KEY0_WaitPress(void)
{
    /* 简单轮询和20 ms延时消抖，不增加按键中断等课外内容。 */
    while (1)
    {
        if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == Bit_RESET)
        {
            delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == Bit_RESET)
            {
                while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == Bit_RESET)
                {
                    /* 等待松开，一次按下只切换一次。PWM仍由硬件连续输出。 */
                }
                delay_ms(20);
                return;
            }
        }
    }
}

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init((u8)(SystemCoreClock / 1000000));
    uart_init(115200);
    KEY0_Init();
    TIM9_PWM_Init(2000 - 1, 168 - 1, 15);
    TIM3_CH2_Capture_Init();

    printf("Lab2 Task3: PE5 -> PC7, period=2000 us\r\n");
    while (1)
    {
        /* 只改比较值，不改周期。等10 ms，确保新CCR1已在周期边界生效。 */
        TIM9_PWM_SetPulse(target_high_us);
        delay_ms(10);
        sample_count = 0;
        printf("Target HIGH = %u us, record 3 samples:\r\n", target_high_us);

        while (sample_count < 3)
        {
            TIM3_Capture_Start();
            while (TIM3_CaptureReady == 0)
            {
                /* 两次边沿捕获完成后，中断会置完成标志。 */
            }
            if (TIM3_CaptureError == 0)
            {
                if (target_high_us == 15)
                    measured_15us[sample_count] = TIM3_HighWidthUs;
                else
                    measured_1500us[sample_count] = TIM3_HighWidthUs;

                sample_count++;
                printf("Group %u: HIGH = %lu us\r\n", sample_count,
                       (unsigned long)TIM3_HighWidthUs);
                delay_ms(500); /* 每组稍作间隔，方便在串口助手中阅读 */
            }
            else
            {
                printf("Capture overrun, retry this group.\r\n");
            }
        }

        printf("3 samples saved. Measure with scope, then press KEY0.\r\n");
        KEY0_WaitPress();
        if (target_high_us == 15)
            target_high_us = 1500;
        else
            target_high_us = 15;
    }
}
