#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "delay.h"

/* 按键状态：0 表示按下，1 表示释放 */
static uint8_t key0_state = 1;   // KEY0: PE4
static uint8_t key1_state = 1;   // KEY1: PE3

static void GPIO_Config(void)
{
    GPIO_InitTypeDef gpio;

    /* 使能 GPIOE、GPIOF 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE |
                           RCC_AHB1Periph_GPIOF, ENABLE);

    /* 初始状态：蜂鸣器停止、LED1 熄灭 */
    GPIO_ResetBits(GPIOF, GPIO_Pin_8);   // PF8 = 0，蜂鸣器停止
    GPIO_SetBits(GPIOF, GPIO_Pin_10);    // PF10 = 1，LED1 熄灭

    GPIO_StructInit(&gpio);

    /* PF8：蜂鸣器；PF10：LED1 */
    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOF, &gpio);

    /* PE4：KEY0，上拉输入 */
    /* PE3：KEY1，上拉输入 */
    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &gpio);
}

/*
 * 每 10 ms 调用一次。
 * 返回值：1 表示本次新确认按下；0 表示没有新的按下事件。
 * 长按不会重复返回 1。
 */
static uint8_t Key_Scan10ms(GPIO_TypeDef *GPIOx, uint16_t pin, uint8_t *key_state)
{
    static uint8_t last[2] = {1, 1};
    static uint8_t same[2] = {0, 0};
    static uint8_t index = 0;
    uint8_t raw, pressed = 0;

    /* 简单区分两个按键的静态变量 */
    if (pin == GPIO_Pin_4)
        index = 0;   // KEY0
    else
        index = 1;   // KEY1

    raw = GPIO_ReadInputDataBit(GPIOx, pin);

    if (raw != last[index]) {
        last[index] = raw;
        same[index] = 1;
    } else if (same[index] < 2) {
        ++same[index];
    }

    if ((same[index] >= 2) && (raw != *key_state)) {
        *key_state = raw;          // 更新消抖后的稳定状态
        if (*key_state == 0)       // 0 表示按下
            pressed = 1;
    }

    return pressed;
}

int main(void)
{
    uint16_t key0_remain = 0;      // KEY0 触发的 2 s 倒计时，单位 10 ms
    uint8_t key0_pressed;
    uint8_t key1_pressed;
    uint8_t blink_count = 0;       // KEY1 闪烁半周期计数，单位 10 ms

    SystemCoreClockUpdate();
    delay_init((uint8_t)(SystemCoreClock / 1000000U));
    GPIO_Config();

    while (1)
    {
        delay_ms(10);

        /* 1. 扫描 KEY0(PE4) 和 KEY1(PE3) */
        key0_pressed = Key_Scan10ms(GPIOE, GPIO_Pin_4, &key0_state);
        key1_pressed = Key_Scan10ms(GPIOE, GPIO_Pin_3, &key1_state);

        /* 2. KEY0 新按下：启动 2 s 倒计时 */
        if (key0_pressed) {
            key0_remain = 200;     // 200 * 10 ms = 2 s
        }

        /* 3. KEY0 倒计时递减 */
        if (key0_remain > 0) {
            --key0_remain;
        }

        /* 4. KEY1 按住时的 LED1 闪烁 */
        if (key1_state == 0)       // KEY1 按住
        {
            if (key1_pressed) {
                /* 新按下 KEY1：LED1 先点亮，并清零闪烁计数 */
                GPIO_ResetBits(GPIOF, GPIO_Pin_10); // LED1 点亮
                blink_count = 0;
            } else {
                /* 每 25 个 10 ms 翻转一次，即 0.25 s 亮、0.25 s 灭 */
                if (++blink_count >= 25) {
                    blink_count = 0;
                    GPIO_ToggleBits(GPIOF, GPIO_Pin_10);
                }
            }
        }
        else                       // KEY1 释放
        {
            GPIO_SetBits(GPIOF, GPIO_Pin_10); // LED1 熄灭
            blink_count = 0;
        }

        /* 5. 统一控制蜂鸣器 PF8 —— 高电平鸣响 */
        if ((key0_remain > 0) || (key1_state == 0)) {
            GPIO_SetBits(GPIOF, GPIO_Pin_8);     // PF8 = 1，鸣响
        } else {
            GPIO_ResetBits(GPIOF, GPIO_Pin_8);   // PF8 = 0，停止
        }
    }
}
