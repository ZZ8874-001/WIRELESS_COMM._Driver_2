#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#include "adc.h"

#define ADC_DataSize 2
#define ADCVIN 0
#define ADCVCC 1
#define ADCRatio 3.3f/4096.0f
#define ADCVoltageRatio 12.528f

void Bsp_ADC_Init();
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);

#endif