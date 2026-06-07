#ifndef __SYSTEM_STATE_H
#define __SYSTEM_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dac_app.h"
#include <stdint.h>

/**
 * @brief 幅度控制模式枚举
 */
typedef enum
{
    MODE_MANUAL = 0,   /* 手动模式：固定100mV RMS输出 */
    MODE_AGC    = 1    /* 自动模式：PI闭环跟踪Uo=1.5V RMS */
} amp_mode_t;

/**
 * @brief 全局系统状态结构体
 * @note  集中管理DAC控制参数与ADC测量结果，所有模块通过 g_sys 访问
 */
typedef struct
{
    /* ---- DAC 控制参数 ---- */
    dac_waveform_t  waveform;           /* 当前波形类型：SINE / TRIANGLE */
    uint32_t        freq_hz;            /* 当前频率：1000 ~ 2000 Hz, step 100 */
    amp_mode_t      amp_mode;           /* 幅度控制模式 */
    uint16_t        dac_amplitude_mv;   /* 当前DAC峰值幅度 (mV) */

    /* ---- AGC 参数 ---- */
    float           agc_target_rms;     /* AGC目标有效值：1.5V */
    uint8_t         agc_locked;         /* AGC锁定标志：连续5次误差<1% */

    /* ---- ADC 测量结果 (AC RMS, 单位: V) ---- */
    float           ui_rms;             /* Ui 交流有效值 */
    float           uo_rms;             /* Uo 交流有效值 */
    float           ui_freq;            /* Ui 测量频率 */
    float           uo_freq;            /* Uo 测量频率 */
    float           gain;               /* 放大倍数 = Uo / Ui */
    float           phase_diff_deg;     /* 相位差 (Uo - Ui), 单位: 度 */

    /* ---- 通道分配 ---- */
    uint8_t         ui_ch;              /* Ui 对应ADC通道索引 (0=ch1/PA5) */
    uint8_t         uo_ch;              /* Uo 对应ADC通道索引 (1=ch2/PA6) */
} system_state_t;

/* 全局系统状态实例 */
extern system_state_t g_sys;

/* ---- 辅助函数 ---- */
void system_state_init(void);
void sys_cycle_frequency(void);
void sys_toggle_amp_mode(void);
void sys_toggle_waveform(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_STATE_H */
