#include "waveform_analyzer_app.h"
#include <float.h>
#include <math.h>

arm_cfft_radix4_instance_f32 scfft;
float FFT_InputBuf[FFT_LENGTH * 2U];
float FFT_OutputBuf[FFT_LENGTH];

float Map_Input_To_FFT_Frequency(float input_frequency)
{
    if (input_frequency <= 2600.0f) return input_frequency;
    else if (input_frequency <= 6100.0f) return input_frequency * 2.0f;
    else if (input_frequency <= 8100.0f) return input_frequency * 3.0f;
    else if (input_frequency <= 11100.0f) return input_frequency * 4.0f;
    else if (input_frequency <= 14100.0f) return input_frequency * 5.0f;
    else if (input_frequency <= 17100.0f) return input_frequency * 6.0f;
    else if (input_frequency <= 19600.0f) return input_frequency * 7.0f;
    else if (input_frequency <= 21600.0f) return input_frequency * 8.0f;
    else if (input_frequency <= 25100.0f) return input_frequency * 9.0f;
    else if (input_frequency <= 26600.0f) return input_frequency * 10.0f;
    else if (input_frequency <= 29600.0f) return input_frequency * 11.0f;
    else if (input_frequency <= 32100.0f) return input_frequency * 12.0f;
    else return input_frequency * 13.0f;
}

float Map_FFT_To_Input_Frequency(float fft_frequency)
{
    const float breaks[] = {2600.0f, 6100.0f, 8100.0f, 11100.0f, 14100.0f,
                            17100.0f, 19600.0f, 21600.0f, 25100.0f, 26600.0f,
                            29600.0f, 32100.0f};
    const float dividers[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
                              8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f};
    int i;
    float last_break_fft = breaks[(sizeof(breaks) / sizeof(breaks[0])) - 1U] * dividers[(sizeof(breaks) / sizeof(breaks[0])) - 1U];

    if (fft_frequency > last_break_fft)
    {
        return fft_frequency / dividers[(sizeof(dividers) / sizeof(dividers[0])) - 1U];
    }

    for (i = 0; i < (int)(sizeof(breaks) / sizeof(breaks[0])); i++)
    {
        float break_fft = breaks[i] * dividers[i];
        float next_break_fft = (i < (int)(sizeof(breaks) / sizeof(breaks[0])) - 1) ? breaks[i + 1] * dividers[i + 1] : FLT_MAX;

        if (fft_frequency <= break_fft)
        {
            return fft_frequency / dividers[i];
        }
        else if (fft_frequency < next_break_fft)
        {
            return fft_frequency / dividers[i + 1];
        }
    }

    return fft_frequency / 13.0f;
}

void My_FFT_Init(void)
{
    arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);
}

float Get_Waveform_Vpp(uint32_t *adc_val_buffer_f, float *mean, float *rms)
{
    uint32_t i;
    float min_val = 3.3f;
    float max_val = 0.0f;
    float sum = 0.0f;
    float sum_squares = 0.0f;

    if ((adc_val_buffer_f == 0) || (mean == 0) || (rms == 0))
    {
        return 0.0f;
    }

    for (i = 0U; i < FFT_LENGTH; i++)
    {
        float voltage = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        if (voltage > max_val) max_val = voltage;
        if (voltage < min_val) min_val = voltage;
        sum += voltage;
        sum_squares += voltage * voltage;
    }

    *mean = sum / (float)FFT_LENGTH;
    *rms = sqrtf(sum_squares / (float)FFT_LENGTH);
    return max_val - min_val;
}

void Perform_FFT(uint32_t *adc_val_buffer_f)
{
    uint32_t i;
    if (adc_val_buffer_f == 0)
    {
        return;
    }

    for (i = 0U; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2U * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2U * i + 1U] = 0.0f;
    }

    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);
    arm_cmplx_mag_f32(FFT_InputBuf, FFT_OutputBuf, FFT_LENGTH);
}

float Get_Component_Phase(float *fft_buffer, int component_idx)
{
    float real_part;
    float imag_part;

    if ((fft_buffer == 0) || (component_idx < 0) || (component_idx >= (int)FFT_LENGTH))
    {
        return 0.0f;
    }

    real_part = fft_buffer[2 * component_idx];
    imag_part = fft_buffer[2 * component_idx + 1];
    return atan2f(imag_part, real_part);
}

float Get_Waveform_Phase(uint32_t *adc_val_buffer_f, float frequency)
{
    uint32_t i;
    float fft_frequency;
    float sampling_interval_us;
    float sampling_frequency;
    int fundamental_idx;

    if ((adc_val_buffer_f == 0) || (frequency <= 0.0f))
    {
        return 0.0f;
    }

    for (i = 0U; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2U * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2U * i + 1U] = 0.0f;
    }

    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);

    fft_frequency = Map_Input_To_FFT_Frequency(frequency);
    sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    if (sampling_interval_us <= 0.0f)
    {
        return 0.0f;
    }
    sampling_frequency = 1000000.0f / sampling_interval_us;
    fundamental_idx = (int)(fft_frequency * (float)FFT_LENGTH / sampling_frequency + 0.5f);

    if ((fundamental_idx <= 0) || (fundamental_idx >= (int)FFT_LENGTH / 2))
    {
        return 0.0f;
    }

    return Get_Component_Phase(FFT_InputBuf, fundamental_idx);
}

float Get_Waveform_Phase_ZeroCrossing(uint32_t *adc_val_buffer_f, float frequency)
{
    uint32_t i;
    float sampling_interval_us;
    float sampling_frequency;
    float mean = 0.0f;
    int zero_crossing_idx = -1;
    float prev_val;
    float current_val;
    float fraction;
    float exact_crossing;
    float fft_frequency;
    float samples_per_period;
    float phase;

    if ((adc_val_buffer_f == 0) || (frequency <= 0.0f))
    {
        return 0.0f;
    }

    sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    if (sampling_interval_us <= 0.0f)
    {
        return 0.0f;
    }
    sampling_frequency = 1000000.0f / sampling_interval_us;

    for (i = 0U; i < FFT_LENGTH; i++)
    {
        mean += (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
    }
    mean /= (float)FFT_LENGTH;

    for (i = 1U; i < FFT_LENGTH; i++)
    {
        current_val = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f - mean;
        prev_val = (float)adc_val_buffer_f[i - 1U] / 4096.0f * 3.3f - mean;
        if ((prev_val < 0.0f) && (current_val >= 0.0f))
        {
            zero_crossing_idx = (int)i;
            break;
        }
    }

    if (zero_crossing_idx < 0)
    {
        return 0.0f;
    }

    prev_val = (float)adc_val_buffer_f[zero_crossing_idx - 1] / 4096.0f * 3.3f - mean;
    current_val = (float)adc_val_buffer_f[zero_crossing_idx] / 4096.0f * 3.3f - mean;
    if ((current_val - prev_val) == 0.0f)
    {
        return 0.0f;
    }

    fraction = -prev_val / (current_val - prev_val);
    exact_crossing = (float)(zero_crossing_idx - 1) + fraction;
    fft_frequency = Map_Input_To_FFT_Frequency(frequency);
    samples_per_period = sampling_frequency / fft_frequency;

    if (samples_per_period <= 0.0f)
    {
        return 0.0f;
    }

    phase = 2.0f * PI * exact_crossing / samples_per_period;
    while (phase < 0.0f) phase += 2.0f * PI;
    while (phase >= 2.0f * PI) phase -= 2.0f * PI;

    return phase;
}

float Calculate_Phase_Difference(float phase1, float phase2)
{
    float phase_diff = phase1 - phase2;
    while (phase_diff > PI) phase_diff -= 2.0f * PI;
    while (phase_diff <= -PI) phase_diff += 2.0f * PI;
    return phase_diff;
}

float Get_Waveform_Frequency(uint32_t *adc_val_buffer_f)
{
    uint32_t i;
    float sampling_interval_us;
    float sampling_frequency;
    float max_harmonic = 0.0f;
    int max_harmonic_idx = 0;
    float fft_frequency;

    sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    if (sampling_interval_us <= 0.0f)
    {
        return 0.0f;
    }
    sampling_frequency = 1000000.0f / sampling_interval_us;

    Perform_FFT(adc_val_buffer_f);

    for (i = 1U; i < FFT_LENGTH / 2U; i++)
    {
        if (FFT_OutputBuf[i] > max_harmonic)
        {
            max_harmonic = FFT_OutputBuf[i];
            max_harmonic_idx = (int)i;
        }
    }

    if (FFT_OutputBuf[0] > max_harmonic * 5.0f)
    {
        return 0.0f;
    }

    fft_frequency = (float)max_harmonic_idx * sampling_frequency / (float)FFT_LENGTH;
    return Map_FFT_To_Input_Frequency(fft_frequency);
}

void Analyze_Harmonics(uint32_t *adc_val_buffer_f, WaveformInfo *waveform_info)
{
    uint32_t i;
    float sampling_interval_us;
    float sampling_frequency;
    float magnitude_spectrum[FFT_LENGTH];
    float fft_frequency;
    int fundamental_idx;
    float fundamental_amp;
    float fundamental_phase;
    int expected_third_harmonic_idx;
    int third_search_start;
    int third_search_end;
    float third_harmonic_amp = 0.0f;
    int third_harmonic_idx = 0;
    int expected_fifth_harmonic_idx;
    int fifth_search_start;
    int fifth_search_end;
    float fifth_harmonic_amp = 0.0f;
    int fifth_harmonic_idx = 0;

    if ((adc_val_buffer_f == 0) || (waveform_info == 0) || (waveform_info->frequency <= 0.0f))
    {
        return;
    }

    sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    if (sampling_interval_us <= 0.0f)
    {
        return;
    }
    sampling_frequency = 1000000.0f / sampling_interval_us;

    for (i = 0U; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2U * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2U * i + 1U] = 0.0f;
    }

    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);
    arm_cmplx_mag_f32(FFT_InputBuf, magnitude_spectrum, FFT_LENGTH);

    fft_frequency = Map_Input_To_FFT_Frequency(waveform_info->frequency);
    fundamental_idx = (int)(fft_frequency * (float)FFT_LENGTH / sampling_frequency + 0.5f);

    if ((fundamental_idx <= 0) || (fundamental_idx >= (int)FFT_LENGTH / 2))
    {
        return;
    }

    fundamental_amp = magnitude_spectrum[fundamental_idx];
    fundamental_phase = Get_Component_Phase(FFT_InputBuf, fundamental_idx);

    expected_third_harmonic_idx = 3 * fundamental_idx;
    third_search_start = expected_third_harmonic_idx - fundamental_idx / 4;
    third_search_end = expected_third_harmonic_idx + fundamental_idx / 4;
    if (third_search_start < fundamental_idx) third_search_start = fundamental_idx + 1;
    if (third_search_end >= (int)FFT_LENGTH / 2) third_search_end = (int)FFT_LENGTH / 2 - 1;

    for (i = (uint32_t)third_search_start; i <= (uint32_t)third_search_end; i++)
    {
        if (magnitude_spectrum[i] > third_harmonic_amp)
        {
            third_harmonic_amp = magnitude_spectrum[i];
            third_harmonic_idx = (int)i;
        }
    }

    expected_fifth_harmonic_idx = 5 * fundamental_idx;
    fifth_search_start = expected_fifth_harmonic_idx - fundamental_idx / 4;
    fifth_search_end = expected_fifth_harmonic_idx + fundamental_idx / 4;
    if (fifth_search_start < third_harmonic_idx + 1) fifth_search_start = third_harmonic_idx + 1;
    if (fifth_search_end >= (int)FFT_LENGTH / 2) fifth_search_end = (int)FFT_LENGTH / 2 - 1;

    for (i = (uint32_t)fifth_search_start; i <= (uint32_t)fifth_search_end; i++)
    {
        if (magnitude_spectrum[i] > fifth_harmonic_amp)
        {
            fifth_harmonic_amp = magnitude_spectrum[i];
            fifth_harmonic_idx = (int)i;
        }
    }

    if (third_harmonic_amp < fundamental_amp * 0.05f)
    {
        third_harmonic_amp = 0.0f;
        third_harmonic_idx = 0;
    }
    if (fifth_harmonic_amp < fundamental_amp * 0.05f)
    {
        fifth_harmonic_amp = 0.0f;
        fifth_harmonic_idx = 0;
    }

    waveform_info->third_harmonic.frequency = (third_harmonic_idx > 0) ? Map_FFT_To_Input_Frequency((float)third_harmonic_idx * sampling_frequency / (float)FFT_LENGTH) : 0.0f;
    waveform_info->third_harmonic.amplitude = third_harmonic_amp;
    waveform_info->third_harmonic.phase = (third_harmonic_idx > 0) ? Calculate_Phase_Difference(Get_Component_Phase(FFT_InputBuf, third_harmonic_idx), fundamental_phase) : 0.0f;
    waveform_info->third_harmonic.relative_amp = (fundamental_amp > 0.0f) ? third_harmonic_amp / fundamental_amp : 0.0f;

    waveform_info->fifth_harmonic.frequency = (fifth_harmonic_idx > 0) ? Map_FFT_To_Input_Frequency((float)fifth_harmonic_idx * sampling_frequency / (float)FFT_LENGTH) : 0.0f;
    waveform_info->fifth_harmonic.amplitude = fifth_harmonic_amp;
    waveform_info->fifth_harmonic.phase = (fifth_harmonic_idx > 0) ? Calculate_Phase_Difference(Get_Component_Phase(FFT_InputBuf, fifth_harmonic_idx), fundamental_phase) : 0.0f;
    waveform_info->fifth_harmonic.relative_amp = (fundamental_amp > 0.0f) ? fifth_harmonic_amp / fundamental_amp : 0.0f;
}

ADC_WaveformType Analyze_Frequency_And_Type(uint32_t *adc_val_buffer_f, float *signal_frequency)
{
    uint32_t i;
    float sampling_interval_us;
    float sampling_frequency;
    float dc_component;
    float fundamental_amp = 0.0f;
    int fundamental_idx = 0;
    float fft_frequency;
    float third_harmonic_amp = 0.0f;
    float fifth_harmonic_amp = 0.0f;
    float third_ratio;
    float fifth_ratio;
    int start_idx;
    int end_idx;

    if ((adc_val_buffer_f == 0) || (signal_frequency == 0))
    {
        return ADC_WAVEFORM_UNKNOWN;
    }

    sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    if (sampling_interval_us <= 0.0f)
    {
        *signal_frequency = 0.0f;
        return ADC_WAVEFORM_UNKNOWN;
    }
    sampling_frequency = 1000000.0f / sampling_interval_us;

    Perform_FFT(adc_val_buffer_f);
    dc_component = FFT_OutputBuf[0];

    for (i = 1U; i < FFT_LENGTH / 2U; i++)
    {
        if (FFT_OutputBuf[i] > fundamental_amp)
        {
            fundamental_amp = FFT_OutputBuf[i];
            fundamental_idx = (int)i;
        }
    }

    fft_frequency = (float)fundamental_idx * sampling_frequency / (float)FFT_LENGTH;
    *signal_frequency = Map_FFT_To_Input_Frequency(fft_frequency);

    if (dc_component > fundamental_amp * 5.0f)
    {
        *signal_frequency = 0.0f;
        return ADC_WAVEFORM_DC;
    }
    if (fundamental_amp < 5.0f)
    {
        return ADC_WAVEFORM_UNKNOWN;
    }

    start_idx = 2 * fundamental_idx;
    end_idx = 4 * fundamental_idx;
    if (end_idx > (int)FFT_LENGTH / 2) end_idx = (int)FFT_LENGTH / 2;
    for (i = (uint32_t)start_idx; i < (uint32_t)end_idx; i++)
    {
        if (FFT_OutputBuf[i] > third_harmonic_amp) third_harmonic_amp = FFT_OutputBuf[i];
    }

    start_idx = 4 * fundamental_idx;
    end_idx = 6 * fundamental_idx;
    if (end_idx > (int)FFT_LENGTH / 2) end_idx = (int)FFT_LENGTH / 2;
    for (i = (uint32_t)start_idx; i < (uint32_t)end_idx; i++)
    {
        if (FFT_OutputBuf[i] > fifth_harmonic_amp) fifth_harmonic_amp = FFT_OutputBuf[i];
    }

    if (third_harmonic_amp < fundamental_amp * 0.05f) third_harmonic_amp = 0.0f;
    if (fifth_harmonic_amp < fundamental_amp * 0.05f) fifth_harmonic_amp = 0.0f;

    third_ratio = (third_harmonic_amp > 0.0f) ? third_harmonic_amp / fundamental_amp : 0.0f;
    fifth_ratio = (fifth_harmonic_amp > 0.0f) ? fifth_harmonic_amp / fundamental_amp : 0.0f;

    if ((third_ratio < 0.05f) && (fifth_ratio < 0.05f))
    {
        return ADC_WAVEFORM_SINE;
    }
    else if (third_ratio > (1.0f / 5.0f))
    {
        return ADC_WAVEFORM_SQUARE;
    }
    else if (third_ratio > (1.0f / 15.0f))
    {
        return ADC_WAVEFORM_TRIANGLE;
    }
    else
    {
        return ADC_WAVEFORM_UNKNOWN;
    }
}

ADC_WaveformType Get_Waveform_Type(uint32_t *adc_val_buffer_f)
{
    float frequency = 0.0f;
    return Analyze_Frequency_And_Type(adc_val_buffer_f, &frequency);
}

const char *GetWaveformTypeString(ADC_WaveformType waveform)
{
    switch (waveform)
    {
        case ADC_WAVEFORM_DC:       return "DC";
        case ADC_WAVEFORM_SINE:     return "Sine";
        case ADC_WAVEFORM_TRIANGLE: return "Triangle";
        case ADC_WAVEFORM_SQUARE:   return "Square";
        case ADC_WAVEFORM_UNKNOWN:  return "Unknown";
        default:                    return "Invalid";
    }
}

float Get_Phase_Difference(uint32_t *adc_val_buffer1, uint32_t *adc_val_buffer2, float frequency)
{
    float phase1;
    float phase2;

    if ((adc_val_buffer1 == 0) || (adc_val_buffer2 == 0) || (frequency <= 0.0f))
    {
        return 0.0f;
    }

    phase1 = Get_Waveform_Phase(adc_val_buffer1, frequency);
    phase2 = Get_Waveform_Phase(adc_val_buffer2, frequency);
    return Calculate_Phase_Difference(phase1, phase2);
}

WaveformInfo Get_Waveform_Info(uint32_t *adc_val_buffer_f)
{
    WaveformInfo result;

    result.waveform_type = ADC_WAVEFORM_UNKNOWN;
    result.frequency = 0.0f;
    result.vpp = 0.0f;
    result.mean = 0.0f;
    result.rms = 0.0f;
    result.phase = 0.0f;
    result.third_harmonic.frequency = 0.0f;
    result.third_harmonic.amplitude = 0.0f;
    result.third_harmonic.phase = 0.0f;
    result.third_harmonic.relative_amp = 0.0f;
    result.fifth_harmonic.frequency = 0.0f;
    result.fifth_harmonic.amplitude = 0.0f;
    result.fifth_harmonic.phase = 0.0f;
    result.fifth_harmonic.relative_amp = 0.0f;

    if (adc_val_buffer_f == 0)
    {
        return result;
    }

    result.vpp = Get_Waveform_Vpp(adc_val_buffer_f, &result.mean, &result.rms);
    result.waveform_type = Analyze_Frequency_And_Type(adc_val_buffer_f, &result.frequency);

    if ((result.waveform_type != ADC_WAVEFORM_DC) && (result.frequency > 0.0f))
    {
        result.phase = Get_Waveform_Phase(adc_val_buffer_f, result.frequency);
        Analyze_Harmonics(adc_val_buffer_f, &result);
    }

    return result;
}
