#ifndef __UART_H
#define __UART_H	 
#include "mydefine.h"

void USART1_Init(uint32_t baudrate);
void uart_task(void);
int my_printf(USART_TypeDef *USARTx, const char *format, ...);

#endif


