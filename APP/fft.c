#include "fft.h"
#include <stdlib.h>
#include "adc_app.h"

/* FFT计算静态缓冲区 */
static float fft_in[ADC_SAMPLES];
static float fft_out[ADC_SAMPLES];
static float mag[ADC_SAMPLES/2];

/* FFT实例静态化，只初始化一次，避免反复创建导致内部状态残留 */
static arm_rfft_fast_instance_f32 fft_s;
static uint8_t fft_init_done = 0;

/**
 * @brief 计算波形基本参数
 */
void fft_calc_basic_params(uint16_t *buf, uint16_t len, adc_signal_result_t *result)
{
    if(buf == NULL || result == NULL || len == 0) return;

    uint32_t sum = 0;
    uint64_t sum_sq = 0;
    result->max = buf[0];
    result->min = buf[0];

    for(uint16_t i=0; i<len; i++)
    {
        if(buf[i] > result->max) result->max = buf[i];
        if(buf[i] < result->min) result->min = buf[i];
        sum += buf[i];
        sum_sq += (uint64_t)buf[i] * buf[i];
    }

    result->vpp  = result->max - result->min;
    result->avg  = (float)sum / len;
    result->rms  = sqrtf((float)sum_sq / len);
}

/**
 * @brief 计算波形时域参数
 */
void fft_calc_time_params(uint16_t *buf, uint16_t len, uint32_t sample_rate, uint16_t threshold, adc_signal_result_t *result)
{
    if(buf == NULL || result == NULL || len < 2 || sample_rate == 0) return;

    int32_t first_rise = -1, second_rise = -1;
    uint32_t high_cnt=0, low_cnt=0;

    for(uint16_t i=1;i<len;i++)
    {
        uint8_t prev = (buf[i-1]>=threshold)?1:0;
        uint8_t curr = (buf[i]>=threshold)?1:0;

        if(prev==0 && curr==1)
        {
            if(first_rise<0) first_rise=i;
            else if(second_rise<0) second_rise=i;
        }

        if(curr) high_cnt++;
        else low_cnt++;
    }

    if(first_rise>=0 && second_rise>first_rise)
    {
        uint32_t period_cnt = second_rise - first_rise;
        result->period = (float)period_cnt / sample_rate;
        result->freq   = (result->period>0)?1.0f/result->period:0.0f;
        result->t_high = (float)high_cnt / sample_rate;
        result->t_low  = (float)low_cnt / sample_rate;
        result->duty   = ((float)high_cnt/(high_cnt+low_cnt))*100.0f;
    }
}

/**
 * @brief FFT谐波分析
 */
void fft_calc_harmonics(uint16_t *buf, uint16_t len, uint32_t sample_rate, adc_signal_result_t *result)
{
    /* DC信号保护：Vpp太小（<10 LSB），判定为无AC分量 */
    if(result->vpp < 10)
    {
        result->h1 = 0.0f;
        result->h3 = 0.0f;
        result->h5 = 0.0f;
        result->thd = 0.0f;
        return;
    }

    if(buf==NULL || result==NULL || len==0 || sample_rate==0) return;

    for(uint16_t i=0; i<len; i++)
        fft_in[i] = (float)buf[i] - result->avg;

    /* FFT实例只初始化一次，避免反复init导致内部状态异常 */
    if(!fft_init_done)
    {
        if(arm_rfft_fast_init_f32(&fft_s, len) != ARM_MATH_SUCCESS) return;
        fft_init_done = 1;
    }

    arm_rfft_fast_f32(&fft_s, fft_in, fft_out, 0);

    mag[0] = fabsf(fft_out[0]) / len;
    for(uint16_t i=1; i<len/2; i++)
    {
        float re = fft_out[2*i];
        float im = fft_out[2*i+1];
        mag[i] = 2.0f * sqrtf(re*re + im*im) / len;
    }

    /* 自动寻找基波（跳过DC bin） */
    uint16_t base_idx = 0;
    float max_mag = 0.0f;
    for(uint16_t i=1; i<len/2; i++)
    {
        if(mag[i] > max_mag)
        {
            max_mag = mag[i];
            base_idx = i;
        }
    }

    /* 未找到有效基波（全噪声或DC），安全返回 */
    if(base_idx == 0 || max_mag < 1e-6f)
    {
        result->h1 = 0.0f;
        result->h3 = 0.0f;
        result->h5 = 0.0f;
        result->thd = 0.0f;
        return;
    }

    result->h1 = mag[base_idx];
    result->h3 = ((3 * base_idx) < (len / 2)) ? mag[3 * base_idx] : 0.0f;
    result->h5 = ((5 * base_idx) < (len / 2)) ? mag[5 * base_idx] : 0.0f;

    if(result->h1 < 1e-3f)
    {
        result->thd = 0.0f;
    }
    else
    {
        result->thd = sqrtf(result->h3 * result->h3 + result->h5 * result->h5) / result->h1 * 100.0f;
    }
}

/**
 * @brief 对一波形数据进行完整测量分析
 */
void fft_analyze_signal(uint16_t *buf, uint16_t len, uint32_t sample_rate, uint16_t threshold, adc_signal_result_t *result)
{
    if(buf==NULL || result==NULL) return;

    memset(result, 0, sizeof(adc_signal_result_t));
    fft_calc_basic_params(buf,len,result);
    fft_calc_time_params(buf,len,sample_rate,threshold,result);
    fft_calc_harmonics(buf,len,sample_rate,result);
}
