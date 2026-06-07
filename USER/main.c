#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key_app.h"
#include "GUI.h"
#include "Lcd_Driver.h"
#include "timer.h"
#include "math.h"
#include "arm_math.h"
#include "adc_app.h"
#include "scheduler.h"
#include "dac_app.h"
#include "adc_app.h"
#include "waveform_analyzer_app.h"

/* ================================================================
 * 2026 合肥工业大学电赛 - 运算放大电路增益测试电路
 * 硬件平台: STM32F407VGTx, 168MHz
 * ================================================================ */

/* 全局FFT缓冲 (保留用于自测试, FFT_LENGTH 由 waveform_analyzer_app.h 定义) */
float fft_inputbuf[FFT_LENGTH*2];
float fft_outputbuf[FFT_LENGTH];
u8 timeout;

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(168);
    uart_init(115200);

    /* ---- 1. 系统状态初始化 (必须在DAC/ADC之前) ---- */
    system_state_init();

    /* ---- 2. ADC初始化: 3通道, 20kHz采样率 ---- */
    adc1_init(20000);

    /* ---- 3. DAC初始化: 1kHz, 141mV峰值 (约100mV RMS for sine) ---- */
    dac_app_init(g_sys.freq_hz, g_sys.dac_amplitude_mv);
    dac_app_set_zero_based(0);  /* 中点基准波形 (非零基准) */

    /* ---- 4. PID AGC 控制器: 初始化 + 启动 TIM4 10ms 高优先级闭环 ---- */
    pid_agc_init(50.0f, 5.0f, 0.5f, 1.5f, 1, 1650);
    pid_agc_start(10);  /* 10ms 周期, TIM4 最高优先级中断 */

    /* ---- 5. 外设初始化 ---- */
    LED_Init();
    key_init();

    /* ---- 6. LCD初始化 ---- */
    Lcd_Init();
    Lcd_Clear(GRAY0);
    ui_init();

    /* ---- 7. FFT初始化 ---- */
    My_FFT_Init();

    /* ---- 8. 启动调度器 ---- */
    scheduler_init();

    printf("\r\n=== OpAmp Gain Test v7 ===\r\n");
    printf("DAC: PA4, ADC: PA5(Ui)/PA6(Uo)/PA7\r\n");
    printf("KEY1:Freq  KEY2:Mode  KEY3:Wave\r\n");

    while (1)
    {
        scheduler_run();
    }
}
