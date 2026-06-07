#include "key_app.h"
#include "system_state.h"

uint8_t key_down, key_val, key_up, key_old;
uint8_t oled_mode;

/**
 * @brief 按键初始化
 * PE0 ~ PE4
 * 上拉输入，按下为低电平
 */
void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIOE时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        /* 上拉 */
    GPIO_Init(GPIOE, &GPIO_InitStructure);
}

/**
 * @brief 读取当前按键值
 * @return 0:无按键 1~5:对应按键
 *
 * 说明：
 * 这里使用"低电平表示按下"。
 */
uint8_t key_read(void)
{
    uint8_t temp = 0;

    if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_0) == Bit_RESET) temp = 1;
    if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_1) == Bit_RESET) temp = 2;
    if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) == Bit_RESET) temp = 3;
    if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == Bit_RESET) temp = 4;
    if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == Bit_RESET) temp = 5;

    return temp;
}

/**
 * @brief 按键业务处理
 * @note  调度器每10ms调用一次
 *
 * 竞赛按键功能：
 *   KEY1 (PE0): 频率循环 1000->1100->...->2000->1000Hz, step 100Hz
 *   KEY2 (PE1): 幅度模式切换 手动(100mV RMS) / 自动(AGC Uo=1.5V)
 *   KEY3 (PE2): 波形切换 正弦波 <-> 三角波
 *   KEY4 (PE3): 保留
 *   KEY5 (PE4): 保留
 */
void key_proc(void)
{
    key_val  = key_read();
    key_down = key_val & (key_val ^ key_old);
    key_up   = ~key_val & (key_val ^ key_old);
    key_old  = key_val;

    if (key_down != 0)
    {
        switch (key_down)
        {
        case 1:
            sys_cycle_frequency();
            break;

        case 2:
            sys_toggle_amp_mode();
            break;

        case 3:
            sys_toggle_waveform();
            break;

        case 4:
            ui_next_page();
            break;

        case 5:
            /* KEY5 (PE4): 保留 */
            break;

        default:
            break;
        }
    }
}
