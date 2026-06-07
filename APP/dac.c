#include "mydefine.h"
#include "dac.h"
#include "math.h"
#include "mydefine.h"

#include "math.h"

//#define DAC_MAX 4095      // 12位DAC最大值
//#define DAC_MIN 0
//#define VREF 3.3f         // STM32F407 DAC参考电压

//// DAC初始化，使用PB0 (DAC_OUT1) 或 PB1 (DAC_OUT2)
//void dac_init(void)
//{
//    GPIO_InitTypeDef GPIO_InitStruct;
//    DAC_InitTypeDef DAC_InitStruct;

//    // 使能时钟
//    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

//    // 配置PB0为模拟输出
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
//    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
//    GPIO_Init(GPIOB, &GPIO_InitStruct);

//    // DAC 配置
//    DAC_InitStruct.DAC_Trigger = DAC_Trigger_None;
//    DAC_InitStruct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
//    DAC_Init(DAC_Channel_1, &DAC_InitStruct);
//    DAC_Cmd(DAC_Channel_1, ENABLE);

//    // 默认输出0V
//    DAC_SetChannel1Data(DAC_Align_12b_R, 0);
//}

//// 设置DAC原始数值，自动限幅
//void set_dac_val(uint16_t val)
//{
//    if(val > DAC_MAX) val = DAC_MAX;
//    if(val < DAC_MIN) val = DAC_MIN;
//    DAC_SetChannel1Data(DAC_Align_12b_R, val);
//}

//// 设置目标电压输出，自动转换成DAC数值
//void set_voltage_val(float voltage)
//{
//    if(voltage > VREF) voltage = VREF;
//    if(voltage < 0.0f) voltage = 0.0f;

//    uint16_t dac_val = (uint16_t)((voltage / VREF) * DAC_MAX);
//    set_dac_val(dac_val);
//}
