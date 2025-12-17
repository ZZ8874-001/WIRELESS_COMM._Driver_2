#include "bsp_adc.h"

#include "wirelessrx.h"
#include "filter32.h"

First_Order_Filter_t VInFilter;
First_Order_Filter_t VCCFilter;

uint32_t ADC_values[ADC_DataSize];
float V_values[ADC_DataSize];

float VIn_f;
float VCC_f;

void Bsp_ADC_Init()
{
    while(HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED) != HAL_OK)
    {
    }
    while(HAL_ADC_Start_DMA(&hadc1,ADC_values,ADC_DataSize) != HAL_OK)
    {
    }

    First_Order_Filter_Init(&VInFilter,1/97500.0f,30.0f);
    First_Order_Filter_Init(&VCCFilter,1/97500.0f,30.0f);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc == &hadc1)
    {
        V_values[ADCVIN] = ADC_values[ADCVIN] * ADCRatio * ADCVoltageRatio;
        V_values[ADCVCC] = ADC_values[ADCVCC] * ADCRatio * ADCVoltageRatio;

        VIn_f = First_Order_Filter_Calculate(&VInFilter,V_values[ADCVIN]);
        VCC_f = First_Order_Filter_Calculate(&VCCFilter,V_values[ADCVCC]);

        // VIn_f = 20.0f;
        if(RxStatus != RxStatus_Debug)
        {
            // if(VIn_f > 19.0f)
            // {
            //     // RxStatus = RxStatus_Connecting;
            //     RxStatus = RxStatus_Connected;
            // }
            // else if(VIn_f < 17.0f)
            // {
            //     RxStatus = RxStatus_Disconnected;
            // }
        }
    }
}