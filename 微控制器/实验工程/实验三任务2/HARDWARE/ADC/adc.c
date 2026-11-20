#include "adc.h"
#include "delay.h"

/*
 * 参考标准例程实验18，把软件触发改成定时器触发，采样结果放进数组。
 * TIM2 每次更新产生 TRGO，ADC1 收到上升沿就开始一次规则通道转换；
 * 转换结束产生 EOC 中断，在中断里读 DR 存数组，采满 400 点后关掉 EOC 中断。
 * 这样每个采样点之间的间隔完全由定时器决定，不额外加延时。
 */

volatile u16 ADC_Buf[ADC_SAMPLE_POINTS];
volatile u16 ADC_Count = 0;
volatile u8  ADC_Done  = 0;

void ADC1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 1. 使能 GPIOA 和 ADC1 时钟，PA4 配置为模拟输入。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = ADC_AC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);

    /* 2. 独立模式，ADCCLK=PCLK2/4=21 MHz。 */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* 3. 12 位、右对齐、单通道、非扫描；触发源选 TIM2 的 TRGO 上升沿。 */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  /* 每次触发只转换一次 */
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
    delay_us(10);   /* 使能后要等 tSTAB，ADC 才能开始准确转换 */

    /* 4. 采样时间 480 个周期，转换时间 492 个 ADCCLK = 23.4 us，
     *    远小于 10 kHz 的 100 us 采样间隔，不会丢点。 */
    ADC_RegularChannelConfig(ADC1, ADC_AC_CHANNEL, 1, ADC_SampleTime_480Cycles);

    /* 5. 开转换结束中断，先关着，等调用 ADC1_StartCapture 再打开。 */
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);

    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void ADC1_StartCapture(void)
{
    ADC_Count = 0;
    ADC_Done  = 0;
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);   /* 由 TIM2 触发后陆续进入中断 */
}

/* ADC1/2/3 共用同一个中断向量 */
void ADC_IRQHandler(void)
{
    if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != RESET)
    {
        if (ADC_Count < ADC_SAMPLE_POINTS)
        {
            ADC_Buf[ADC_Count] = ADC_GetConversionValue(ADC1);  /* 读 DR 清 EOC */
            ADC_Count++;
        }
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);

        if (ADC_Count >= ADC_SAMPLE_POINTS)
        {
            ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);  /* 采满 400 点，停止记录 */
            ADC_Done = 1;
        }
    }
}
