#include "timer.h"

/*
 * 参考标准例程实验10：先捕获上升沿，再切换为下降沿捕获。
 * TIM5_CH1改为TIM3_CH2：引脚改为PC7，复用AF2，中断改为TIM3_IRQHandler，
 * 捕获通道、标志、读取CCR的函数，都相应改为通道2。
 * TIM3是16位定时器，ARR最大65535，不能照搬TIM5的0xFFFFFFFF。
 *
 * 本程序直接保存上升沿、下降沿的硬件捕获时刻，两个时刻相减得到脉宽。
 * 不在上升沿中断里清零CNT，避免把“进入中断之前已经经过的时间”丢掉。
 * 对15 us短脉冲，这个区别尤其重要。切换边沿仍须在下降沿到来前完成。
 */
volatile u8 TIM3_CaptureReady = 1;
volatile u8 TIM3_CaptureError = 0;
volatile u32 TIM3_HighWidthUs = 0;
volatile u32 TIM3_OverflowCount = 0;
static u8 capture_started = 0;       /* 0等上升沿，1等下降沿 */
static u32 rising_time = 0;         /* 上升沿的扩展计数时刻 */

void TIM3_CH2_Capture_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 1. TIM3挂在APB1：PCLK1=42 MHz，定时器时钟为84 MHz。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 2. PC7配置为TIM3_CH2。下拉使未接信号时尽量保持低电平。 */
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* 3. 84 MHz / 84 = 1 MHz，每次计数1 us，每65536 us溢出一次。 */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* 4. 初始捕获上升沿，直接映射TI2，不对边沿分频，不加数字滤波。 */
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0;
    TIM_ICInit(TIM3, &TIM_ICInitStructure);

    /* 5. 初始化会产生更新事件，先清标志再开中断，防止多计一次溢出。 */
    TIM_ClearFlag(TIM3, TIM_FLAG_Update | TIM_FLAG_CC2 | TIM_FLAG_CC2OF);
    TIM3_OverflowCount = 0;
    TIM3_CaptureReady = 1;
    TIM_ITConfig(TIM3, TIM_IT_Update | TIM_IT_CC2, ENABLE);

    /* 6. 捕获中断优先级高于原例程USART1接收中断，尽快切换捕获边沿。 */
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_Cmd(TIM3, ENABLE);
}

void TIM3_Capture_Start(void)
{
    /* 清理上一次捕获，只暂时关通道2中断，不停止持续计时的TIM3。 */
    TIM_ITConfig(TIM3, TIM_IT_CC2, DISABLE);
    capture_started = 0;
    TIM3_CaptureError = 0;
    TIM_OC2PolarityConfig(TIM3, TIM_ICPolarity_Rising);
    TIM_ClearFlag(TIM3, TIM_FLAG_CC2 | TIM_FLAG_CC2OF);
    TIM3_CaptureReady = 0;
    TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);
}

void TIM3_IRQHandler(void)
{
    u16 capture_value;
    u32 overflow;
    u32 capture_time;

    /* 先处理捕获，再处理更新，保证下面的溢出边界判断有固定顺序。 */
    if (TIM_GetITStatus(TIM3, TIM_IT_CC2) != RESET)
    {
        capture_value = TIM_GetCapture2(TIM3); /* 读硬件锁存的CCR2 */
        overflow = TIM3_OverflowCount;

        /*
         * 若溢出与捕获几乎同时发生，更新中断可能还未计入overflow。
         * CCR2很小表示边沿在回零之后到来，临时补上这一次溢出；
         * CCR2很大则表示边沿在回零之前到来，不补加。
         * 前提是中断及时响应，延迟远小于半个计数周期(32768 us)。
         */
        if ((TIM_GetFlagStatus(TIM3, TIM_FLAG_Update) != RESET) &&
            (capture_value < 32768))
        {
            overflow++;
        }
        capture_time = (overflow << 16) + capture_value;
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC2);

        if (TIM3_CaptureReady == 0)
        {
            if (TIM_GetFlagStatus(TIM3, TIM_FLAG_CC2OF) != RESET)
            {
                /* 捕获被后续边沿覆盖，不能把这次数据当成有效脉宽。 */
                TIM3_CaptureError = 1;
                capture_started = 0;
                TIM_OC2PolarityConfig(TIM3, TIM_ICPolarity_Rising);
                TIM3_CaptureReady = 1;
            }
            else if (capture_started == 0)
            {
                rising_time = capture_time;
                capture_started = 1;
                /* 切换成下降沿，下一次捕获对应高电平结束。 */
                TIM_OC2PolarityConfig(TIM3, TIM_ICPolarity_Falling);
            }
            else
            {
                /*
                 * 脉宽计数 = N * (ARR+1) + 下降沿CCR2 - 上升沿CCR2。
                 * ARR=65535，所以每次溢出必须加65536，而不是65535。
                 * 这里用两个扩展时刻相减，已经包含溢出次数差。
                 * 无符号32位相减还能处理扩展时刻本身的一次回绕。
                 * 每个计数为1 us，所以差值可直接作为us数使用。
                 */
                TIM3_HighWidthUs = capture_time - rising_time;
                capture_started = 0;
                TIM_OC2PolarityConfig(TIM3, TIM_ICPolarity_Rising);
                /* 最后置完成标志。主程序读完以前不覆盖结果。 */
                TIM3_CaptureReady = 1;
            }
        }
        TIM_ClearFlag(TIM3, TIM_FLAG_CC2OF);
    }

    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        TIM3_OverflowCount++; /* 一次溢出代表65536个1 us计数 */
    }
    /* 中断只保存计数和切换边沿，串口打印与延时都放在主循环中。 */
}
