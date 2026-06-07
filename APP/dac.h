#ifndef __DAC_H
#define __DAC_H



// DAC初始化
void dac_init(void);

// 直接设置DAC值：0~4095
void set_dac_val(uint16_t val);

// 按电压设置输出：0.0f~3.3f
void set_voltage_val(float voltage);

#endif


