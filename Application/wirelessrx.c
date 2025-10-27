#include "wirelessrx.h"

//develop brench test

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
    static uint8_t count_connected = 0;
    static uint8_t count_connecting = 0;

    switch(RxStatus)
    {
    case RxStatus_Connecting:
        SwitchHighOrLowPower(SOF[count_connecting%8]);
        count_connected = 0;
        count_connecting++;
        break;
    case RxStatus_Connected:
        // 0.5ms
        static uint8_t data = 0;

        if(count_connected < 8)
        {
            SwitchHighOrLowPower(SOF[count_connected]);
            data =  (uint8_t)VIn_f;
        }
        else if(count_connected < 16)
        {
            DOF[count_connected - 8] = (data >> (15 - count_connected)) & 0x01;
            SwitchHighOrLowPower(DOF[count_connected - 8]);
        }
        else if(count_connected < 200)
        {
            HighPower();
        }
        else
        {
            count_connected = 0;
        }
        count_connected++;
        count_connecting = 0;
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
        else if(RxStatus != RxStatus_Connecting && RxStatus != RxStatus_Connected)
        {
            RxStatus = RxStatus_Disconnected;
        }
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
    HAL_GPIO_WritePin(PULSEA_GPIO_Port,PULSEA_Pin,GPIO_PIN_SET);
    HAL_GPIO_WritePin(PULSEB_GPIO_Port,PULSEB_Pin,GPIO_PIN_SET);
}

void HighPower()
{
    HAL_GPIO_WritePin(PULSEA_GPIO_Port,PULSEA_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PULSEB_GPIO_Port,PULSEB_Pin,GPIO_PIN_RESET);
}

//BBEN开则电容充电，关则电容不充电
void SwitchBBEN(int On_Off)
{
    if(On_Off)
    {
        HAL_GPIO_WritePin(BBEN_GPIO_Port,BBEN_Pin,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(BBEN_GPIO_Port,BBEN_Pin,GPIO_PIN_RESET);
    }

}

void SwitchENA_ENB(int On_Off)
{
    if(On_Off)
    {
        HAL_GPIO_WritePin(ENA_GPIO_Port,ENA_Pin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(ENB_GPIO_Port,ENB_Pin,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(ENA_GPIO_Port,ENA_Pin,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ENB_GPIO_Port,ENB_Pin,GPIO_PIN_RESET);
    }
}

void SwitchHighOrLowPower(int On_Off)
{
    if(On_Off)
    {
        LowPower();
    }
    else
    {
        HighPower();
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