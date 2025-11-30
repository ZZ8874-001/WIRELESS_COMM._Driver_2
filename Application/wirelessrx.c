#include "wirelessrx.h"

//develop brench test

static void Debug_Task();
static void Connected_Task();
static void LowPower();
static void HighPower();
static void SwitchBBEN(int On_Off);
static void SwitchENA_ENB(int On_Off);
static void SwitchHighOrLowPower(uint8_t On_Off);

uint32_t ADC_values[ADC_DataSize];
float V_values[ADC_DataSize];
// uint32_t AdcAllValues[ADC_DataSize];
uint32_t Rx_DWT_Count;
float rx_dt = 0.0f;
float rx_t = 0.0f;
float VIn_f;
float VCC_f;
// 10100110
// 01001101
uint8_t SOF[8] = {1,0,1,0,0,1,1,0};//0是高功率，1是低功率
uint8_t DOF[16] = {1,0,0,0,0,0,0,0};
uint8_t num_0_or_1[2][5] = {{1,1,1,1,0},
                            {1,1,0,0,0}
                                };
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
    while(HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED) != HAL_OK)
    {
    }
    while(HAL_ADC_Start_DMA(&hadc1,ADC_values,ADC_DataSize) != HAL_OK)
    {
    }

    First_Order_Filter_Init(&VInFilter,1/97500.0f,30.0f);
    First_Order_Filter_Init(&VCCFilter,1/97500.0f,30.0f);

    RxStatus = RxStatus_Connected;

}

void Transmit_Task()
{
    rx_dt = DWT_GetDeltaT(&Rx_DWT_Count);
    rx_t += rx_dt;
    switch(RxStatus)
    {
    case RxStatus_Debug:
        Debug_Task();
        break;
    case RxStatus_Connected:
        Connected_Task();
        break;
    case RxStatus_Disconnected:
        SwitchENA_ENB(Off);
        HighPower();
        break;
    }
    
}

static void Debug_Task()
{
    static uint8_t debug_frame_count = 0;
    static uint8_t debug_byte_count = 0;
    volatile static uint8_t debug_mode = 0;
    uint8_t flag_debug = debug_frame_count%16;
    

    switch(debug_mode)
    {
        case 1:
            if(flag_debug < 8)
            {
                SwitchENA_ENB(On);
                SwitchHighOrLowPower(num_0_or_1[SOF[flag_debug]][debug_byte_count]);
            }
            else if(8 <= flag_debug && flag_debug < 16)
            {
                SwitchENA_ENB(Off);
            }
        break;

        case 2:
            if(flag_debug < 8)
            {
                SwitchENA_ENB(On);
                SwitchHighOrLowPower(num_0_or_1[DOF[flag_debug]][debug_byte_count]);
            }
            else if(flag_debug < 16)
            {
                SwitchENA_ENB(Off);
            }
        break;

        case 3:
            SwitchENA_ENB(On);    
            if(flag_debug < 8)
            {
                SwitchHighOrLowPower(num_0_or_1[SOF[flag_debug]][debug_byte_count]);
            }
            else if(flag_debug < 16)
            {
                SwitchHighOrLowPower(num_0_or_1[DOF[flag_debug]][debug_byte_count]);
            }
        break;
        default:
        
    }
    

    debug_byte_count++;
    if(debug_byte_count >= 5)
    {
        debug_byte_count = 0;
        debug_frame_count++;
    }
}

static void Connected_Task()
{
    static uint8_t connected_frame_count = 0;
    static uint8_t connected_byte_count = 0;

    if(connected_frame_count < 8)
    {
        SwitchENA_ENB(On);    
        SwitchHighOrLowPower(num_0_or_1[SOF[connected_frame_count]][connected_byte_count]);
    }
    else if(connected_frame_count < 16)
    {
        SwitchENA_ENB(On);    
        SwitchHighOrLowPower(num_0_or_1[DOF[connected_frame_count - 8]][connected_byte_count]);
    }
    else if(connected_frame_count < 200)
    {
        SwitchENA_ENB(Off);   
    }
    else
        connected_frame_count = 0;

    connected_byte_count++;
    if(connected_byte_count >= 5)
    {
        connected_byte_count = 0;
        connected_frame_count++;
    }
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

void SwitchHighOrLowPower(uint8_t On_Off)
{
    if(On_Off){LowPower();}
    else{HighPower();}
    
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