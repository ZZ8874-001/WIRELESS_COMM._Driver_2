#include "bsp_adc.h"

#include "wirelessrx.h"
#include "detect_task.h"
#include "filter32.h"

#define ADC_SAMPLING_FREQUENCY (12e6/74.0f)
#define ADCRatio 3.3f/4096.0f
#define ADCVoltageRatio 12.528f

First_Order_Filter_t VInFilter;
First_Order_Filter_t VCCFilter;

First_Order_Filter_t CurrentFilter;

uint16_t ADC1_values[2];
uint16_t ADC2_values[1];

float VIn_f;
float VCC_f;
float Current_f;

void Bsp_ADC_Init()
{
    while(HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED) != HAL_OK);
    while(HAL_ADCEx_Calibration_Start(&hadc2,ADC_DIFFERENTIAL_ENDED) != HAL_OK);
    while(HAL_ADC_Start_DMA(&hadc1,ADC1_values,sizeof(ADC1_values)/sizeof(ADC1_values[0])) != HAL_OK);
    while(HAL_ADC_Start_DMA(&hadc2,ADC2_values,sizeof(ADC2_values)/sizeof(ADC2_values[0])) != HAL_OK);
    
    ADC1->IER |= 0x100;
    ADC1->TR1 = 0 << 16 | 0;
    ADC1->TR2 = 0 << 16  | 0;
    ADC1->AWD2CR = 1 << 3;

    DMA1_Channel1->CCR &= 0xFFFB;
    DMA1_Channel2->CCR &= 0xFFFB;


    First_Order_Filter_Init(&VInFilter,1/ADC_SAMPLING_FREQUENCY,30.0f);
    First_Order_Filter_Init(&VCCFilter,1/ADC_SAMPLING_FREQUENCY,30.0f);
    First_Order_Filter_Init(&CurrentFilter,1/ADC_SAMPLING_FREQUENCY,30.0f);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        float V_values[2];
        V_values[ADCVIN] = ADC1_values[ADCVIN] * ADCRatio * ADCVoltageRatio;
        V_values[ADCVCC] = ADC1_values[ADCVCC] * ADCRatio * ADCVoltageRatio;

        VIn_f = First_Order_Filter_Calculate(&VInFilter,V_values[ADCVIN]);
        VCC_f = First_Order_Filter_Calculate(&VCCFilter,V_values[ADCVCC]);

    }
    else if(hadc->Instance == ADC2)
    {
        float Current =(2048 - ADC2_values[0]) * ADCRatio * ADCVoltageRatio;

        Current_f = First_Order_Filter_Calculate(&CurrentFilter,Current);
    }
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        static uint16_t adcwatch1 = 0;
        adcwatch1++;

        Detect_Hook(ADC1_WATCHDOG1_TOE);
        RxStatus = RxStatus_Disconnected;
    }
}

void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        static uint16_t adcwatch2 = 0;
        adcwatch2++;
        
        Detect_Hook(ADC1_WATCHDOG2_TOE);
        RxStatus = RxStatus_Disconnected;
    }
}