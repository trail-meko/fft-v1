#ifndef __ADC_APP_H
#define __ADC_APP_H

#include <stdint.h>

/* ADC通道数 */
#define ADC_CH_NUM          3

/* 每个通道每帧采样点数 */
#define ADC_SAMPLES         256

/* DMA总缓冲区长度 = 2帧（双缓冲思想：前半帧 + 后半帧） */
#define ADC_DMA_BUF_SIZE    (ADC_CH_NUM * ADC_SAMPLES * 2)

/* DMA原始缓冲区：交织存放 ch1 ch2 ch3 */
extern uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];

/* 拆分后的三路数据 */
extern uint16_t g_adc_ch1_buf[ADC_SAMPLES];
extern uint16_t g_adc_ch2_buf[ADC_SAMPLES];
extern uint16_t g_adc_ch3_buf[ADC_SAMPLES];

/* 给LCD显示用的一路波形缓存 */
extern uint16_t lcd_wave_buf[ADC_DMA_BUF_SIZE/2];

/* 给频谱/算法处理用的一路波形缓存 */
extern uint16_t ttf_wave_buf[ADC_DMA_BUF_SIZE/2];

/* DMA半传输/全传输标志
   0: 无新数据
   1: 前半缓冲可处理
   2: 后半缓冲可处理
*/
extern volatile uint8_t adc_flag;

/* ADC1 + DMA + TIM3 初始化，参数是每个通道采样率 */
void adc1_init(uint32_t sample_rate);

/* ADC数据分发处理函数，在主循环或调度器中调用 */
void adc_proc(void);

/* LCD处理接口：你自己在 .c 里补逻辑 */
void lcd_proc(void);

/* TTF处理接口：你自己在 .c 里补逻辑
   如果你本意是 FFT，也可以自己改名为 fft_proc
*/
void ttf_proc(void);

#endif

