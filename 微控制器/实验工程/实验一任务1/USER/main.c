#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "delay.h"

int main(void)
{
    GPIO_InitTypeDef gpio;
    uint8_t led0_count = 0;     // LED0 的计数器，每次代表约 10 ms

    /* 一、初始化延时模块 */
    SystemCoreClockUpdate();
    delay_init((uint8_t)(SystemCoreClock / 1000000U));

    /* 二、开启 GPIOE 和 GPIOF 的时钟 */
    RCC_AHB1PeriphClockCmd(
        RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF,
        ENABLE
    );

    /* 三、预置输出状态：两个 LED 熄灭，蜂鸣器停止 */
    GPIO_SetBits(GPIOF, GPIO_Pin_9 | GPIO_Pin_10);
    GPIO_ResetBits(GPIOF, GPIO_Pin_8);

    /* 四、配置输出引脚
       PF9 ：LED0，低电平点亮
       PF10：LED1，低电平点亮
       PF8 ：蜂鸣器，高电平鸣响
    */
    GPIO_StructInit(&gpio);

    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOF, &gpio);

    /* 五、配置按键为上拉输入
       PE4：KEY0，按下为低电平
       PE3：KEY1，按下为低电平
    */
    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &gpio);

    /* LED0 从点亮状态开始，先亮约 1 s */
    GPIO_ResetBits(GPIOF, GPIO_Pin_9);

    while (1)
    {
        /* 每轮以约 10 ms 为一个时间片 */
        delay_ms(10);

        /* 功能1：LED0 自动亮 1 s、灭 1 s */
        ++led0_count;

        if (led0_count >= 100)
        {
            led0_count = 0;
            GPIO_ToggleBits(GPIOF, GPIO_Pin_9);
        }

        /* 功能2：KEY0 按下时 LED1 亮，释放时灭 */
        if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == Bit_RESET)
        {
            GPIO_ResetBits(GPIOF, GPIO_Pin_10);
        }
        else
        {
            GPIO_SetBits(GPIOF, GPIO_Pin_10);
        }

        /* 功能3：KEY1 按下时蜂鸣器响，释放时停止 */
        if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == Bit_RESET)
        {
            GPIO_SetBits(GPIOF, GPIO_Pin_8);
        }
        else
        {
            GPIO_ResetBits(GPIOF, GPIO_Pin_8);
        }
    }
}
