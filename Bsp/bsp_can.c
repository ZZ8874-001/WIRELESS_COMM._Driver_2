#include "bsp_can.h"

#include "main.h"
#include "can.h"

void Bsp_CAN_Init()
{
    CAN_FilterTypeDef can_filter;
    can_filter.FilterIdHigh = 0x0000;
    can_filter.FilterIdLow = 0x0000;
    can_filter.FilterMaskIdHigh = 0x0000;
    can_filter.FilterMaskIdLow = 0x0000;
    can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter.FilterBank = 0;
    can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter.FilterActivation = ENABLE;

    while(HAL_CAN_ConfigFilter(&hcan, &can_filter) != HAL_OK);

    while(HAL_CAN_Start(&hcan) != HAL_OK);

    while(HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK);
}

/**
  * @brief  Rx FIFO 0 message pending callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);

    if(hcan->Instance == CAN)
    {
        switch(rx_header.StdId)
        {
        case 0x201:
            break;
            
        default:
            break;
        }
    }
}