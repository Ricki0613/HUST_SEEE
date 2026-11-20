#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "adc.h"
#include "timer.h"

/*
 * 实验三任务3：单 ADC 扫描与双独立 ADC 采样（二选一）—— 本工程实现方式B
 *
 * 方式B：ADC1、ADC2 独立模式（ADC_Mode_Independent），不使用双重 ADC 同步。
 *   ADC1 的通道5（PA5，板上 STM_ADC）接 1.00 V 直流；
 *   ADC2 的通道4（PA4，板上 STM_DAC 多功能端口）接 50 Hz 正弦波。
 * 两个信号必须共地；交流信号要带 1.0 V 直流偏置，保证 0~3.3 V 内。
 * 配置：12 位、右对齐、TIM2 TRGO 触发、采样率 10 kHz、每通道 400 点。
 * 计算：直流取平均值；交流取 Vpp=(max-min)*VREF/4096。
 * 显示：LCD 显示数值及单位，同时串口 USART1（115200）输出一份。
 */

#define VREF_MV  3300    /* 参考电压，mV */

u16 dc_code_avg = 0;     /* 直流通道 400 点平均码值 */
u16 ac_min = 0;          /* 交流通道最小值 */
u16 ac_max = 0;          /* 交流通道最大值 */
u16 vpp_code = 0;        /* 交流通道峰峰值码值 */
float v_dc  = 0.0f;      /* 直流电压，V */
float v_pp  = 0.0f;      /* 交流峰峰值，V */

/* 在 (x,y) 处显示“x.xxx”形式的电压，整数部分 1 位、小数 3 位 */
static void LCD_ShowVoltage(u16 x, u16 y, u32 mv)
{
    LCD_ShowxNum(x, y, mv / 1000, 1, 16, 0);          /* 整数部分 */
    LCD_ShowChar(x + 8, y, '.', 16, 0);                /* 小数点 */
    LCD_ShowxNum(x + 16, y, mv % 1000, 3, 16, 0x80);  /* 小数部分，不足补 0 */
}

static void print_voltage(u32 mv)
{
    printf("%lu.%03lu V", (unsigned long)(mv / 1000), (unsigned long)(mv % 1000));
}

int main(void)
{
    u32 sum, mv_dc, mv_pp;
    u16 i;
    u8 round = 0;

    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init((u8)(SystemCoreClock / 1000000));
    uart_init(115200);

    LCD_Init();
    POINT_COLOR = BLUE;
    BACK_COLOR  = WHITE;
    LCD_Clear(WHITE);

    TIM2_SampleTimer_Init();
    TIM2_SetSampleRate(10000);      /* 本任务采样率固定 10 kHz（ARR = 99） */
    ADC1_ADC2_Init();

    /* 固定不变的说明文字只显示一次，数值区每轮刷新。
     * lcd.h 的字符串参数类型是 u8*，这里统一加 (u8 *) 强转，避免编译告警。 */
    POINT_COLOR = RED;
    LCD_ShowString(10, 10, 220, 16, 16, (u8 *)"STM32F407 ADC LAB3");
    POINT_COLOR = BLUE;
    LCD_ShowString(10, 35, 220, 16, 16, (u8 *)"ADC1+ADC2 12bit");
    LCD_ShowString(10, 55, 220, 16, 16, (u8 *)"fs=10kHz  N=400");

    LCD_ShowString(10, 90, 220, 16, 16, (u8 *)"DC CH5/PA5:");
    LCD_ShowString(10, 130, 220, 16, 16, (u8 *)"AC CH4/PA4:");
    LCD_ShowString(10, 155, 220, 16, 16, (u8 *)"Vpp =");
    LCD_ShowString(158, 90, 20, 16, 16, (u8 *)"V");
    LCD_ShowString(158, 130, 20, 16, 16, (u8 *)"V");

    printf("\r\n===== Lab3 Task3: ADC1+ADC2 (mode B) =====\r\n");
    printf("DC: ADC1_IN5 (PA5), AC: ADC2_IN4 (PA4)\r\n");
    printf("12bit right-align, fs=10 kHz, N=%u\r\n", ADC_SAMPLE_POINTS);

    while (1)
    {
        round++;

        /* 1. 启动一次 400 点的同步采集，两路各自中断存数 */
        ADC_StartCapture();
        TIM2_Start();
        while (ADC_AllDone == 0)
        {
            /* 10 kHz 采 400 点约 40 ms */
        }
        TIM2_Stop();

        /* 2. 直流：400 点求平均 */
        sum = 0;
        for (i = 0; i < ADC_SAMPLE_POINTS; i++)
        {
            sum += ADC_DC_Buf[i];
        }
        dc_code_avg = (u16)((sum + ADC_SAMPLE_POINTS / 2) / ADC_SAMPLE_POINTS);
        mv_dc = (u32)dc_code_avg * VREF_MV / 4096;
        v_dc  = (float)mv_dc / 1000.0f;

        /* 3. 交流：求最大值、最小值，再算峰峰值 */
        ac_max = ADC_AC_Buf[0];
        ac_min = ADC_AC_Buf[0];
        for (i = 1; i < ADC_SAMPLE_POINTS; i++)
        {
            if (ADC_AC_Buf[i] > ac_max) ac_max = ADC_AC_Buf[i];
            if (ADC_AC_Buf[i] < ac_min) ac_min = ADC_AC_Buf[i];
        }
        vpp_code = ac_max - ac_min;
        mv_pp = (u32)vpp_code * VREF_MV / 4096;
        v_pp  = (float)mv_pp / 1000.0f;

        /* 4. LCD 刷新数值 */
        LCD_ShowVoltage(106, 90, mv_dc);
        LCD_ShowVoltage(106, 130, mv_pp);
        LCD_ShowVoltage(58, 155, mv_pp);
        LCD_ShowString(10, 195, 220, 16, 16, (u8 *)"DCavg code=");
        LCD_ShowxNum(114, 195, dc_code_avg, 4, 16, 0);
        LCD_ShowString(10, 215, 220, 16, 16, (u8 *)"AC min=");
        LCD_ShowxNum(74, 215, ac_min, 4, 16, 0);
        LCD_ShowString(114, 215, 100, 16, 16, (u8 *)"max=");
        LCD_ShowxNum(154, 215, ac_max, 4, 16, 0);

        /* 5. 串口同步输出，便于写实验报告 */
        printf("\r\n---------- Round %u ----------\r\n", round);
        printf("DC  CH5/PA5: code_avg=%u, v_dc  = ", dc_code_avg);
        print_voltage(mv_dc);
        printf("\r\nAC  CH4/PA4: min=%u, max=%u, vpp_code=%u, v_pp = ",
               ac_min, ac_max, vpp_code);
        print_voltage(mv_pp);
        printf("\r\n");

        delay_ms(500);
    }
}
