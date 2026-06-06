//#include "sys.h"
//#include "usart_app.h"

//uint8_t uart_dma_buffer[128] = {0};
//uint8_t uart_rx_dma_buffer[128] = {0};
//uint8_t uart_flag = 0;

//int my_printf(USART_TypeDef *USARTx, const char *format, ...)
//{
//    char buffer[128];
//    va_list arg;
//    int len;
//    va_start(arg, format);
//    len = vsnprintf(buffer, sizeof(buffer), format, arg);
//    va_end(arg);

//    for (int i = 0; i < len; i++)
//    {
//        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
//        USART_SendData(USARTx, (uint8_t)buffer[i]);
//    }
//    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
//    return len;
//}

///**
// * @brief USART1中断处理函数，处理空闲中断+DMA接收
// */
//void USART1_IRQHandler(void)
//{
//    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
//    {
//        // 清除空闲中断标志（读SR再读DR）
//        volatile uint32_t tmp = USART1->SR;
//        tmp = USART1->DR;
//        (void)tmp;

//        // 停止DMA
//        DMA_Cmd(DMA1_Channel5, DISABLE);  // USART1_RX 对应 DMA1_Channel5

//        // 计算实际接收长度
//        uint16_t size = sizeof(uart_rx_dma_buffer) - DMA_GetCurrDataCounter(DMA1_Channel5);

//        // 复制数据
//        memcpy(uart_dma_buffer, uart_rx_dma_buffer, size);

//        // 举旗
//        uart_flag = 1;

//        // 清空DMA缓冲区，重新启动DMA
//        memset(uart_rx_dma_buffer, 0, sizeof(uart_rx_dma_buffer));
//        DMA_SetCurrDataCounter(DMA1_Channel5, sizeof(uart_rx_dma_buffer));
//        DMA_Cmd(DMA1_Channel5, ENABLE);
//    }
//}








//void uart_task(void)
//{
//    if (uart_flag == 0)
//        return;

//    uart_flag = 0;
//    my_printf(USART1, "data: %s\n", uart_dma_buffer);
//    memset(uart_dma_buffer, 0, sizeof(uart_dma_buffer));
//}


//void USART1_Init(uint32_t baudrate)
//{
//    USART_InitTypeDef USART_InitStructure;
//    GPIO_InitTypeDef GPIO_InitStructure;
//    DMA_InitTypeDef DMA_InitStructure;
//    NVIC_InitTypeDef NVIC_InitStructure;

//    // 时钟使能
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
//    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

//    // TX: PA9 复用推挽输出
//    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
//    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);

//    // RX: PA10 浮空输入
//    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);

//    // USART1配置
//    USART_InitStructure.USART_BaudRate            = baudrate;
//    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
//    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
//    USART_InitStructure.USART_Parity              = USART_Parity_No;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
//    USART_Init(USART1, &USART_InitStructure);

//    // DMA1_Channel5 接收配置
//    DMA_DeInit(DMA1_Channel5);
//    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
//    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)uart_rx_dma_buffer;
//    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
//    DMA_InitStructure.DMA_BufferSize         = sizeof(uart_rx_dma_buffer);
//    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
//    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
//    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
//    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
//    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
//    DMA_InitStructure.DMA_M2M               = DMA_M2M_Disable;
//    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
//    DMA_Cmd(DMA1_Channel5, ENABLE);

//    // 使能USART DMA接收请求 + 空闲中断
//    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
//    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

//    // NVIC配置
//    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

//    USART_Cmd(USART1, ENABLE);
//}


