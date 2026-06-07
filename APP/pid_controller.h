#ifndef __PID_CONTROLLER_H
#define __PID_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief PID AGC 控制器初始化
 * @param kp            比例系数
 * @param ki            积分系数
 * @param kd            微分系数
 * @param target_rms    目标有效值 (V)
 * @param min_dac       DAC峰值幅度下限 (mV)
 * @param max_dac       DAC峰值幅度上限 (mV)
 */
void pid_agc_init(float kp, float ki, float kd, float target_rms,
                  uint16_t min_dac, uint16_t max_dac);

/**
 * @brief 启动 PID 闭环定时器 (TIM4, 最高优先级)
 * @param period_ms   PID 计算周期 (ms), 建议 10~20ms
 * @note  启动后 TIM4 ISR 自动读取 Uo ADC 数据 -> 算RMS -> PID -> 更新DAC
 */
void pid_agc_start(uint16_t period_ms);

/**
 * @brief 停止 PID 闭环定时器
 */
void pid_agc_stop(void);

/**
 * @brief 核心计算: 传入 ADC 缓冲 -> 返回目标 DAC 幅度
 * @param  adc_buf  ADC原始数据缓冲 (0-4095)
 * @param  len      缓冲长度
 * @return          目标 DAC 峰值幅度 (mV), 已钳位
 * @note   可在手动模式或测试时直接调用, 不依赖定时器
 */
uint16_t pid_agc_compute(uint16_t *adc_buf, uint16_t len);

/**
 * @brief 复位 PID 状态 (积分清零, 微分清零, 解除锁定)
 */
void pid_agc_reset(void);

/**
 * @brief 查询 PID 是否已锁定到目标
 * @return 1=已锁定 (连续5次误差<1%), 0=未锁定
 */
uint8_t pid_agc_is_locked(void);

#ifdef __cplusplus
}
#endif

#endif /* __PID_CONTROLLER_H */
