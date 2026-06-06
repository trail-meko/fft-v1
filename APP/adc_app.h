#ifndef __ADC_APP_H
#define __ADC_APP_H

#include <stdint.h>
#include "fft.h"

/* ADC 通道数 */
#define ADC_CH_NUM          3

/* 每个通道每帧采样数 */
#define ADC_SAMPLES         256

/* DMA 总缓冲大小 = 2帧(双缓冲思想：前半帧 + 后半帧) */
#define ADC_DMA_BUF_SIZE    (ADC_CH_NUM * ADC_SAMPLES * 2)

/* DMA 原始缓冲（交织排列 ch1 ch2 ch3） */
extern uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];

/* 分拣后三通道缓冲（LCD 显示用） */
extern uint16_t lcd_adc_ch1_buf[ADC_SAMPLES];
extern uint16_t lcd_adc_ch2_buf[ADC_SAMPLES];
extern uint16_t lcd_adc_ch3_buf[ADC_SAMPLES];

/* 分拣后三通道缓冲（FFT 算法用） */
extern uint16_t ttf_adc_ch1_buf[ADC_SAMPLES];
extern uint16_t ttf_adc_ch2_buf[ADC_SAMPLES];
extern uint16_t ttf_adc_ch3_buf[ADC_SAMPLES];

/* FFT 分析结果（三通道） */
extern adc_signal_result_t ch1_result;
extern adc_signal_result_t ch2_result;
extern adc_signal_result_t ch3_result;

/* DMA 半传输/全传输完成标志
   0: 无新数据
   1: 前半缓冲完成传输
   2: 后半缓冲完成传输
*/
extern volatile uint8_t adc_flag;

/* ADC1 + DMA + TIM3 初始化（配置每通道采样率） */
void adc1_init(uint32_t sample_rate);

/* ADC 数据处理函数（在调度器循环中调用） */
void adc_proc(void);

#endif
