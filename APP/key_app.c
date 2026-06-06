#include "key_app.h"

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
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // 输入模式下其实无效，写上也可以
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // 输入模式下影响不大
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // 上拉
    GPIO_Init(GPIOE, &GPIO_InitStructure);
}

/**
 * @brief 读取当前按键值
 * @return 0:无按键 1~5:对应按键
 *
 * 说明：
 * 这里使用“低电平表示按下”
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
 * @brief 按键处理函数
 * 建议每10ms调用一次
 */
void key_proc(void)
{
    key_val  = key_read();
    key_down = key_val & (key_val ^ key_old);
    key_up   = ~key_val & (key_val ^ key_old);
    key_old  = key_val;

    switch(key_down)
    {
        case 1:
            oled_mode++;
            if(oled_mode >= 3)
                oled_mode = 0;
            break;

        case 2:
            /* 这里写按键2按下后的处理 */
            break;

        case 3:
            /* 这里写按键3按下后的处理 */
            break;

        case 4:
            /* 这里写按键4按下后的处理 */
            break;

        case 5:
            /* 这里写按键5按下后的处理 */
            break;

        default:
            break;
    }
}


