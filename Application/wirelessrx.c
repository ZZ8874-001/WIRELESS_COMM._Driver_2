#include "wirelessrx.h"

uint32_t ADC_values[ADC_DataSize];
float V_values[ADC_DataSize];
// uint32_t AdcAllValues[ADC_DataSize];
uint32_t DWT_Count;
float VIn_f;
float VCC_f;
uint8_t SOF[8] = {0,1,1,1,1,0,1,0};//0是高功率，1是低功率
uint8_t DOF[8] = {0,0,0,0,0,0,0,0};
First_Order_Filter_t VInFilter;
First_Order_Filter_t VCCFilter;
enum RxStatus_t RxStatus = RxStatus_Unknow;

void WirelessInit()
{
    SwitchBBEN(Off);
    SwitchENA_ENB(Off);
    LowPower();

    DWT_Delay(0.1);
    SwitchBBEN(On);
    SwitchENA_ENB(On);
    if( HAL_ADC_Start_DMA(&hadc1,ADC_values,ADC_DataSize) != HAL_OK  ||
        HAL_ADCEx_Calibration_GetValue(&hadc1,ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }

    First_Order_Filter_Init(&VInFilter,1/97500.0f,30.0f);
    First_Order_Filter_Init(&VCCFilter,1/97500.0f,30.0f);

}

void QITask()
{
    switch(RxStatus)
    {
    case RxStatus_Connecting:
        // 0.5ms
        static uint8_t count = 0;
        static uint8_t data = 0;

        if(count < 8)
        {
            SwitchHighOrLowPower(SOF[count]);
            data =  (uint8_t)VIn_f;
        }
        else if(count < 16)
        {
            DOF[count - 8] = (data >> (15 - count)) & 0x01;
            SwitchHighOrLowPower(DOF[count - 8]);
        }
        else if(count < 200)
        {
            HighPower();
        }
        else
        {
            count = 0;
        }
        count++;
        break;
    case RxStatus_Disconnected:
        HighPower();
        break;
    }
    
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc == &hadc1)
    {
        static float dt;
        dt = DWT_GetDeltaT(&DWT_Count);

        V_values[ADCVIN] = ADC_values[ADCVIN] * ADCRatio * ADCVoltageRatio;
        V_values[ADCVCC] = ADC_values[ADCVCC] * ADCRatio * ADCVoltageRatio;

        VIn_f = First_Order_Filter_Calculate(&VInFilter,V_values[ADCVIN]);
        VCC_f = First_Order_Filter_Calculate(&VCCFilter,V_values[ADCVCC]);

        if(VIn_f > 18.0f && RxStatus != RxStatus_Connected)
        {
            RxStatus = RxStatus_Connecting;
        }
        else
        {
            RxStatus = RxStatus_Disconnected;
        };
    }
    
    // static int time = 0;        //ADC采样次数
    // static int framestime = 0;  //帧次数
    // static int NumOfSOF = 0;    //帧头次数
    // static uint32_t ceshiman = 0;

    // if(ceshiman >= 50)
    // {
    //     SwitchBBEN(Off);
    // }
    // if(hadc->Instance == ADC1)
    // {
    //     time++;
    //     AdcAllValues[ADCCurrentIn] += ADC_values[ADCCurrentIn];
    //     AdcAllValues[ADCVCC] += ADC_values[ADCVCC];

    //     if(time >= 102)
    //     {
    //         ADC_values_f[ADCCurrentIn] = AdcAllValues[ADCCurrentIn]/time;
    //         ADC_values_f[ADCVCC] = AdcAllValues[ADCVCC]/time;             //2223 - 24.63V

    //         AdcAllValues[ADCCurrentIn] = 0;
    //         AdcAllValues[ADCVCC] = 0;

    //         time = 0;
    //         framestime++;
            
    //         SwitchHighOrLowPower(SOF[framestime-1]);
    //         if(framestime == 8)
    //         {
    //             framestime = 0;
    //             NumOfSOF++;
    //         }
    //         if(NumOfSOF >= 2)
    //         {
    //             NumOfSOF = 0;
    //             DWT_Delay(0.092);
    //             ceshiman++;
    //         }
    //     }
    // }
}

void LowPower()
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_9,GPIO_PIN_SET);
}

void HighPower()
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_9,GPIO_PIN_RESET);
}

//BBEN开则电容充电，关则电容不充电
void SwitchBBEN(int On_Off)
{
    if(On_Off)
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_RESET);
    }

}

void SwitchENA_ENB(int On_Off)
{
    if(On_Off)
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET);
    }
}

void SwitchHighOrLowPower(int On_Off)
{
    if(On_Off == 0)
    {
        HighPower();
    }
    else
    {
        LowPower();
    }
    
}

void IntLimiter(int num,int min,int max)
{
    if(min < num && num < max)
        return num;
    if(num < min)
        return min;
    if(num > max)
        return max;
}