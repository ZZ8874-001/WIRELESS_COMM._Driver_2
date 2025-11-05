#include "wirelessrx.h"

//develop brench test

uint32_t ADC_values[ADC_DataSize];
float V_values[ADC_DataSize];
// uint32_t AdcAllValues[ADC_DataSize];
uint32_t DWT_Count;
float VIn_f;
float VCC_f;
// 10100110
// 0100110 1
uint8_t SOF[8] = {1,0,1,0,0,1,1,0};//0是高功率，1是低功率
uint8_t SOF_decode[16];
uint8_t DOF[16] = {1,0,0,0,0,0,0,0};
uint8_t DOF_decode[16];
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
    SOF_To_Decode();

}

void QITask()
{
    static uint8_t data = 0;
    static int16_t count_connected = 0;
    static int16_t count_debug = 0;

    switch(RxStatus)
    {
    case RxStatus_Debug:
        if(count_debug < 16)
        {
            SwitchENA_ENB(On);
            SwitchHighOrLowPower(SOF_decode[count_debug]);
        }
        else if(count_debug < 32)
        {
            SwitchENA_ENB(On);
            DOF_To_Decode();
            SwitchHighOrLowPower(DOF_decode[count_debug - 16]);
        }
        else
        {
            count_debug = -1;
        }
        count_debug++;
        count_connected = 0;
        break;
    case RxStatus_Connected:
        // 0.25ms 4kHz
        // 100ms 1period
        if(count_connected < 16)
        {
            SwitchENA_ENB(On);
            SwitchHighOrLowPower(SOF_decode[count_connected]);
        }
        else if(count_connected < 32)
        {
            SwitchENA_ENB(On);
            DOF_To_Decode();
            SwitchHighOrLowPower(DOF_decode[count_connected - 16]);
        }
        else if(count_connected < 400)
        {
            SwitchENA_ENB(Off);
            HighPower();
        }
        else
        {
            count_connected = -1;
        }
        count_connected++;
        count_debug = 0;
        break;
    case RxStatus_Disconnected:
        SwitchENA_ENB(Off);
        HighPower();
        break;
    }
    
}

void SOF_To_Decode()
{
    SOF_decode[0] = 1;
    SOF_decode[1] = 0;
    for(int i=1;i<8;i++)
    {
        if(SOF[i] == 1)
        {
            SOF_decode[i*2] = !SOF_decode[i*2-1];
            SOF_decode[i*2+1] = SOF_decode[i*2-1];       
        }
        else
        {
            SOF_decode[i*2] = !SOF_decode[i*2-1];
            SOF_decode[i*2+1] = !SOF_decode[i*2-1];
        }
    }
}

void DOF_To_Decode()
{
    if(DOF[0] == 1)
    {
        DOF_decode[0] = !SOF_decode[15];
        DOF_decode[1] = SOF_decode[15];
    }
    else
    {
        DOF_decode[0] = !SOF_decode[15];
        DOF_decode[1] = !SOF_decode[15];
    }

    for(int i=1;i<8;i++)
    {
        if(DOF[i] == 1)
        {
            DOF_decode[i*2] = !DOF_decode[i*2-1];
            DOF_decode[i*2+1] = DOF_decode[i*2-1];       
        }
        else
        {
            DOF_decode[i*2] = !DOF_decode[i*2-1];
            DOF_decode[i*2+1] = !DOF_decode[i*2-1];
        }
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

        VIn_f = 20.0f;
        if(RxStatus != RxStatus_Debug)
        {
            if(VIn_f > 19.0f)
            {
                // RxStatus = RxStatus_Connecting;
                RxStatus = RxStatus_Connected;
            }
            else if(VIn_f < 17.0f)
            {
                RxStatus = RxStatus_Disconnected;
            }
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