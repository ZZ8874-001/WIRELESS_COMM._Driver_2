#include "bsp_adc.h"
#include "main.h"

#include "wirelessrx.h"
#include "detect_task.h"
#include "filter32.h"

#define ADC_SAMPLING_FREQUENCY (12.5e3)

//#define ADC_RATIO (2.9832f/4096.0f)
//#define ADC_RATIO_DIFF (ADC_RATIO*2)
//#define ADC_VOLTAGE_RATIO 22.227f
//#define ADC_CURRENT_RATIO 10.3341f
//#define CURRENT_OUT_OFFSET -17.1348f
//#define VOLTAGE_OUT_OFFSET -0.0129f

float ADC_RATIO[BOARD_NUM] = { 2.9832f/4096.0f, 2.9827f/4096.0f, 2.9845f/4096.0f, 2.9786f/4096.0f ,2.9815f/4096.0f };
float ADC_RATIO_DIFF[BOARD_NUM] = { 2.9832f/4096.0f * 2 , 2.9837f/4096.0f * 2 , 2.9845f/4096.0f * 2 , 2.9786f/4096.0f * 2 , 2.9815f/4096.0f * 2};
float CURRENT_OUT_OFFSET[BOARD_NUM] = { -8.3960f, -17.1348f , -12.6225f , -16.7374f , -16.7374f};
float VOLTAGE_OUT_OFFSET[BOARD_NUM] = { -0.012900f, -0.012900f , 0.037000f , 0.037000f , 0.037000f};
float ADC_VOLTAGE_RATIO[BOARD_NUM] = { 22.227f , 22.227f , 20.134f , 20.846f , 20.846f};
float ADC_CURRENT_RATIO[BOARD_NUM] = { 5.100f , 10.3341f , 7.6570f , 10.1010f , 10.1010f};

#define VIN_MIN 4.8f
#define VIN_MAX 48.0f

#define VOUT_MIN 0.0f
#define VOUT_MAX 32.0f

#define CURRENT_MIN 0.0f
#define CURRENT_MAX 6.0f

static void Change_ADC_AWD_Threshold(uint32_t *ADCx_TRx,uint16_t low_threshold,uint16_t high_threshold);

First_Order_Filter_t VInFilter;
First_Order_Filter_t VCCFilter;

First_Order_Filter_t CurrentFilter;
First_Order_Filter_t Current_f_Filter;

uint16_t ADC1_values[2];
uint16_t ADC2_values[1];

float VIN_f;
float VOUT_f;
float Current_f;
float Current_f_f;

void Bsp_ADC_Init()
{
    First_Order_Filter_Init(&VInFilter,1.0f/ADC_SAMPLING_FREQUENCY,8000.0f);
    First_Order_Filter_Init(&VCCFilter,1.0f/ADC_SAMPLING_FREQUENCY,8000.0f);
    First_Order_Filter_Init(&CurrentFilter,1.0f/ADC_SAMPLING_FREQUENCY,8000.0f);
    First_Order_Filter_Init(&Current_f_Filter,1.0f/ADC_SAMPLING_FREQUENCY,10.0f);

    float VIN_WATCHDOG_MIN = VIN_MIN/ADC_RATIO[IDCard]/ADC_VOLTAGE_RATIO[IDCard];
    float VIN_WATCHDOG_MAX = VIN_MAX/ADC_RATIO[IDCard]/ADC_VOLTAGE_RATIO[IDCard];

    float VOUT_WATCHDOG_MIN = VOUT_MIN/ADC_RATIO[IDCard]/ADC_VOLTAGE_RATIO[IDCard];
    float VOUT_WATCHDOG_MAX = VOUT_MAX/ADC_RATIO[IDCard]/ADC_VOLTAGE_RATIO[IDCard];

    float CURRENT_WATCHDOG_MIN = CURRENT_MIN/ADC_RATIO[IDCard]/ADC_VOLTAGE_RATIO[IDCard];
    float CURRENT_WATCHDOG_MAX = CURRENT_MAX/ADC_RATIO[IDCard]/ADC_VOLTAGE_RATIO[IDCard];

    while(HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED) != HAL_OK);
    while(HAL_ADCEx_Calibration_Start(&hadc2,ADC_DIFFERENTIAL_ENDED) != HAL_OK);
    while(HAL_ADC_Start_DMA(&hadc1,ADC1_values,sizeof(ADC1_values)/sizeof(ADC1_values[0])) != HAL_OK);
    while(HAL_ADC_Start_DMA(&hadc2,ADC2_values,sizeof(ADC2_values)/sizeof(ADC2_values[0])) != HAL_OK);

    Change_ADC_AWD_Threshold(&ADC1->TR1,VIN_WATCHDOG_MIN,(VIN_WATCHDOG_MAX > 4095 ? 4095 : VIN_WATCHDOG_MAX));   //  VIN 14-48
    Change_ADC_AWD_Threshold(&ADC1->TR2,VOUT_WATCHDOG_MIN/16,VOUT_WATCHDOG_MAX/16);   //  VOUT  0-28
    Change_ADC_AWD_Threshold(&ADC2->TR1,0,4095);    //  CURRENT
    ADC1->AWD2CR = 1 << 2;
    
    DMA1_Channel1->CCR &= ~(DMA_CCR_HTIE | DMA_CCR_TCIE);
    DMA1_Channel4->CCR &= ~(DMA_CCR_HTIE | DMA_CCR_TCIE);

    ADC1->IER |= ADC_IER_AWD1 | ADC_IER_AWD2 | ADC_IER_EOS;
    ADC2->IER |= ADC_IER_AWD1 | ADC_IER_EOS;

    DMA1_Channel1->CCR &= 0xFFFB;
    DMA1_Channel2->CCR &= 0xFFFB; 
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        float V_values[2];
        V_values[ADCVIN] = ADC1_values[ADCVIN] * ADC_RATIO[IDCard] * ADC_VOLTAGE_RATIO[IDCard] + VOLTAGE_OUT_OFFSET[IDCard];
        V_values[ADCVCC] = ADC1_values[ADCVCC] * ADC_RATIO[IDCard] * ADC_VOLTAGE_RATIO[IDCard] + VOLTAGE_OUT_OFFSET[IDCard];

        VIN_f = First_Order_Filter_Calculate(&VInFilter,V_values[ADCVIN]);
        VOUT_f = First_Order_Filter_Calculate(&VCCFilter,V_values[ADCVCC]);

    }
    else if(hadc->Instance == ADC2)
    {
        static float Current;
        Current =(ADC2_values[0] - 2048) * ADC_RATIO_DIFF[IDCard] * ADC_CURRENT_RATIO[IDCard] + CURRENT_OUT_OFFSET[IDCard];

        Current_f = First_Order_Filter_Calculate(&CurrentFilter,Current);
        Current_f_f = First_Order_Filter_Calculate(&Current_f_Filter,Current);

        if(Current_f < 0.8 * Current_f_f && RxStatus == RxStatus_Connected && Current_f > 0.6f)
        {
            last_RxStatus = RxStatus;
            RxStatus = RxStatus_CurrentError;
        }
    }
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        // VIN
        Detect_Hook(ADC1_WATCHDOG1_TOE);
        static uint8_t uvlo_timeout_flag,adc1_watchdog1_sb_flag;
        //static uint32_t adc1_watchdog2_sb_flag;
        uvlo_timeout_flag = (is_TOE_Overtime(CONNECTED_UVLO_TIMEOUT_TOE) && Current_f <= 0.2f);
        adc1_watchdog1_sb_flag = (ADC1_values[ADCVIN] < (ADC1->TR1 & 0xfff) && RxStatus != RxStatus_CurrentError ) && uvlo_timeout_flag;
        if(RxStatus != RxStatus_Connecting && adc1_watchdog1_sb_flag)
        {
            RxStatus = RxStatus_Disconnected;
            //adc1_watchdog2_sb_flag = HAL_GetTick();
        }
        else if(ADC1_values[ADCVIN] > ((ADC1->TR1 >> 16) & 0xfff))
        {
            RxStatus = RxStatus_CurrentError;
        }

        // if(RxStatus != RxStatus_CurrentError)
        // {
        //     last_RxStatus = RxStatus;
        //     // RxStatus = RxStatus_Disconnected;
        //     RxStatus = RxStatus_CurrentError;
        // }
        
    }
    else if(hadc->Instance == ADC2)
    {
        // Current
        Detect_Hook(ADC2_WATCHDOG1_TOE);
    }
}

void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {        
        // VOUT
        Detect_Hook(ADC1_WATCHDOG2_TOE);

        if(RxStatus != RxStatus_CurrentError)
        {
            last_RxStatus = RxStatus;
            // RxStatus = RxStatus_Disconnected;
            RxStatus = RxStatus_CurrentError;
        }
    }
}

static void Change_ADC_AWD_Threshold(uint32_t *ADCx_TRx,uint16_t low_threshold,uint16_t high_threshold)
{
    *ADCx_TRx = (high_threshold << 16) | low_threshold;
}
