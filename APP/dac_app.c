#include "dac_app.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* Private variables */
static uint16_t waveform_buffer[WAVEFORM_SAMPLES];
static dac_waveform_t current_waveform = WAVEFORM_SINE;
static uint32_t current_frequency_hz = 1000U;
static uint16_t current_peak_amplitude_mv = DAC_VREF_MV / 2U;
static uint16_t dac_amplitude_raw = DAC_MAX_VALUE / 2U;
static uint8_t zero_based_waveform = 1U;
static uint8_t adc_sync_enabled = ADC_DAC_SYNC_ENABLE;
static uint8_t adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER;

/* Private function declarations */
static void dac_gpio_init(void);
static void dac_tim6_init(void);
static void dac_dma_init(void);
static void dac_peripheral_init(void);
static uint32_t get_tim6_clock_hz(void);
static ErrorStatus update_timer_frequency(void);
static ErrorStatus update_adc_timer_frequency(void);
static void generate_waveform(void);
static void generate_sine(uint16_t amp_raw);
static void generate_square(uint16_t amp_raw);
static void generate_triangle(uint16_t amp_raw);
static void start_dac_output(void);
static void stop_dac_output(void);
static void amplitude_mv_to_raw(uint16_t peak_amplitude_mv);

static uint32_t get_tim6_clock_hz(void)
{
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);

    /* TIM6 is on APB1. On STM32F4, if APB1 prescaler != 1, timer clock = PCLK1 * 2. */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1)
    {
        return clocks.PCLK1_Frequency;
    }
    else
    {
        return clocks.PCLK1_Frequency * 2U;
    }
}

static void dac_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;          /* DAC_OUT1: PA4 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void dac_tim6_init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

    TIM_DeInit(TIM6);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 0U;
    TIM_TimeBaseStructure.TIM_Period = 100U - 1U;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

    /* TIM6 Update Event as TRGO, used to trigger DAC conversion. */
    TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
    TIM_ARRPreloadConfig(TIM6, ENABLE);
}

static void dac_dma_init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    DMA_Cmd(DMA1_Stream5, DISABLE);
    while (DMA_GetCmdStatus(DMA1_Stream5) != DISABLE)
    {
    }

    DMA_DeInit(DMA1_Stream5);

    DMA_InitStructure.DMA_Channel = DMA_Channel_7;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R1;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)waveform_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize = WAVEFORM_SAMPLES;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    DMA_Init(DMA1_Stream5, &DMA_InitStructure);
}

static void dac_peripheral_init(void)
{
    DAC_InitTypeDef DAC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    DAC_DeInit();

    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
}

static void start_dac_output(void)
{
    TIM_Cmd(TIM6, DISABLE);
    DMA_Cmd(DMA1_Stream5, DISABLE);
    while (DMA_GetCmdStatus(DMA1_Stream5) != DISABLE)
    {
    }

    DMA_SetCurrDataCounter(DMA1_Stream5, WAVEFORM_SAMPLES);
    DAC_DMACmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DMA_Cmd(DMA1_Stream5, ENABLE);
    TIM_SetCounter(TIM6, 0U);
    TIM_Cmd(TIM6, ENABLE);
}

static void stop_dac_output(void)
{
    TIM_Cmd(TIM6, DISABLE);
    DAC_DMACmd(DAC_Channel_1, DISABLE);
    DMA_Cmd(DMA1_Stream5, DISABLE);
    while (DMA_GetCmdStatus(DMA1_Stream5) != DISABLE)
    {
    }
}

static ErrorStatus update_timer_frequency(void)
{
    uint64_t dac_update_freq;
    uint32_t timer_clk;
    uint32_t prescaler;
    uint32_t arr;
    uint32_t div_factor;

    if ((current_frequency_hz == 0U) || (WAVEFORM_SAMPLES == 0U))
    {
        return ERROR;
    }

    dac_update_freq = (uint64_t)current_frequency_hz * (uint64_t)WAVEFORM_SAMPLES;
    timer_clk = get_tim6_clock_hz();

    if ((timer_clk == 0U) || (dac_update_freq == 0U) || (dac_update_freq > timer_clk))
    {
        return ERROR;
    }

    prescaler = 0U;
    arr = (uint32_t)(timer_clk / dac_update_freq);
    if (arr == 0U)
    {
        arr = 1U;
    }
    arr -= 1U;

    if (arr > 0xFFFFU)
    {
        div_factor = (arr / 0xFFFFU) + 1U;
        prescaler = div_factor - 1U;
        arr = (uint32_t)(timer_clk / (dac_update_freq * (uint64_t)(prescaler + 1U)));
        if (arr == 0U)
        {
            arr = 1U;
        }
        arr -= 1U;

        if ((prescaler > 0xFFFFU) || (arr > 0xFFFFU))
        {
            return ERROR;
        }
    }

    TIM_Cmd(TIM6, DISABLE);
    TIM_PrescalerConfig(TIM6, (uint16_t)prescaler, TIM_PSCReloadMode_Immediate);
    TIM_SetAutoreload(TIM6, (uint16_t)arr);
    TIM_GenerateEvent(TIM6, TIM_EventSource_Update);

    if (adc_sync_enabled != 0U)
    {
        (void)update_adc_timer_frequency();
    }

    return SUCCESS;
}

static ErrorStatus update_adc_timer_frequency(void)
{
#if ADC_DAC_SYNC_ENABLE
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    uint64_t dac_update_freq;
    uint64_t adc_sample_freq;
    uint32_t timer_clk;
    uint32_t arr;

    if ((adc_sampling_multiplier == 0U) || (current_frequency_hz == 0U))
    {
        return ERROR;
    }

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    dac_update_freq = (uint64_t)current_frequency_hz * (uint64_t)WAVEFORM_SAMPLES;
    adc_sample_freq = dac_update_freq * (uint64_t)adc_sampling_multiplier;
    timer_clk = get_tim6_clock_hz(); /* TIM3 is also on APB1 in STM32F407. */

    if ((adc_sample_freq == 0U) || (timer_clk == 0U))
    {
        return ERROR;
    }
    if (adc_sample_freq > timer_clk)
    {
        adc_sample_freq = timer_clk;
    }

    arr = (uint32_t)(timer_clk / adc_sample_freq);
    if (arr == 0U)
    {
        arr = 1U;
    }
    if (arr > 0xFFFFU)
    {
        arr = 0xFFFFU;
    }

    TIM_Cmd(TIM3, DISABLE);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 0U;
    TIM_TimeBaseStructure.TIM_Period = (uint16_t)(arr - 1U);
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    TIM_GenerateEvent(TIM3, TIM_EventSource_Update);
    TIM_Cmd(TIM3, ENABLE);

    return SUCCESS;
#else
    return SUCCESS;
#endif
}

static void generate_sine(uint16_t amp_raw)
{
    uint32_t i;
    float step = 2.0f * PI / (float)WAVEFORM_SAMPLES;

    if (zero_based_waveform != 0U)
    {
        uint32_t full_amp = (uint32_t)amp_raw * 2U;
        if (full_amp > DAC_MAX_VALUE)
        {
            full_amp = DAC_MAX_VALUE;
        }

        for (i = 0U; i < WAVEFORM_SAMPLES; i++)
        {
            float val = (sinf((float)i * step) + 1.0f) * 0.5f;
            waveform_buffer[i] = (uint16_t)(val * (float)full_amp);
        }
    }
    else
    {
        for (i = 0U; i < WAVEFORM_SAMPLES; i++)
        {
            int32_t dac_val = (int32_t)((sinf((float)i * step) * (float)amp_raw) + ((float)DAC_MAX_VALUE / 2.0f));
            if (dac_val < 0)
            {
                dac_val = 0;
            }
            else if (dac_val > (int32_t)DAC_MAX_VALUE)
            {
                dac_val = (int32_t)DAC_MAX_VALUE;
            }
            waveform_buffer[i] = (uint16_t)dac_val;
        }
    }
}

static void generate_square(uint16_t amp_raw)
{
    uint32_t i;
    uint32_t half_samples = WAVEFORM_SAMPLES / 2U;
    uint16_t high_val;
    uint16_t low_val;

    if (zero_based_waveform != 0U)
    {
        uint32_t full_amp = (uint32_t)amp_raw * 2U;
        high_val = (full_amp > DAC_MAX_VALUE) ? (uint16_t)DAC_MAX_VALUE : (uint16_t)full_amp;
        low_val = 0U;
    }
    else
    {
        int32_t center = (int32_t)(DAC_MAX_VALUE / 2U);
        int32_t high = center + (int32_t)amp_raw;
        int32_t low = center - (int32_t)amp_raw;
        high_val = (high > (int32_t)DAC_MAX_VALUE) ? (uint16_t)DAC_MAX_VALUE : (uint16_t)high;
        low_val = (low < 0) ? 0U : (uint16_t)low;
    }

    for (i = 0U; i < half_samples; i++)
    {
        waveform_buffer[i] = high_val;
    }
    for (i = half_samples; i < WAVEFORM_SAMPLES; i++)
    {
        waveform_buffer[i] = low_val;
    }
}

static void generate_triangle(uint16_t amp_raw)
{
    uint32_t i;
    uint32_t half_samples = WAVEFORM_SAMPLES / 2U;
    uint16_t high_val;
    uint16_t low_val;

    if (zero_based_waveform != 0U)
    {
        uint32_t full_amp = (uint32_t)amp_raw * 2U;
        high_val = (full_amp > DAC_MAX_VALUE) ? (uint16_t)DAC_MAX_VALUE : (uint16_t)full_amp;
        low_val = 0U;
    }
    else
    {
        int32_t center = (int32_t)(DAC_MAX_VALUE / 2U);
        int32_t high = center + (int32_t)amp_raw;
        int32_t low = center - (int32_t)amp_raw;
        high_val = (high > (int32_t)DAC_MAX_VALUE) ? (uint16_t)DAC_MAX_VALUE : (uint16_t)high;
        low_val = (low < 0) ? 0U : (uint16_t)low;
    }

    for (i = 0U; i < half_samples; i++)
    {
        waveform_buffer[i] = low_val + (uint16_t)(((uint32_t)(high_val - low_val) * i) / half_samples);
    }
    for (i = half_samples; i < WAVEFORM_SAMPLES; i++)
    {
        waveform_buffer[i] = high_val - (uint16_t)(((uint32_t)(high_val - low_val) * (i - half_samples)) / (WAVEFORM_SAMPLES - half_samples));
    }

    waveform_buffer[0] = low_val;
    waveform_buffer[half_samples - 1U] = high_val;
    waveform_buffer[WAVEFORM_SAMPLES - 1U] = low_val;
}

static void generate_waveform(void)
{
    switch (current_waveform)
    {
    case WAVEFORM_SINE:
        generate_sine(dac_amplitude_raw);
        break;
    case WAVEFORM_SQUARE:
        generate_square(dac_amplitude_raw);
        break;
    case WAVEFORM_TRIANGLE:
        generate_triangle(dac_amplitude_raw);
        break;
    default:
        generate_sine(dac_amplitude_raw);
        break;
    }
}

static void amplitude_mv_to_raw(uint16_t peak_amplitude_mv)
{
    uint16_t max_amplitude_mv = DAC_VREF_MV / 2U;

    if (peak_amplitude_mv > max_amplitude_mv)
    {
        peak_amplitude_mv = max_amplitude_mv;
    }

    current_peak_amplitude_mv = peak_amplitude_mv;
    dac_amplitude_raw = (uint16_t)(((uint32_t)current_peak_amplitude_mv * (DAC_MAX_VALUE / 2U)) / max_amplitude_mv);

    if (dac_amplitude_raw > (DAC_MAX_VALUE / 2U))
    {
        dac_amplitude_raw = DAC_MAX_VALUE / 2U;
    }
}

void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv)
{
    current_frequency_hz = (initial_freq_hz == 0U) ? 1U : initial_freq_hz;
    amplitude_mv_to_raw(initial_peak_amplitude_mv);
    generate_waveform();

    adc_sync_enabled = ADC_DAC_SYNC_ENABLE;
    adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER;

    dac_gpio_init();
    dac_tim6_init();
    dac_dma_init();
    dac_peripheral_init();

    if (update_timer_frequency() == SUCCESS)
    {
        start_dac_output();
    }
}

ErrorStatus dac_app_set_waveform(dac_waveform_t type)
{
    stop_dac_output();
    current_waveform = type;
    generate_waveform();
    start_dac_output();
    return SUCCESS;
}

ErrorStatus dac_app_set_frequency(uint32_t freq_hz)
{
    if (freq_hz == 0U)
    {
        return ERROR;
    }

    stop_dac_output();
    current_frequency_hz = freq_hz;
    if (update_timer_frequency() != SUCCESS)
    {
        return ERROR;
    }
    start_dac_output();
    return SUCCESS;
}

ErrorStatus dac_app_set_amplitude(uint16_t peak_amplitude_mv)
{
    stop_dac_output();
    amplitude_mv_to_raw(peak_amplitude_mv);
    generate_waveform();
    start_dac_output();
    return SUCCESS;
}

uint16_t dac_app_get_amplitude(void)
{
    return current_peak_amplitude_mv;
}

ErrorStatus dac_app_set_zero_based(uint8_t enable)
{
    stop_dac_output();
    zero_based_waveform = (enable != 0U) ? 1U : 0U;
    generate_waveform();
    start_dac_output();
    return SUCCESS;
}

uint8_t dac_app_get_zero_based(void)
{
    return zero_based_waveform;
}

ErrorStatus dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier)
{
    if (multiplier == 0U)
    {
        return ERROR;
    }

    adc_sync_enabled = (enable != 0U) ? 1U : 0U;
    adc_sampling_multiplier = multiplier;

    if (adc_sync_enabled != 0U)
    {
        return update_adc_timer_frequency();
    }

#if ADC_DAC_SYNC_ENABLE
    TIM_Cmd(TIM3, DISABLE);
#endif
    return SUCCESS;
}

uint32_t dac_app_get_update_frequency(void)
{
    return current_frequency_hz * WAVEFORM_SAMPLES;
}

float dac_app_get_adc_sampling_interval_us(void)
{
    uint64_t dac_update_freq;
    uint64_t adc_sample_freq;

    if ((adc_sync_enabled == 0U) || (current_frequency_hz == 0U) || (adc_sampling_multiplier == 0U))
    {
        return 0.0f;
    }

    dac_update_freq = (uint64_t)current_frequency_hz * (uint64_t)WAVEFORM_SAMPLES;
    adc_sample_freq = dac_update_freq * (uint64_t)adc_sampling_multiplier;

    if (adc_sample_freq == 0U)
    {
        return 0.0f;
    }

    return 1000000.0f / (float)adc_sample_freq;
}
