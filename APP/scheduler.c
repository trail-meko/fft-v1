#include "sys.h"
#include "scheduler.h"


// 全局变量，用于存储任务数量
uint8_t task_num;
//定义时间戳
uint32_t uwTick;

typedef struct {
    void (*task_func)(void);
    uint32_t rate_ms;
    uint32_t last_run;
} task_t;

// 静态任务数组，每个任务包含任务函数、执行周期（毫秒）和上次运行时间（毫秒）
static task_t scheduler_task[] =
{
	//{adc_proc, 10, 0},
	
	{0},


};


void scheduler_init(void)
{
    // 计算任务数组的元素个数，并将结果存储在 task_num 中
    task_num = sizeof(scheduler_task) / sizeof(task_t);
};


void scheduler_run(void)
{
    // 遍历任务数组中的所有任务
    for (uint8_t i = 0; i < task_num; i++)
    {
        // 获取当前的系统时间（毫秒）
        uint32_t now_time =uwTick;

        // 检查当前时间是否达到任务的执行时间
        if (now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_run)
        {
            // 更新任务的上次运行时间为当前时间
            scheduler_task[i].last_run = now_time;

            // 执行任务函数
            scheduler_task[i].task_func();
        }
    }
}


void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能TIM2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 定时器基础配置
    TIM_TimeBaseStructure.TIM_Period        = 1000 - 1;  // 自动重装值 
    TIM_TimeBaseStructure.TIM_Prescaler     = 72 - 1;    // 预分频
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 使能更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // NVIC配置
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 启动定时器
    TIM_Cmd(TIM2, ENABLE);
}

void TIM3_Init(uint32_t frequency)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    // 1. 使能时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    

    uint16_t period = (uint16_t)(72000000 / frequency - 1);

    // 3. 定时器基础配置
    TIM_TimeBaseStructure.TIM_Period        = period;     // 自动重装值
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;          // 不分频，精度最高
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 4. 设置触发输出 (TRGO) 供 ADC 使用
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    
    // 5. 开启定时器
    TIM_Cmd(TIM3, ENABLE);


}


void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  // 清除中断标志
		uwTick++;				
    }
}


