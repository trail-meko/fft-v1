#include "key_app.h"
#include "time.h"
uint8_t key_down,key_val,key_up,key_old;
uint8_t oled_mode;


//按键初始化函数
void key_init() //IO初始化
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG|RCC_APB2Periph_GPIOC, ENABLE);//使能时钟

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8;//KEY0-KEY1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化GPIOG

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_7;//
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIOC
}

// 检查
uint8_t check(uint16_t year, uint8_t month)
{
    struct tm tm_info = {0};
    
    tm_info.tm_year = year - 1900;
    tm_info.tm_mon  = month;     
    tm_info.tm_mday = 0;          
    
    mktime(&tm_info);
    
    return tm_info.tm_mday;  
}

uint8_t key_read()
{
    uint8_t temp = 0;
		if(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_5)==0)temp=1;
		if(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_6)==0)temp=2;
		if(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8)==0)temp=3;
		if(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_7)==0)temp=4;
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)==0)temp=5;		
    return temp;
	
}

void key_proc()
{
    key_val = key_read();
    key_down = key_val & (key_val ^ key_old);
    key_up = ~key_val & (key_val ^ key_old);
    key_old = key_val;		
	
		switch ( key_down )
		{
			case 1:
						if(++oled_mode%3==0)oled_mode=0;
			break;
		}

}



