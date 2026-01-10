#include "bsp_adc.h"

#include "wirelessrx.h"
#include "detect_task.h"
#include "filter32.h"

#define ADC_SAMPLING_FREQUENCY (12e6/74.0f)

#define ADC_RATIO (2.9832f/4096.0f)
#define ADC_RATIO_DIFF (ADC_RATIO*2)
#define ADC_VOLTAGE_RATIO 22.227f
#define ADC_CURRENT_RATIO 5.100f
#define CURRENT_OUT_OFFSET -8.396f
#define VOLTAGE_OUT_OFFSET -0.0129f

#define VIN_MIN 14.0f
#define VIN_MAX 48.0f
#define VIN_WATCHDOG_MIN (VIN_MIN/ADC_RATIO/ADC_VOLTAGE_RATIO)
#define VIN_WATCHDOG_MAX (VIN_MAX/ADC_RATIO/ADC_VOLTAGE_RATIO)

#define VOUT_MIN 0.0f
#define VOUT_MAX 28.0f
#define VOUT_WATCHDOG_MIN (VOUT_MIN/ADC_RATIO/ADC_VOLTAGE_RATIO)
#define VOUT_WATCHDOG_MAX (VOUT_MAX/ADC_RATIO/ADC_VOLTAGE_RATIO)

#define CURRENT_MIN 0.0f
#define CURRENT_MAX 6.0f
#define CURRENT_WATCHDOG_MIN (CURRENT_MIN/ADC_RATIO/ADC_VOLTAGE_RATIO)
#define CURRENT_WATCHDOG_MAX (CURRENT_MAX/ADC_RATIO/ADC_VOLTAGE_RATIO)

static void Change_ADC_AWD_Threshold(uint32_t *ADCx_TRx,uint16_t low_threshold,uint16_t high_threshold);

First_Order_Filter_t VInFilter;
First_Order_Filter_t VCCFilter;

First_Order_Filter_t CurrentFilter;

uint16_t ADC1_values[2];
uint16_t ADC2_values[1];

float VIN_f;
float VOUT_f;
float Current_f;

void Bsp_ADC_Init()
{
    First_Order_Filter_Init(&VInFilter,1.0f/ADC_SAMPLING_FREQUENCY,30.0f);
    First_Order_Filter_Init(&VCCFilter,1.0f/ADC_SAMPLING_FREQUENCY,30.0f);
    First_Order_Filter_Init(&CurrentFilter,1.0f/ADC_SAMPLING_FREQUENCY,30.0f);

    while(HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED) != HAL_OK);
    while(HAL_ADCEx_Calibration_Start(&hadc2,ADC_DIFFERENTIAL_ENDED) != HAL_OK);
    while(HAL_ADC_Start_DMA(&hadc1,ADC1_values,sizeof(ADC1_values)/sizeof(ADC1_values[0])) != HAL_OK);
    while(HAL_ADC_Start_DMA(&hadc2,ADC2_values,sizeof(ADC2_values)/sizeof(ADC2_values[0])) != HAL_OK);
    
    Change_ADC_AWD_Threshold(&ADC1->TR1,VIN_WATCHDOG_MIN,VIN_WATCHDOG_MAX);   //  VIN 14-48
    Change_ADC_AWD_Threshold(&ADC1->TR2,VOUT_WATCHDOG_MIN/16,VOUT_WATCHDOG_MAX/16);   //  VOUT  0-28
    Change_ADC_AWD_Threshold(&ADC2->TR1,0,4095);    //  CURRENT
    ADC1->AWD2CR = 1 << 2;
    
    ADC1->IER |= ADC_IER_AWD1 | ADC_IER_AWD2;
    ADC2->IER |= ADC_IER_AWD1;

    DMA1_Channel1->CCR &= 0xFFFB;
    DMA1_Channel2->CCR &= 0xFFFB; 
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        float V_values[2];
        V_values[ADCVIN] = ADC1_values[ADCVIN] * ADC_RATIO * ADC_VOLTAGE_RATIO + VOLTAGE_OUT_OFFSET;
        V_values[ADCVCC] = ADC1_values[ADCVCC] * ADC_RATIO * ADC_VOLTAGE_RATIO + VOLTAGE_OUT_OFFSET;

        VIN_f = First_Order_Filter_Calculate(&VInFilter,V_values[ADCVIN]);
        VOUT_f = First_Order_Filter_Calculate(&VCCFilter,V_values[ADCVCC]);

    }
    else if(hadc->Instance == ADC2)
    {
        float Current =(ADC2_values[0] - 2048) * ADC_RATIO_DIFF * ADC_CURRENT_RATIO + CURRENT_OUT_OFFSET;

        Current_f = First_Order_Filter_Calculate(&CurrentFilter,Current);
    }
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        Detect_Hook(ADC1_WATCHDOG1_TOE);

        last_RxStatus = RxStatus;
        RxStatus = RxStatus_Disconnected;
    }
}

void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {        
        Detect_Hook(ADC1_WATCHDOG2_TOE);

        last_RxStatus = RxStatus;
        RxStatus = RxStatus_Disconnected;
    }
}

static void Change_ADC_AWD_Threshold(uint32_t *ADCx_TRx,uint16_t low_threshold,uint16_t high_threshold)
{
    *ADCx_TRx = (high_threshold << 16) | low_threshold;
}