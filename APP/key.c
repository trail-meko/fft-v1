#include "mydefine.h"
#include "lcd.h"
#include "key.h"
uint8_t Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量



void Key_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 |  GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);	

}

extern uint8_t oled_mode;
uint16_t temp;
unsigned char Key_Read(void)
{
    unsigned char temp = 0;

	if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4)==0)temp=1;
	if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3)==0)temp=2;
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)==1)temp=3;
    return temp;
}


void key_proc(void)
{

	Key_Val = Key_Read(); //实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val); //捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val); //捕捉按键上降沿
	Key_Old = Key_Val; //辅助扫描变量

	
    if (Key_Down != 0) 
     {
		OLED_Clear();
		
        if (Key_Down == 1)      // KEY1：增加步数
        {

        }
        else if (Key_Down == 2) // KEY2：减少步数
        {

        }
        else if (Key_Down == 3) // KEY3：保存
        {
            
        }
	}

 
	
}







