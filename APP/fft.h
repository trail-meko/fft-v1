#ifndef __FFT_H
#define __FFT_H

#include "stdint.h"
#include <math.h>
#include "arm_math.h"

/**
 * @brief ���η�������ṹ��
 */
typedef struct
{
    uint16_t max;        /* ���ֵ */
    uint16_t min;        /* ��Сֵ */
    uint16_t vpp;        /* ���ֵ */
    float avg;           /* ƽ��ֵ */
    float rms;           /* ����Чֵ */
    float phase;         /* ��λ (radians) */
    float period;        /* ���� s */
    float freq;          /* Ƶ�� Hz */
    float t_high;        /* �ߵ�ƽʱ�� s */
    float t_low;         /* �͵�ƽʱ�� s */
    float duty;          /* ռ�ձ� % */
    float h1;            /* ������ֵ */
    float h3;            /* 3��г����ֵ */
    float h5;            /* 5��г����ֵ */
    float thd;           /* ��г��ʧ�� % */
} adc_signal_result_t;


/**
 * @brief ���㲨�λ������������ֵ����Сֵ�����ֵ��ƽ��ֵ��RMS
 */
void fft_calc_basic_params(uint16_t *buf, uint16_t len, adc_signal_result_t *result);

/**
 * @brief ���㲨��ʱ����������ڡ�Ƶ�ʡ��ߵ͵�ƽʱ�䡢ռ�ձ�
 */
void fft_calc_time_params(uint16_t *buf, uint16_t len, uint32_t sample_rate, uint16_t threshold, adc_signal_result_t *result);

/**
 * @brief FFT����������1/3/5��г����THD
 */
void fft_calc_harmonics(uint16_t *buf, uint16_t len, uint32_t sample_rate, adc_signal_result_t *result);

/**
 * @brief ��һ�鲨�����ݽ�����������
 */
void fft_analyze_signal(uint16_t *buf, uint16_t len, uint32_t sample_rate, uint16_t threshold, adc_signal_result_t *result);

#endif



