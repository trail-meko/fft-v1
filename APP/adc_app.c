#include "adc_app.h"
#include "mydefine.h"
#include "system_state.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif
/* DMAԭʼ���ݻ���������֯��� ch1 ch2 ch3 */
uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];
volatile uint16_t *p = NULL;

/* ��ͨ������ */
uint16_t lcd_adc_ch1_buf[ADC_SAMPLES];
uint16_t lcd_adc_ch2_buf[ADC_SAMPLES];
uint16_t lcd_adc_ch3_buf[ADC_SAMPLES];

uint16_t ttf_adc_ch1_buf[ADC_SAMPLES];
uint16_t ttf_adc_ch2_buf[ADC_SAMPLES];
uint16_t ttf_adc_ch3_buf[ADC_SAMPLES];

/* DMA�봫��/ȫ������ɱ�־ */
volatile uint8_t adc_flag = 0;

/* �ڲ��������� */
static void adc_gpio_init(void);
static void adc_dma_init(void);
static void adc_core_init(void);
static void tim3_init(uint32_t sample_rate);

adc_signal_result_t ch1_result, ch2_result, ch3_result;

/**
 * @brief ADC1����ϵͳ��ʼ��
 * @param sample_rate ÿ��ͨ���Ĳ����ʣ���λ Hz
 */
void adc1_init(uint32_t sample_rate)
{
    adc_gpio_init();
    adc_dma_init();
    adc_core_init();
    tim3_init(sample_rate);

    /* ʹ��ADC DMA���� */
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);

    /* ʹ��ADC */
    ADC_Cmd(ADC1, ENABLE);

    /* ʹ��DMA */
    DMA_Cmd(DMA2_Stream0, ENABLE);

    adc_flag = 0;
    p = NULL;

    memset(g_adc_dma_buf, 0, sizeof(g_adc_dma_buf));
    memset(lcd_adc_ch1_buf, 0, sizeof(lcd_adc_ch1_buf));
    memset(lcd_adc_ch2_buf, 0, sizeof(lcd_adc_ch2_buf));
    memset(lcd_adc_ch3_buf, 0, sizeof(lcd_adc_ch3_buf));
    memset(ttf_adc_ch1_buf, 0, sizeof(ttf_adc_ch1_buf));
    memset(ttf_adc_ch2_buf, 0, sizeof(ttf_adc_ch2_buf));
    memset(ttf_adc_ch3_buf, 0, sizeof(ttf_adc_ch3_buf));

    /* ����TIM3����ʱ����ADC */
    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief GPIO��ʼ��������PA5 PA6 PA7Ϊģ������
 *
 * PA5 -> ADC_Channel_5
 * PA6 -> ADC_Channel_6
 * PA7 -> ADC_Channel_7
 */
static void adc_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief DMA2_Stream0��ʼ�������ڰ���ADC1ת�����
 */
static void adc_dma_init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    DMA_DeInit(DMA2_Stream0);
    while (DMA_GetCmdStatus(DMA2_Stream0) != DISABLE);

    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)g_adc_dma_buf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = ADC_DMA_BUF_SIZE;
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
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);

    /* �����봫�䡢ȫ�����ж� */
    DMA_ITConfig(DMA2_Stream0, DMA_IT_HT, ENABLE);
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief ADC1��ʼ����3ͨ��ɨ�裬TIM3����
 *
 * ˳��Ϊ��
 *   ��1��ת����PA5 -> ADC_Channel_5 -> ch1
 *   ��2��ת����PA6 -> ADC_Channel_6 -> ch2
 *   ��3��ת����PA7 -> ADC_Channel_7 -> ch3
 */
static void adc_core_init(void)
{
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = ADC_CH_NUM;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ����3������ͨ����PA5/PA6/PA7 -> ADC_Channel_5/6/7 */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_84Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 2, ADC_SampleTime_84Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 3, ADC_SampleTime_84Cycles);
}

/**
 * @brief TIM3��ʼ�������ڰ�ָ�������ʴ���ADC
 * @param sample_rate ÿ��ͨ��������
 *
 * ˵����
 * һ��TIM�����¼����ᴥ��һ��ADC������ת��
 * �������а���3��ͨ��������ÿ�δ������������ ch1/ch2/ch3 ���β���
 */
static void tim3_init(uint32_t sample_rate)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    uint32_t prescaler = 84 - 1;      /* 84MHz / 84 = 1MHz */
    uint32_t period;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* ��ʱ������Ƶ�� = 1MHz / (period + 1) = sample_rate */
    period = (1000000 / sample_rate) - 1;

    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* ѡ������¼���ΪTRGO��� */
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
}

/**
 * @brief DMA�жϷ�����
 * @note ����ֻ�ñ�־λ�ͻ�����ָ�룬������ʱ����
 */
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_HTIF0) != RESET)
    {
        p = g_adc_dma_buf;
        adc_flag = 1;   /* ǰ�뻺�����ɴ��� */

        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0);
    }

    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0) != RESET)
    {
        p = g_adc_dma_buf + ADC_CH_NUM * ADC_SAMPLES;
        adc_flag = 2;   /* ��뻺�����ɴ��� */

        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
    }
}

/**
 * @brief ADC���ݴ�������
 * @note ����ѭ�������ڵ���
 *
 * �������̣�
 * 1. �ж�DMA��һ�뻺����׼����
 * 2. ����֯���ݲ�ֵ���ͨ������
 * 3. ����FFT����
 */
/**
 * @brief 计算ADC缓冲的交流有效值 (AC RMS, 去直流)
 * @param  buf  原始ADC缓冲 (0-4095)
 * @param  len  缓冲长度
 * @return      交流有效值 (V)
 */
static float adc_compute_ac_rms_volts(uint16_t *buf, uint16_t len)
{
    uint32_t i;
    float sum;
    float dc;
    float sum_sq;
    float diff;

    if (buf == NULL || len == 0)
    {
        return 0.0f;
    }

    /* 计算直流偏置 */
    sum = 0.0f;
    for (i = 0; i < len; i++)
    {
        sum += (float)buf[i];
    }
    dc = sum / (float)len;

    /* 计算去直流后的RMS (ADC原始值) */
    sum_sq = 0.0f;
    for (i = 0; i < len; i++)
    {
        diff = (float)buf[i] - dc;
        sum_sq += diff * diff;
    }

    /* 转换为电压: ADC值 / 4096 * 3.3V */
    return sqrtf(sum_sq / (float)len) / 4096.0f * 3.3f;
}

void adc_proc(void)
{
    uint16_t *q;

    __disable_irq();
    if (adc_flag == 0 || p == NULL)
    {
        __enable_irq();
        return;
    }

    q = (uint16_t *)p;
    adc_flag = 0;
    __enable_irq();

    for (uint16_t i = 0; i < ADC_SAMPLES; i++)
    {
        /* ˳���� ADC_RegularChannelConfig ����һ�� */
        ttf_adc_ch1_buf[i] = *q++;   /* PA5 -> ADC_Channel_5 */
        ttf_adc_ch2_buf[i] = *q++;   /* PA6 -> ADC_Channel_6 */
        ttf_adc_ch3_buf[i] = *q++;   /* PA7 -> ADC_Channel_7 */
    }
	printf("%d   %d   %d\r\n",ttf_adc_ch1_buf[0],ttf_adc_ch2_buf[0],ttf_adc_ch3_buf[0]);
	
    /* FFT分析 */
    fft_analyze_signal(ttf_adc_ch1_buf, ADC_SAMPLES, 20000, 2048, &ch1_result);
    fft_analyze_signal(ttf_adc_ch2_buf, ADC_SAMPLES, 20000, 2048, &ch2_result);
    fft_analyze_signal(ttf_adc_ch3_buf, ADC_SAMPLES, 20000, 2048, &ch3_result);

    /* ---- 竞赛测量: AC RMS, 增益, 相位差, AGC ---- */

    /* Ui = ch1 (PA5), Uo = ch2 (PA6) */
    g_sys.ui_rms = adc_compute_ac_rms_volts(
        (g_sys.ui_ch == 0) ? ttf_adc_ch1_buf :
        (g_sys.ui_ch == 1) ? ttf_adc_ch2_buf : ttf_adc_ch3_buf,
        ADC_SAMPLES);

    g_sys.uo_rms = adc_compute_ac_rms_volts(
        (g_sys.uo_ch == 0) ? ttf_adc_ch1_buf :
        (g_sys.uo_ch == 1) ? ttf_adc_ch2_buf : ttf_adc_ch3_buf,
        ADC_SAMPLES);

    /* 测量频率 (从FFT结果获取) */
    {
        adc_signal_result_t *ui_res = (g_sys.ui_ch == 0) ? &ch1_result :
                                      (g_sys.ui_ch == 1) ? &ch2_result : &ch3_result;
        adc_signal_result_t *uo_res = (g_sys.uo_ch == 0) ? &ch1_result :
                                      (g_sys.uo_ch == 1) ? &ch2_result : &ch3_result;
        g_sys.ui_freq = ui_res->freq;
        g_sys.uo_freq = uo_res->freq;

        /* 增益 = Uo / Ui */
        if (g_sys.ui_rms > 0.001f)
        {
            g_sys.gain = g_sys.uo_rms / g_sys.ui_rms;
        }
        else
        {
            g_sys.gain = 0.0f;
        }

        /* 相位差 = phase(Uo) - phase(Ui), 归一化到 [-180, 180] */
        {
            float pd = uo_res->phase - ui_res->phase;
            while (pd > PI)  pd -= 2.0f * PI;
            while (pd <= -PI) pd += 2.0f * PI;
            g_sys.phase_diff_deg = pd * 180.0f / PI;
        }
    }

    /* AGC 闭环由 TIM4 高优先级中断独立运行 (pid_controller.c) */
}


