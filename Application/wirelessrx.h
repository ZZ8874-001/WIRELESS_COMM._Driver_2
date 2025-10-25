#ifndef WIRELESSRX_H
#define WIRELESSRX_H

#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "bsp_dwt.h"
#include "filter32.h"

#define ADC_DataSize 2
#define ADCVIN 0
#define ADCVCC 1
#define On 1
#define Off 0
#define ADCRatio 3.3f/4096.0f
#define ADCVoltageRatio 12.528f

enum RxStatus_t
{
    RxStatus_Unknow = 0,
    RxStatus_Connecting = 1,
    RxStatus_Disconnected = 2,
    RxStatus_Connected = 3,
};



void WirelessInit();
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void LowPower();
void HighPower();
void SwitchBBEN(int On_Off);
void SwitchENA_ENB(int On_Off);
void SwitchHighOrLowPower(int On_Off);
void IntLimiter(int num,int min,int max);
void QITask();




#endif