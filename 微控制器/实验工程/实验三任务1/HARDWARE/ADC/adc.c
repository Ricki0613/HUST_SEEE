#include "adc.h"
#include "delay.h"

/*
 * 参考标准例程实验18：ADC1 通道5、独立模式、软件触发、单次转换。
 * 本任务只需要改分辨率，所以把分辨率做成 ADC1_Init 的参数，
 * 每次切换 8 位/12 位都重新初始化一次 ADC1（ADC 运行中不能改分辨率）。
 */

void ADC1_Init(u32 resolution)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    /* 1. 使能 GPIOA 和 ADC1 时钟，PA5 配置为模拟输入。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = ADC_DC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;      /* 模拟输入 */
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  /* 模拟输入不加内部上下拉 */
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 2. 复位 ADC1，保证上一次的分辨率等配置被彻底清除。 */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);

    /* 3. 通用控制：独立模式，ADCCLK=PCLK2/4=84/4=21 MHz（不超过 36 MHz）。 */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* 4. 分辨率由参数决定；非扫描、非连续，一次软件触发只转换 1 个通道。 */
    ADC_InitStructure.ADC_Resolution = resolution;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;  /* 右对齐 */
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
    delay_us(10);   /* 使能后要等 tSTAB，ADC 才能开始准确转换 */
}

u16 ADC1_Read(void)
{
    /* 采样时间取最长的 480 个周期，直流测量精度较高；转换共 480+12 个周期。 */
    ADC_RegularChannelConfig(ADC1, ADC_DC_CHANNEL, 1, ADC_SampleTime_480Cycles);
    ADC_SoftwareStartConv(ADC1);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    {
        /* 等待规则通道转换结束 */
    }
    return ADC_GetConversionValue(ADC1);  /* 读 DR 同时清除 EOC */
}
