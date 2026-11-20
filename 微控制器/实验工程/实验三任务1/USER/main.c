#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "adc.h"

/*
 * 实验三任务1：8 位与 12 位分辨率比较
 *
 * 接线：直流稳压电源输出 1.00 V，正端接 PA5（ADC1_IN5，板上 STM_ADC），
 *       负端接开发板 GND；拔掉 P12 跳线帽，断开 TPAD 对 PA5 的影响。
 * 用数字电压表监测输入电压，把实测值填到 DMM_MV，程序据此算偏差。
 * 结果：adc8_avg / adc12_avg 是自定义单元1、2（10 次平均码值，右对齐）；
 *       v8 / v12 是对应的电压值，可在 Keil 的 Watch 窗口查看。
 * 串口 USART1：115200、8 位、无校验、1 停止位。
 */

#define VREF_MV       3300    /* 参考电压 VREF+，单位 mV，实测 3.3 V */
#define DMM_MV        1000    /* 数字电压表实测值，单位 mV，请按实测改写 */
#define SAMPLE_TIMES  10      /* 每种分辨率连续测量次数 */

u16 adc8_avg  = 0;            /* 自定义单元1：8 位分辨率 10 次平均码值 */
u16 adc12_avg = 0;            /* 自定义单元2：12 位分辨率 10 次平均码值 */
float v8  = 0.0f;             /* 8 位平均码值换算出的电压，V */
float v12 = 0.0f;             /* 12 位平均码值换算出的电压，V */

u16 code8[SAMPLE_TIMES];      /* 8 位 10 次原始码值 */
u16 code12[SAMPLE_TIMES];     /* 12 位 10 次原始码值 */

/* 打印“x.xxx V”形式的电压，全部用整数运算，避免依赖 printf 的浮点支持 */
static void print_voltage(u32 mv)
{
    printf("%lu.%03lu V", (unsigned long)(mv / 1000), (unsigned long)(mv % 1000));
}

/* 按给定分辨率初始化 ADC1，连续测量 SAMPLE_TIMES 次，返回四舍五入的平均码值 */
static u16 measure_average(u32 resolution, u16 *buf)
{
    u32 sum = 0;
    u8 i;

    ADC1_Init(resolution);    /* 改分辨率必须重新初始化 ADC1 */
    for (i = 0; i < SAMPLE_TIMES; i++)
    {
        buf[i] = ADC1_Read();
        sum += buf[i];
        delay_ms(20);         /* 两次测量之间留间隔，避免读数完全相关 */
    }
    return (u16)((sum + SAMPLE_TIMES / 2) / SAMPLE_TIMES);
}

int main(void)
{
    u32 mv8, mv12;
    u8 i, round = 0;

    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init((u8)(SystemCoreClock / 1000000));
    uart_init(115200);

    printf("\r\n===== Lab3 Task1: 8-bit / 12-bit resolution compare =====\r\n");
    printf("Channel: ADC1_IN5 (PA5), input about 1.00 V\r\n");
    printf("DMM reading: ");
    print_voltage(DMM_MV);
    printf(", VREF = %u mV\r\n", VREF_MV);

    while (1)
    {
        round++;
        printf("\r\n---------- Round %u ----------\r\n", round);

        /* (1) 8 位分辨率，连续测量 10 次 */
        printf("[8-bit ] N=%u\r\n", SAMPLE_TIMES);
        adc8_avg = measure_average(ADC_Resolution_8b, code8);
        for (i = 0; i < SAMPLE_TIMES; i++)
        {
            printf("  code8[%u] = %u\r\n", i, code8[i]);
        }
        /* 8 位满量程 256，最小分辨单位 VREF/256 ≈ 12.9 mV */
        mv8 = (u32)adc8_avg * VREF_MV / 256;
        v8  = (float)mv8 / 1000.0f;
        printf("  adc8_avg = %u, v8 = ", adc8_avg);
        print_voltage(mv8);
        printf("  (1 LSB = 12.9 mV)\r\n");

        /* (2) 12 位分辨率，连续测量 10 次 */
        printf("[12-bit] N=%u\r\n", SAMPLE_TIMES);
        adc12_avg = measure_average(ADC_Resolution_12b, code12);
        for (i = 0; i < SAMPLE_TIMES; i++)
        {
            printf("  code12[%u] = %u\r\n", i, code12[i]);
        }
        /* 12 位满量程 4096，最小分辨单位 VREF/4096 ≈ 0.81 mV */
        mv12 = (u32)adc12_avg * VREF_MV / 4096;
        v12  = (float)mv12 / 1000.0f;
        printf("  adc12_avg = %u, v12 = ", adc12_avg);
        print_voltage(mv12);
        printf("  (1 LSB = 0.81 mV)\r\n");

        /* (3) 与数字电压表读数比较 */
        printf("DMM = %u mV, error(8-bit) = %ld mV, error(12-bit) = %ld mV\r\n",
               DMM_MV, (long)mv8 - (long)DMM_MV, (long)mv12 - (long)DMM_MV);

        delay_ms(2000);
    }
}
