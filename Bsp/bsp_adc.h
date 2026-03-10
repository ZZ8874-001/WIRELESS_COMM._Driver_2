#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#include "adc.h"

#define ADCVIN 0
#define ADCVCC 1

extern float VIN_f;
extern float VOUT_f;
extern float Current_f;

extern uint16_t ADC1_values[2];
extern uint16_t ADC2_values[1];

void Bsp_ADC_Init();
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc);
void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc);

#endif