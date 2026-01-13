#ifndef __BSP_USART_H__
#define __BSP_USART_H__

#include "main.h"

typedef struct{
    uint8_t Head;
    uint8_t Data;
}USART_Tx_Buf_t;

void Bsp_USART_Init(void);
void USER_USART_InterruptCallback(UART_HandleTypeDef *huart);

#endif