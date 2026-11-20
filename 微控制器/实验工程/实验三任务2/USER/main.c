#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "adc.h"
#include "timer.h"

/*
 * 实验三任务2：50 Hz 正弦波在不同采样率下的峰峰值测量
 *
 * 接线：信号发生器输出 Vp-p=2 V（含 1.0 V 直流偏置）的 50 Hz 正弦波，
 *       输出端接 PA4（ADC1_IN4，板上 STM_DAC 多功能端口），地接开发板 GND。
 *       先用示波器直流耦合确认波形在 0~2 V 之间，再接到板子上。
 * 方法：TIM2 更新事件触发 ADC，1 kHz/10 kHz 各采 400 点，
 *       查最大值、最小值，Vpp=(max-min)*VREF/4096。
 * 结果：vpp_1k_code / vpp_10k_code 是峰峰值码值（右对齐），
 *       vpp_1k / vpp_10k 是对应的电压，可在 Keil 的 Watch 窗口查看。
 */

#define VREF_MV  3300    /* 参考电压，mV */

u16 vpp_1k_code  = 0;    /* 1 kHz 采样的峰峰值码值差 */
u16 vpp_10k_code = 0;    /* 10 kHz 采样的峰峰值码值差 */
float vpp_1k  = 0.0f;    /* 1 kHz 采样的峰峰值电压，V */
float vpp_10k = 0.0f;    /* 10 kHz 采样的峰峰值电压，V */
u16 min_1k, max_1k;      /* 1 kHz 采样的最小、最大码值 */
u16 min_10k, max_10k;    /* 10 kHz 采样的最小、最大码值 */

static void print_voltage(u32 mv)
{
    printf("%lu.%03lu V", (unsigned long)(mv / 1000), (unsigned long)(mv % 1000));
}

/* 用指定采样率采满 ADC_SAMPLE_POINTS 点，并找出最大值、最小值 */
static void capture_once(u32 fs, u16 *pmin, u16 *pmax)
{
    u16 i;

    TIM2_SetSampleRate(fs);   /* 先定采样间隔 */
    ADC1_StartCapture();      /* 再清计数、开 EOC 中断 */
    TIM2_Start();             /* 最后一个启动，避免第一个间隔被压缩 */

    while (ADC_Done == 0)
    {
        /* 400 点在 1 kHz 时约 400 ms，10 kHz 时约 40 ms */
    }
    TIM2_Stop();

    /* 最大值、最小值查找，参考实验指导书给出的写法 */
    *pmax = ADC_Buf[0];
    *pmin = ADC_Buf[0];
    for (i = 1; i < ADC_SAMPLE_POINTS; i++)
    {
        if (ADC_Buf[i] > *pmax) *pmax = ADC_Buf[i];
        if (ADC_Buf[i] < *pmin) *pmin = ADC_Buf[i];
    }
}

int main(void)
{
    u32 mv_1k, mv_10k;
    u8 round = 0;

    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init((u8)(SystemCoreClock / 1000000));
    uart_init(115200);

    TIM2_SampleTimer_Init();
    ADC1_Init();

    printf("\r\n===== Lab3 Task2: 50 Hz sine, Vpp measurement =====\r\n");
    printf("Channel: ADC1_IN4 (PA4), Vp-p=2 V with 1.0 V offset\r\n");
    printf("Points per capture: %u, VREF = %u mV\r\n", ADC_SAMPLE_POINTS, VREF_MV);

    while (1)
    {
        round++;
        printf("\r\n---------- Round %u ----------\r\n", round);

        /* (1) 1 kHz 采样：一个 50 Hz 周期采 20 点，400 点覆盖 20 个周期 */
        capture_once(1000, &min_1k, &max_1k);
        vpp_1k_code = max_1k - min_1k;
        mv_1k = (u32)vpp_1k_code * VREF_MV / 4096;
        vpp_1k = (float)mv_1k / 1000.0f;
        printf("[fs = 1 kHz ] min=%u, max=%u, vpp_1k_code=%u\r\n",
               min_1k, max_1k, vpp_1k_code);
        printf("             vpp_1k = ");
        print_voltage(mv_1k);
        printf(" (%lu mV), expected 2000 mV, error %ld mV\r\n",
               (unsigned long)mv_1k, (long)mv_1k - 2000);

        delay_ms(500);

        /* (2) 10 kHz 采样：一个 50 Hz 周期采 200 点，400 点覆盖 2 个周期 */
        capture_once(10000, &min_10k, &max_10k);
        vpp_10k_code = max_10k - min_10k;
        mv_10k = (u32)vpp_10k_code * VREF_MV / 4096;
        vpp_10k = (float)mv_10k / 1000.0f;
        printf("[fs = 10 kHz] min=%u, max=%u, vpp_10k_code=%u\r\n",
               min_10k, max_10k, vpp_10k_code);
        printf("              vpp_10k = ");
        print_voltage(mv_10k);
        printf(" (%lu mV), expected 2000 mV, error %ld mV\r\n",
               (unsigned long)mv_10k, (long)mv_10k - 2000);

        delay_ms(2000);
    }
}
