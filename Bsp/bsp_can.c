#include "bsp_can.h"

#include "main.h"
#include "can.h"

#include "detect_task.h"
#include "wirelessrx.h"
#include "bsp_adc.h"

#include <string.h>

#define BIGCUP_CAN_RX_ID 0x210
#define BIGCUP_CAN_TX_ID 0x209
#define MIAO_CAN_RX_ID 0x170

BIGCUP_DATA_TX_T bigcup_data_tx;
BIGCUP_DATA_RX_T bigcup_data_rx;
BIGCUP_DATA_RX_T miao_data_rx;

static void Bigcup_Data_Update();

void Bsp_CAN_Init()
{
    CAN_FilterTypeDef can_filter;
    can_filter.FilterIdHigh = BIGCUP_CAN_RX_ID << 5;
    can_filter.FilterIdLow = 0x0000;
    can_filter.FilterMaskIdHigh = MIAO_CAN_RX_ID << 5;
    can_filter.FilterMaskIdLow = 0x0000;
    can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter.FilterBank = 0;
    can_filter.FilterMode = CAN_FILTERMODE_IDLIST;
    can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter.FilterActivation = ENABLE;
    can_filter.SlaveStartFilterBank = 14;

    while(HAL_CAN_ConfigFilter(&hcan, &can_filter) != HAL_OK)
    {
    }
    while(HAL_CAN_Start(&hcan) != HAL_OK)
    {
    }
    while(HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
    {
    }

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
        case BIGCUP_CAN_RX_ID:
            bigcup_data_rx.Cap_Voltage_REM = rx_data[0] / 5.0f;
            bigcup_data_rx.backhome_flag = rx_data[3];
            bigcup_data_rx.charge_complete_flag = rx_data[4];
            bigcup_data_rx.reset_flag = rx_data[5];
            
            Detect_Hook(CAN_BIGCUP_RX_TOE);
            break;
        case MIAO_CAN_RX_ID:
            miao_data_rx.backhome_flag = rx_data[4];
            miao_data_rx.reset_flag = rx_data[5];

            Detect_Hook(CAN_MIAO_RX_TOE);
            break;
        default:
            break;
        }
    }
}

static void Bigcup_Data_Update()
{
    bigcup_data_tx.start_charge_flag = 0;
    bigcup_data_tx.shutdown_flag = 0;
    bigcup_data_tx.reset_success_flag = 0;
    bigcup_data_tx.error_flag = 0;
    switch(RxStatus)
    {
    case RxStatus_Connecting:
        break;
    case RxStatus_Connected:
        bigcup_data_tx.start_charge_flag = 1;
        break;
    case RxStatus_CurrentError:
        bigcup_data_tx.error_flag = 1;
        break;
    case RxStatus_Disconnected:
        bigcup_data_tx.reset_success_flag = 1;
        break;
    default:
        bigcup_data_tx.error_flag = 1;
        break;
    }
}

HAL_StatusTypeDef Send_Bigcup_Data()
{
    static uint8_t can_health = 0;
    static uint8_t vout_integer = 0;
    static uint16_t vout_decimal = 0;
    static uint8_t vout_decimal1 = 0;
    static uint8_t vout_decimal2 = 0;
    if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)
    {
        can_health = 0;
        return;
    }
    else
    {
        can_health = 1;
    }

    uint8_t tx_data[8];
    uint32_t tx_mailbox;

    Bigcup_Data_Update();
    vout_integer = (uint8_t)(VOUT_f);
    vout_decimal = (uint16_t)((VOUT_f - (float)vout_integer)*10000);
    vout_decimal1 = (uint8_t)(vout_decimal/100);
    vout_decimal2 = (uint8_t)(vout_decimal%100);

    tx_data[0] = bigcup_data_tx.start_charge_flag;
    tx_data[1] = bigcup_data_tx.shutdown_flag;
    tx_data[2] = bigcup_data_tx.reset_success_flag;
    tx_data[3] = bigcup_data_tx.error_flag;
    tx_data[4] = vout_integer;
    tx_data[5] = vout_decimal1;
    tx_data[6] = vout_decimal2;
    tx_data[7] = (uint8_t)(RxStatus);

    return HAL_CAN_AddTxMessage(&hcan, &(CAN_TxHeaderTypeDef){.StdId = BIGCUP_CAN_TX_ID, .ExtId = 0, .RTR = CAN_RTR_DATA, .IDE = CAN_ID_STD, .DLC = 8}, tx_data, &tx_mailbox);
}

