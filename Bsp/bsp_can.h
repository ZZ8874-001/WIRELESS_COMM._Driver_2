#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

#include "main.h"

typedef struct
{
    // 1.电容采样的无线充电流，占两个uint8
    // 2.回家信号，准备充电
    // 3.充电完成信号
    // 4.复位信号，将无线充从关断保护状态切换到待命状态
    float Cap_Voltage_REM;
    uint8_t backhome_flag;
    uint8_t charge_complete_flag;
    uint8_t reset_flag;

}BIGCUP_DATA_RX_T;

typedef struct
{
    // 1.开始充电信号
    // 2.关断信号
    // 3.复位成功信号
    // 4.无线充错误状态信号
    uint8_t start_charge_flag;
    uint8_t shutdown_flag;
    uint8_t reset_success_flag;
    uint8_t error_flag;    

}BIGCUP_DATA_TX_T;

extern BIGCUP_DATA_TX_T bigcup_data_tx;
extern BIGCUP_DATA_RX_T bigcup_data_rx;
extern BIGCUP_DATA_RX_T miao_data_rx;

void Bsp_CAN_Init();
void Send_Bigcup_Data();

#endif