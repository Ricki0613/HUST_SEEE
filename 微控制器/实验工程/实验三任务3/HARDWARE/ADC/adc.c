#include "adc.h"
#include "delay.h"

/*
 * 方式B：ADC1、ADC2 各自独立工作（ADC_Mode_Independent），不使用双重 ADC 同步。
 * 两个 ADC 的规则通道都配置为 TIM2 的 TRGO 上升沿触发，
 * 定时器每来一个触发，两个 ADC 同时启动各自的转换，各自产生 EOC 中断。
 * ADC1/ADC2/ADC3 共用同一个中断向量 ADC_IRQHandler，
 * 因此要在中断里分别判断两个 ADC 的 EOC 标志，各读各的 DR。
 */

volatile u16 ADC_DC_Buf[ADC_SAMPLE_POINTS];
volatile u16 ADC_AC_Buf[ADC_SAMPLE_POINTS];
volatile u16 ADC_DC_Count = 0;
volatile u16 ADC_AC_Count = 0;
volatile u8  ADC_AllDone  = 0;

/* 两个 ADC 共用的通用参数：独立模式、ADCCLK=PCLK2/4=21 MHz、DMA 不用 */
static void ADC_Common_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);

    /* PA5、PA4 都配置为模拟输入 */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = ADC_DC_PIN | ADC_AC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC2, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC2, DISABLE);

    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;   /* 不使用同步模式 */
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInit(&ADC_CommonInitStructure);
}

/* 单个 ADC 的规则通道参数：12 位、右对齐、1 个转换、TIM2 TRGO 触发 */
static void ADC_Rule_Config(ADC_TypeDef *ADCx, u8 ch)
{
    ADC_InitTypeDef ADC_InitStructure;

    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;              /* 单通道，不扫描 */
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;        /* 每次触发转换一次 */
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;     /* 右对齐 */
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADCx, &ADC_InitStructure);

    /* 采样时间 480 周期：492 个 ADCCLK = 23.4 us，小于 10 kHz 的 100 us 间隔 */
    ADC_RegularChannelConfig(ADCx, ch, 1, ADC_SampleTime_480Cycles);

    ADC_ClearFlag(ADCx, ADC_FLAG_EOC);
    ADC_ITConfig(ADCx, ADC_IT_EOC, DISABLE);
    ADC_Cmd(ADCx, ENABLE);
    delay_us(10);   /* 使能后要等 tSTAB，ADC 才能开始准确转换 */
}

void ADC1_ADC2_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    ADC_Common_Config();
    ADC_Rule_Config(ADC1, ADC_DC_CHANNEL);   /* ADC1 -> PA5 直流 */
    ADC_Rule_Config(ADC2, ADC_AC_CHANNEL);   /* ADC2 -> PA4 交流 */

    /* 两个 ADC 共用 ADC_IRQn，只配置一次 */
    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void ADC_StartCapture(void)
{
    ADC_DC_Count = 0;
    ADC_AC_Count = 0;
    ADC_AllDone  = 0;

    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_ClearFlag(ADC2, ADC_FLAG_EOC);
    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    ADC_ITConfig(ADC2, ADC_IT_EOC, ENABLE);
}

void ADC_IRQHandler(void)
{
    /* ADC1：直流通道 */
    if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != RESET)
    {
        if (ADC_DC_Count < ADC_SAMPLE_POINTS)
        {
            ADC_DC_Buf[ADC_DC_Count] = ADC_GetConversionValue(ADC1);
            ADC_DC_Count++;
        }
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);

        if (ADC_DC_Count >= ADC_SAMPLE_POINTS)
        {
            ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
        }
    }

    /* ADC2：交流通道 */
    if (ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) != RESET)
    {
        if (ADC_AC_Count < ADC_SAMPLE_POINTS)
        {
            ADC_AC_Buf[ADC_AC_Count] = ADC_GetConversionValue(ADC2);
            ADC_AC_Count++;
        }
        ADC_ClearFlag(ADC2, ADC_FLAG_EOC);

        if (ADC_AC_Count >= ADC_SAMPLE_POINTS)
        {
            ADC_ITConfig(ADC2, ADC_IT_EOC, DISABLE);
        }
    }

    if ((ADC_DC_Count >= ADC_SAMPLE_POINTS) && (ADC_AC_Count >= ADC_SAMPLE_POINTS))
    {
        ADC_AllDone = 1;
    }
}
