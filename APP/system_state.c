#include "system_state.h"
#include "dac_app.h"
#include "pid_controller.h"

/* 全局系统状态实例 */
system_state_t g_sys;

/**
 * @brief 系统状态初始化
 * @note  必须在 dac_app_init() 之前调用
 */
void system_state_init(void)
{
    g_sys.waveform           = WAVEFORM_SINE;
    g_sys.freq_hz            = 1000;
    g_sys.amp_mode           = MODE_MANUAL;
    g_sys.dac_amplitude_mv   = 141;          /* 141mV peak ≈ 100mV RMS (sine) */
    g_sys.agc_target_rms     = 1.5f;
    g_sys.agc_locked         = 0;
    g_sys.ui_rms             = 0.0f;
    g_sys.uo_rms             = 0.0f;
    g_sys.ui_freq            = 0.0f;
    g_sys.uo_freq            = 0.0f;
    g_sys.gain               = 0.0f;
    g_sys.phase_diff_deg     = 0.0f;
    g_sys.ui_ch              = 0;            /* ch1 -> PA5 */
    g_sys.uo_ch              = 1;            /* ch2 -> PA6 */
}

/**
 * @brief 频率循环切换：1000Hz -> 1100Hz -> ... -> 2000Hz -> 1000Hz
 * @note  步进100Hz，调用 dac_app_set_frequency() 立即生效
 */
void sys_cycle_frequency(void)
{
    g_sys.freq_hz += 100;
    if (g_sys.freq_hz > 2000)
    {
        g_sys.freq_hz = 1000;
    }
    dac_app_set_frequency(g_sys.freq_hz);
}

/**
 * @brief 幅度模式切换：手动 <-> 自动(AGC)
 * @note  切入AGC时复位PI积分器防止冲击
 */
void sys_toggle_amp_mode(void)
{
    if (g_sys.amp_mode == MODE_MANUAL)
    {
        g_sys.amp_mode = MODE_AGC;
        pid_agc_reset();
        /* 保守起步：从50mV峰值开始让PI缓慢拉升 */
        g_sys.dac_amplitude_mv = 50;
        dac_app_set_amplitude(50);
    }
    else
    {
        g_sys.amp_mode = MODE_MANUAL;
        /* 恢复手动模式的100mV RMS ≈ 141mV peak */
        g_sys.dac_amplitude_mv = 141;
        dac_app_set_amplitude(141);
    }
}

/**
 * @brief 波形切换：正弦波 <-> 三角波
 * @note  调用 dac_app_set_waveform() 重建波形缓冲
 */
void sys_toggle_waveform(void)
{
    if (g_sys.waveform == WAVEFORM_SINE)
    {
        g_sys.waveform = WAVEFORM_TRIANGLE;
    }
    else
    {
        g_sys.waveform = WAVEFORM_SINE;
    }
    dac_app_set_waveform(g_sys.waveform);
}
