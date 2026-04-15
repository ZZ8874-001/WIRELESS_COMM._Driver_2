#include "bsp_usart.h"

#include "detect_task.h"

// 0xBB为关断使能，0xAA为使能
USART_Tx_Buf_t Tx_Buf = {
    .Head = 0xBB,
    .Data = 0x00,
};

uint8_t Rx_Buf;

void Bsp_USART_Init(void)
{
    // channel2为TX,channel3为RX
    DMA1_Channel2->CCR = 0;
    DMA1_Channel3->CCR = 0;
    USART3->CR1 = 0;

    DMA1_Channel2->CCR |= DMA_CCR_MINC|DMA_CCR_DIR;
    DMA1_Channel2->CNDTR = sizeof(USART_Tx_Buf_t);
    DMA1_Channel2->CPAR = (uint32_t)&USART3->TDR;
    DMA1_Channel2->CMAR = (uint32_t)&Tx_Buf;

    DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CNDTR = sizeof(Rx_Buf)/sizeof(uint8_t);
    DMA1_Channel3->CPAR = (uint32_t)&USART3->RDR;
    DMA1_Channel3->CMAR = (uint32_t)&Rx_Buf;

    USART3->CR3 = 0;
    USART3->CR3 |= USART_CR3_DMAT | USART_CR3_DMAR;

    USART3->ICR |= USART_ICR_TCCF | USART_ICR_IDLECF;

    DMA1_Channel2->CCR |= DMA_CCR_EN;
    USART3->CR1 |= USART_CR1_TCIE | USART_CR1_IDLEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

}

void USER_USART_InterruptCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        if(USART3->ISR & USART_ISR_TC)
        {
            DMA1_Channel2->CCR &= ~DMA_CCR_EN;
            USART3->CR1 &= ~USART_CR1_TE;

            DMA1_Channel2->CNDTR = sizeof(USART_Tx_Buf_t);

            USART3->ICR |= USART_ICR_TCCF;

            Detect_Hook(USART3_TX_TOE);
            
            // DMA1_Channel2->CCR |= DMA_CCR_EN;
            // USART3->CR1 |= USART_CR1_TE;
        }
        else if(USART3->ISR & USART_ISR_IDLE)
        {
            USART3->ICR |= USART_ICR_IDLECF;

            DMA1_Channel3->CCR &= ~DMA_CCR_EN;
            DMA1_Channel3->CNDTR = sizeof(Rx_Buf)/sizeof(uint8_t);
            DMA1_Channel3->CCR |= DMA_CCR_EN;

            Detect_Hook(USART3_RX_TOE);
        }
    }
}