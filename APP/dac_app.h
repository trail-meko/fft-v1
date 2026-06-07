#ifndef __DAC_APP_H
#define __DAC_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_dma.h"
#include <stdint.h>

/*
 * STM32F407 StdPeriph DAC waveform output module
 * DAC1_CH1 -> PA4
 * Trigger  -> TIM6 TRGO Update
 * DMA      -> DMA1_Stream5 / DMA_Channel_7
 */

#define DAC_RESOLUTION_BITS     12U
#define DAC_MAX_VALUE           ((1U << DAC_RESOLUTION_BITS) - 1U)  /* 4095 */
#define DAC_VREF_MV             3300U
#define WAVEFORM_SAMPLES        256U

/* ADC sync option: use TIM3 as ADC trigger timer if your project needs it. */
#define ADC_DAC_SYNC_ENABLE     1U
#define ADC_SAMPLING_MULTIPLIER 1U

typedef enum
{
    WAVEFORM_SINE = 0,
    WAVEFORM_SQUARE,
    WAVEFORM_TRIANGLE
} dac_waveform_t;

/* Public API */
void dac_app_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv);
ErrorStatus dac_app_set_waveform(dac_waveform_t type);
ErrorStatus dac_app_set_frequency(uint32_t freq_hz);
ErrorStatus dac_app_set_amplitude(uint16_t peak_amplitude_mv);
uint16_t dac_app_get_amplitude(void);

ErrorStatus dac_app_set_zero_based(uint8_t enable);
uint8_t dac_app_get_zero_based(void);

ErrorStatus dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier);
uint32_t dac_app_get_update_frequency(void);
float dac_app_get_adc_sampling_interval_us(void);

#ifdef __cplusplus
}
#endif

#endif /* __DAC_APP_H */
