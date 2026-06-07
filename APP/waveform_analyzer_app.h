#ifndef __WAVEFORM_ANALYZER_APP_H
#define __WAVEFORM_ANALYZER_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include "dac_app.h"
#include "arm_math.h"
#include <stdint.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define FFT_LENGTH 1024U

typedef enum
{
    ADC_WAVEFORM_DC = 0,
    ADC_WAVEFORM_SINE = 1,
    ADC_WAVEFORM_SQUARE = 2,
    ADC_WAVEFORM_TRIANGLE = 3,
    ADC_WAVEFORM_UNKNOWN = 255
} ADC_WaveformType;

typedef struct
{
    float frequency;
    float amplitude;
    float phase;
    float relative_amp;
} HarmonicComponent;

typedef struct
{
    ADC_WaveformType waveform_type;
    float frequency;
    float vpp;
    float mean;
    float rms;
    float phase;
    HarmonicComponent third_harmonic;
    HarmonicComponent fifth_harmonic;
} WaveformInfo;

void My_FFT_Init(void);
float Map_Input_To_FFT_Frequency(float input_frequency);
float Map_FFT_To_Input_Frequency(float fft_frequency);
float Get_Waveform_Vpp(uint32_t *adc_val_buffer_f, float *mean, float *rms);
float Get_Waveform_Frequency(uint32_t *adc_val_buffer_f);
ADC_WaveformType Get_Waveform_Type(uint32_t *adc_val_buffer_f);
void Perform_FFT(uint32_t *adc_val_buffer_f);
ADC_WaveformType Analyze_Frequency_And_Type(uint32_t *adc_val_buffer_f, float *signal_frequency);
float Get_Waveform_Phase(uint32_t *adc_val_buffer_f, float frequency);
float Get_Waveform_Phase_ZeroCrossing(uint32_t *adc_val_buffer_f, float frequency);
float Calculate_Phase_Difference(float phase1, float phase2);
float Get_Phase_Difference(uint32_t *adc_val_buffer1, uint32_t *adc_val_buffer2, float frequency);
void Analyze_Harmonics(uint32_t *adc_val_buffer_f, WaveformInfo *waveform_info);
float Get_Component_Phase(float *fft_buffer, int component_idx);
const char *GetWaveformTypeString(ADC_WaveformType waveform);
WaveformInfo Get_Waveform_Info(uint32_t *adc_val_buffer_f);

#ifdef __cplusplus
}
#endif

#endif /* __WAVEFORM_ANALYZER_APP_H */
