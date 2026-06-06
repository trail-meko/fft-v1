#ifndef __FFT_H
#define __FFT_H

#include "stdint.h"
#include <math.h>
#include "arm_math.h"

/**
 * @brief 波形分析结果结构体
 */
typedef struct
{
    uint16_t max;        /* 最大值 */
    uint16_t min;        /* 最小值 */
    uint16_t vpp;        /* 峰峰值 */
    float avg;           /* 平均值 */
    float rms;           /* 真有效值 */
    float period;        /* 周期 s */
    float freq;          /* 频率 Hz */
    float t_high;        /* 高电平时间 s */
    float t_low;         /* 低电平时间 s */
    float duty;          /* 占空比 % */
    float h1;            /* 基波幅值 */
    float h3;            /* 3次谐波幅值 */
    float h5;            /* 5次谐波幅值 */
    float thd;           /* 总谐波失真 % */
} adc_signal_result_t;


/**
 * @brief 计算波形基本参数：最大值、最小值、峰峰值、平均值、RMS
 */
void fft_calc_basic_params(uint16_t *buf, uint16_t len, adc_signal_result_t *result);

/**
 * @brief 计算波形时域参数：周期、频率、高低电平时间、占空比
 */
void fft_calc_time_params(uint16_t *buf, uint16_t len, uint32_t sample_rate, uint16_t threshold, adc_signal_result_t *result);

/**
 * @brief FFT分析：计算1/3/5次谐波和THD
 */
void fft_calc_harmonics(uint16_t *buf, uint16_t len, uint32_t sample_rate, adc_signal_result_t *result);

/**
 * @brief 对一组波形数据进行完整分析
 */
void fft_analyze_signal(uint16_t *buf, uint16_t len, uint32_t sample_rate, uint16_t threshold, adc_signal_result_t *result);

#endif



