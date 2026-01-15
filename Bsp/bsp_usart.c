#include "bsp_usart.h"

// 0xBB为关断使能，0xAA为使能
USART_Tx_Buf_t Tx_Buf = {
    .Head = 0xBB,
    .Data = 0x00,
};

void Bsp_USART_Init(void)
{
    DMA1_Channel2->CCR = 0;
    USART3->CR1 = 0;

    DMA1_Channel2->CCR |= DMA_CCR_MINC|DMA_CCR_DIR;
    DMA1_Channel2->CNDTR = sizeof(USART_Tx_Buf_t);
    DMA1_Channel2->CPAR = (uint32_t)&USART3->TDR;
    DMA1_Channel2->CMAR = (uint32_t)&Tx_Buf;
    
    USART3->CR3 = 0;
    USART3->CR3 |= USART_CR3_DMAT;

    USART3->ICR |= USART_ICR_TCCF;

    DMA1_Channel2->CCR |= DMA_CCR_EN;
    USART3->CR1 |= USART_CR1_TE | USART_CR1_TCIE | USART_CR1_UE;

}

void USER_USART_InterruptCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        if(USART3->ISR & USART_ISR_TC)
        {
            DMA1_Channel2->CCR &= ~DMA_CCR_EN;
            DMA1_Channel2->CNDTR = sizeof(USART_Tx_Buf_t);

            USART3->ICR |= USART_ICR_TCCF;
            DMA1_Channel2->CCR |= DMA_CCR_EN;
        }
    }
}