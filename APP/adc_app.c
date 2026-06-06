#include "adc_app.h"
#include "GUI.h"
#include "Lcd_Driver.h"
#include "mydefine.h"

/* DMA原始缓冲区：交织存放 ch1 ch2 ch3 */
uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];
volatile uint16_t *p = NULL;

/* 拆分后的三路数组 */
uint16_t lcd_adc_ch1_buf[ADC_SAMPLES];
uint16_t lcd_adc_ch2_buf[ADC_SAMPLES];
uint16_t lcd_adc_ch3_buf[ADC_SAMPLES];
	
uint16_t ttf_adc_ch1_buf[ADC_SAMPLES];
uint16_t ttf_adc_ch2_buf[ADC_SAMPLES];
uint16_t ttf_adc_ch3_buf[ADC_SAMPLES];	
/* 预留给显示和算法的缓存 */


/* DMA半传输/全传输完成标志 */
volatile uint8_t adc_flag = 0;
static volatile uint8_t dma_startup_cnt = 0;
static volatile uint8_t last_adc_flag = 0;


/* 内部函数声明 */
static void adc_gpio_init(void);
static void adc_dma_init(void);
static void adc_core_init(void);
static void tim3_init(uint32_t sample_rate);


/**
 * @brief ADC1采样系统初始化
 * @param sample_rate 每个通道采样率，单位Hz
 */
void adc1_init(uint32_t sample_rate)
{
    adc_gpio_init();
    adc_dma_init();
    adc_core_init();
    tim3_init(sample_rate);

    /* 使能ADC DMA请求 */
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);

    /* 使能ADC */
    ADC_Cmd(ADC1, ENABLE);

//    /* F407 建议做一次校准/启动等待 */
//    ADC_ResetCalibration(ADC1);
//    while (ADC_GetResetCalibrationStatus(ADC1));

//    ADC_StartCalibration(ADC1);
//    while (ADC_GetCalibrationStatus(ADC1));

    /* 启动DMA */
    DMA_Cmd(DMA2_Stream0, ENABLE);

	adc_flag = 0;
	dma_startup_cnt = 0;
	last_adc_flag = 0;
	p = NULL;
	memset(g_adc_dma_buf, 0, sizeof(g_adc_dma_buf));
	memset(lcd_adc_ch1_buf, 0, sizeof(lcd_adc_ch1_buf));
	memset(lcd_adc_ch2_buf, 0, sizeof(lcd_adc_ch2_buf));
	memset(lcd_adc_ch3_buf, 0, sizeof(lcd_adc_ch3_buf));
	memset(ttf_adc_ch1_buf, 0, sizeof(ttf_adc_ch1_buf));
	memset(ttf_adc_ch2_buf, 0, sizeof(ttf_adc_ch2_buf));
	memset(ttf_adc_ch3_buf, 0, sizeof(ttf_adc_ch3_buf));
    /* 启动TIM3，开始触发ADC */
    TIM_Cmd(TIM3, ENABLE);
	

}

/**
 * @brief GPIO初始化：PA3 PA4 PA5作为模拟输入
 */
static void adc_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief DMA2_Stream0初始化，用于ADC1搬运数据
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

    /* 开启半传输和全传输中断 */
    DMA_ITConfig(DMA2_Stream0, DMA_IT_HT, ENABLE);
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief ADC1初始化：3通道扫描，TIM3触发
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

    /* 配置3个规则通道：PA3/PA4/PA5 -> ADC_Channel_3/4/5 */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_84Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 2, ADC_SampleTime_84Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 3, ADC_SampleTime_84Cycles);
}

/**
 * @brief TIM3初始化，用于按固定频率触发ADC
 * @param sample_rate 每个通道采样率
 *
 * 说明：
 * 一次TIM触发会完成3个通道转换，因此这里的触发频率就按“每通道采样率”配置即可。
 */
static void tim3_init(uint32_t sample_rate)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    uint32_t tim_clk = 84000000;
    uint32_t prescaler = 84 - 1;       /* 84MHz / 84 = 1MHz */
    uint32_t period;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 计数频率 = 1MHz，则周期 = 1000000 / sample_rate */
    period = (1000000 / sample_rate) - 1;

    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* 选择更新事件作为TRGO输出 */
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
}

/**
 * @brief DMA中断服务函数
 * @note 中断里只置标志，不做复杂运算
 */
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_HTIF0) != RESET)
    {

		p= g_adc_dma_buf;
        adc_flag = 1;   /* 前半缓冲可处理 */

        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0);
    }

    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0) != RESET)
    {

		p= g_adc_dma_buf+ ADC_CH_NUM * ADC_SAMPLES;;
        adc_flag = 2;   /* 后半缓冲可处理 */

        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
    }
}



adc_signal_result_t ch1_result, ch2_result, ch3_result;

/**
 * @brief ADC数据处理函数
 * @note 在主循环或调度器中调用
 *
 * 工作内容：
 * 1. 判断DMA是前半帧还是后半帧准备好了
 * 2. 拆分三通道数据
 * 3. 准备LCD和TTF工作缓存
 * 4. 调用lcd_proc() / ttf_proc()
 */
void adc_proc(void)
{
    uint16_t *q;
    uint8_t current_flag;

    /* Skip first 8 DMA IRQs (~200ms) for ADC/DMA stabilization */
    if(dma_startup_cnt < 8)
    {
        dma_startup_cnt++;
        return;
    }

    __disable_irq();
    current_flag = adc_flag;
    if(current_flag == 0 || p == NULL)
    {
        __enable_irq();
        return;
    }

    /* Skip if same frame as last time */
    if(current_flag == last_adc_flag)
    {
        adc_flag = 0;
        __enable_irq();
        return;
    }

    q = (uint16_t *)p;
    last_adc_flag = current_flag;
    adc_flag = 0;
    __enable_irq();

    for(uint16_t i = 0; i < ADC_SAMPLES; i++)
    {
        ttf_adc_ch1_buf[i] = *q++;
        ttf_adc_ch2_buf[i] = *q++;
        ttf_adc_ch3_buf[i] = *q++;
    }

    /* 后面做FFT或参数计算 */
    fft_analyze_signal(ttf_adc_ch1_buf, ADC_SAMPLES, 20000, 2048, &ch1_result);
    fft_analyze_signal(ttf_adc_ch2_buf, ADC_SAMPLES, 20000, 2048, &ch2_result);
    fft_analyze_signal(ttf_adc_ch3_buf, ADC_SAMPLES, 20000, 2048, &ch3_result);
	
	lcd_printf(0, 0, RED, GRAY0, "F=%.1fHz       \rD=%.1f %%    ", ch2_result.freq, ch2_result.duty);
	lcd_printf(0, 32, BLUE, GRAY0, "Vpp=%d        \rRMS=%.1f    ", ch2_result.vpp, ch2_result.rms);
	//lcd_printf(0, 64, GREEN, GRAY0, "H1=%.1f   \rH3=          \rTHD=%.1f %%", ch2_result.h3, ch2_result.thd);
	lcd_printf(0, 96, RED, GRAY0,
           "  \rH3 = %.3f  \rH5 = %.3f \r",
            ch2_result.h3, ch2_result.h5);
}

