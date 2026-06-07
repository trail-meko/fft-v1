#include "pid_controller.h"
#include "dac_app.h"
#include "system_state.h"
#include "adc_app.h"
#include <math.h>

/* ================================================================
 * PID 参数与状态 (静态, 仅本文件可见)
 * ================================================================ */

static float g_kp;              /* 比例系数 */
static float g_ki;              /* 积分系数 */
static float g_kd;              /* 微分系数 */
static float g_target_rms;      /* 目标有效值 (V) */
static uint16_t g_min_dac;      /* DAC 幅度下限 (mV) */
static uint16_t g_max_dac;      /* DAC 幅度上限 (mV) */

static float g_integral;        /* 积分累加 */
static float g_prev_error;      /* 上一次误差 (用于微分) */
static uint8_t g_lock_counter;  /* 连续锁定计数 */
static uint8_t g_locked;        /* 锁定标志 */

static uint16_t g_pid_period_ms; /* PID 周期 (ms) */

/* 抗积分饱和上限 */
#define INTEGRAL_MAX    300.0f

/* 锁定阈值: 连续 N 次误差 < 1% 目标值 */
#define LOCK_COUNT_MAX  5

/* DAC 幅度更新死区 (mV): 变化小于此值不触发硬件更新 */
#define DAC_DEADBAND_MV 2

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* ================================================================
 * 内部: AC RMS 计算 (去直流)
 * ================================================================ */

/**
 * @brief 计算 ADC 缓冲的交流有效值 (AC RMS)
 * @param  buf  ADC原始数据 (0-4095)
 * @param  len  缓冲长度
 * @return      交流有效值 (V)
 */
static float compute_ac_rms(uint16_t *buf, uint16_t len)
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

    /* 直流偏置 */
    sum = 0.0f;
    for (i = 0; i < len; i++)
    {
        sum += (float)buf[i];
    }
    dc = sum / (float)len;

    /* 去直流后的均方根 (ADC原始值) */
    sum_sq = 0.0f;
    for (i = 0; i < len; i++)
    {
        diff = (float)buf[i] - dc;
        sum_sq += diff * diff;
    }

    /* 转换为电压: V = ADC / 4096 * 3.3 */
    return sqrtf(sum_sq / (float)len) / 4096.0f * 3.3f;
}

/* ================================================================
 * 内部: TIM4 初始化 (高优先级定时器)
 * ================================================================ */

/**
 * @brief 获取 APB1 定时器时钟频率
 * @note  STM32F4: 若 APB1 prescaler != 1, 定时器时钟 = PCLK1 * 2
 */
static uint32_t get_apb1_timer_clock(void)
{
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);

    if ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1)
    {
        return clocks.PCLK1_Frequency;
    }
    else
    {
        return clocks.PCLK1_Frequency * 2U;
    }
}

/**
 * @brief TIM4 初始化: 高优先级定时中断
 * @param period_ms  中断周期 (ms)
 *
 * TIM4 挂载在 APB1, 16位定时器
 * 时钟 = 84MHz (APB1=42MHz, prescaler!=1 所以 x2)
 * PreemptPriority = 0, SubPriority = 0 (最高优先级)
 */
static void tim4_init(uint16_t period_ms)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uint32_t timer_clk;
    uint32_t prescaler;
    uint32_t period;

    if (period_ms == 0) return;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    timer_clk = get_apb1_timer_clock();

    /*
     * 计算预分频和周期:
     *   timer_clk / (prescaler + 1) / (period + 1) = 1 / (period_ms / 1000)
     *   => (prescaler + 1) * (period + 1) = timer_clk * period_ms / 1000
     */
    {
        uint32_t total_ticks = timer_clk / 1000U * (uint32_t)period_ms;

        /* 先尝试无预分频 */
        if (total_ticks <= 0xFFFFU)
        {
            prescaler = 0U;
            period    = total_ticks - 1U;
        }
        else
        {
            /* 需要预分频: 将总tick数降到16位以内 */
            prescaler = (uint32_t)(total_ticks / 0xFFFFU);
            period    = (uint32_t)(total_ticks / (prescaler + 1U)) - 1U;

            if (period > 0xFFFFU)
            {
                period = 0xFFFFU;
            }
        }
    }

    TIM_DeInit(TIM4);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler     = (uint16_t)prescaler;
    TIM_TimeBaseStructure.TIM_Period        = (uint16_t)period;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    /* 使能更新中断 */
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    /* 最高优先级: PreemptPriority=0, SubPriority=0 */
    NVIC_InitStructure.NVIC_IRQChannel    = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* ================================================================
 * 公开接口
 * ================================================================ */

/**
 * @brief PID AGC 初始化
 */
void pid_agc_init(float kp, float ki, float kd, float target_rms,
                  uint16_t min_dac, uint16_t max_dac)
{
    g_kp         = kp;
    g_ki         = ki;
    g_kd         = kd;
    g_target_rms = target_rms;
    g_min_dac    = min_dac;
    g_max_dac    = max_dac;

    pid_agc_reset();
}

/**
 * @brief 启动 PID 闭环定时器
 */
void pid_agc_start(uint16_t period_ms)
{
    g_pid_period_ms = (period_ms > 0) ? period_ms : 10;

    pid_agc_reset();
    tim4_init(g_pid_period_ms);
    TIM_Cmd(TIM4, ENABLE);
}

/**
 * @brief 停止 PID 闭环定时器
 */
void pid_agc_stop(void)
{
    TIM_Cmd(TIM4, DISABLE);
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
}

/**
 * @brief 核心 PID 计算: ADC缓冲 -> DAC幅度
 * @note  纯函数, 无副作用, 可在 ISR 或主循环中调用
 */
uint16_t pid_agc_compute(uint16_t *adc_buf, uint16_t len)
{
    float measured_rms;
    float error;
    float derivative;
    float output;
    float new_amp_f;
    uint16_t new_amp;
    float dt;

    if (adc_buf == NULL || len == 0)
    {
        return g_sys.dac_amplitude_mv;
    }

    /* ---- Step 1: 计算 AC RMS ---- */
    measured_rms = compute_ac_rms(adc_buf, len);

    /* 无信号保护: 冻结积分器 */
    if (measured_rms < 0.001f)
    {
        return g_sys.dac_amplitude_mv;
    }

    /* ---- Step 2: PID 计算 ---- */
    dt    = (float)g_pid_period_ms / 1000.0f;   /* 周期(秒) */
    error = g_target_rms - measured_rms;

    /* P: 比例项 */
    output = g_kp * error;

    /* I: 积分项 (抗饱和) */
    g_integral += error * dt;
    if (g_integral > INTEGRAL_MAX)
    {
        g_integral = INTEGRAL_MAX;
    }
    else if (g_integral < -INTEGRAL_MAX)
    {
        g_integral = -INTEGRAL_MAX;
    }
    output += g_ki * g_integral;

    /* D: 微分项 (带低通特性的差分) */
    if (dt > 0.0f)
    {
        derivative = (error - g_prev_error) / dt;
        output += g_kd * derivative;
    }
    g_prev_error = error;

    /* ---- Step 3: 计算新 DAC 幅度 ---- */
    new_amp_f = (float)g_sys.dac_amplitude_mv + output;

    /* 钳位 */
    if (new_amp_f < (float)g_min_dac)
    {
        new_amp = g_min_dac;
    }
    else if (new_amp_f > (float)g_max_dac)
    {
        new_amp = g_max_dac;
    }
    else
    {
        new_amp = (uint16_t)(new_amp_f + 0.5f);
    }

    /* ---- Step 4: 锁定检测 ---- */
    {
        float abs_error = (error >= 0.0f) ? error : -error;
        if (abs_error < g_target_rms * 0.01f)
        {
            if (g_lock_counter < LOCK_COUNT_MAX)
            {
                g_lock_counter++;
            }
            if (g_lock_counter >= LOCK_COUNT_MAX)
            {
                g_locked = 1;
            }
        }
        else
        {
            g_lock_counter = 0;
            g_locked = 0;
        }
    }

    return new_amp;
}

/**
 * @brief 复位 PID 状态
 */
void pid_agc_reset(void)
{
    g_integral     = 0.0f;
    g_prev_error   = 0.0f;
    g_lock_counter = 0;
    g_locked       = 0;
}

/**
 * @brief 查询锁定状态
 */
uint8_t pid_agc_is_locked(void)
{
    return g_locked;
}

/* ================================================================
 * TIM4 中断服务例程 (最高优先级)
 *
 * 流程:
 *   1. 读取 Uo 通道 ADC 缓冲
 *   2. pid_agc_compute() -> 新 DAC 幅度
 *   3. 若变化超过死区, 更新 DAC 硬件
 *   4. 更新 g_sys 状态
 * ================================================================ */

void TIM4_IRQHandler(void)
{
    uint16_t *adc_buf;
    uint16_t new_amp;
    int32_t delta;

    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

        /* 仅在 AGC 模式下运行 */
        if (g_sys.amp_mode != MODE_AGC)
        {
            return;
        }

        /* 选择 Uo 通道缓冲 */
        adc_buf = (g_sys.uo_ch == 0) ? ttf_adc_ch1_buf :
                  (g_sys.uo_ch == 1) ? ttf_adc_ch2_buf : ttf_adc_ch3_buf;

        /* PID 计算: ADC -> DAC */
        new_amp = pid_agc_compute(adc_buf, ADC_SAMPLES);

        /* 死区检查: 变化 < 2mV 不更新硬件, 减少 DAC 输出中断 */
        delta = (int32_t)new_amp - (int32_t)g_sys.dac_amplitude_mv;
        if (delta < 0) delta = -delta;

        if (delta > DAC_DEADBAND_MV)
        {
            g_sys.dac_amplitude_mv = new_amp;
            g_sys.agc_locked       = g_locked;
            dac_app_set_amplitude(new_amp);
        }
        else
        {
            /* 更新锁定状态 (即使幅度不变) */
            g_sys.agc_locked = g_locked;
        }
    }
}
