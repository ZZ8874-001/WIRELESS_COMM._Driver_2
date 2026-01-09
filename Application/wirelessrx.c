#include "wirelessrx.h"

#include "main.h"
#include "gpio.h"

#include "detect_task.h"
#include "bsp_adc.h"
#include "bsp_dwt.h"
#include "filter32.h"

static void Debug_Task(void);
static void Connecting_Task(void);
static void Connected_Task(void);
static void LowPower(void);
static void HighPower(void);
static void SwitchBBEN(int On_Off);
static void SwitchENA_ENB(int On_Off);
static void SwitchHighOrLowPower(uint8_t On_Off);

float Power;

// 10100110
// 01011001
// 左对齐
uint8_t SOF = 0b01100101;//0是高功率，1是低功率
uint8_t DOF = 0b00000001;
uint8_t num_0_or_1[2] = {
                        0b01111,
                        0b00011
                    };

enum RxStatus_t RxStatus = RxStatus_Unknow;
enum RxStatus_t last_RxStatus = RxStatus_Unknow;

void WirelessInit(void)
{
    SwitchBBEN(Off);
    SwitchENA_ENB(Off);
    LowPower();

    DWT_Delay(0.1);
    SwitchBBEN(On);
    SwitchENA_ENB(On);
    
    last_RxStatus = RxStatus_Connected;
    RxStatus = RxStatus_Connected;

}

void Transmit_Task(void)
{
    Power = VOUT_f * Current_f;
    switch(RxStatus)
    {
    case RxStatus_Debug:
        Debug_Task();
        break;
    case RxStatus_Connecting:
        HAL_GPIO_TogglePin(IND11_GPIO_Port,IND11_Pin);
        Connecting_Task();
        break;
    case RxStatus_Connected:
        IND11_GPIO_Port->BSRR = IND11_Pin;
        Connected_Task();
        break;
    case RxStatus_Disconnected:
        SwitchENA_ENB(Off);
        HighPower();
        IND11_GPIO_Port->BRR = IND11_Pin;
        if(is_TOE_Overtime(ADC1_WATCHDOG1_TOE) && is_TOE_Overtime(ADC1_WATCHDOG2_TOE))
        {
            RxStatus = RxStatus_Connecting;
        }
        break;
    }
    
}

static void Debug_Task(void)
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
                SwitchHighOrLowPower(num_0_or_1[SOF>>flag_debug&1]>>debug_byte_count & 1);
            }
            else if(flag_debug < 16)
            {
                SwitchENA_ENB(Off);
            }
        break;

        case 2:
            if(flag_debug < 8)
            {
                SwitchENA_ENB(On);
                SwitchHighOrLowPower(num_0_or_1[DOF>>flag_debug&1]>>debug_byte_count & 1);
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
                SwitchHighOrLowPower(num_0_or_1[SOF>>flag_debug&1]>>debug_byte_count & 1);
            }
            else if(flag_debug < 16)
            {
                SwitchHighOrLowPower(num_0_or_1[DOF>>(flag_debug - 8)&1]>>debug_byte_count & 1);
            }
        break;
        
        default:
        break;
    }
    

    debug_byte_count++;
    if(debug_byte_count >= 5)
    {
        debug_byte_count = 0;
        debug_frame_count++;
    }
}

static void Connecting_Task(void)
{
    static uint8_t connecting_frame_count = 0;
    static uint8_t connecting_byte_count = 0;

    if(connecting_frame_count < 8)
    {
        SwitchENA_ENB(On);
        SwitchHighOrLowPower(num_0_or_1[SOF>>(connecting_frame_count)&1]>>connecting_byte_count & 1);
    }
    else if(connecting_frame_count < 16)
    {
        SwitchENA_ENB(On);
        SwitchHighOrLowPower(num_0_or_1[DOF>>(connecting_frame_count - 8)&1]>>connecting_byte_count & 1);
    }
    else if(connecting_frame_count < 200)
    {
        SwitchENA_ENB(Off);
    }

    connecting_byte_count++;
    if(connecting_byte_count >= 5)
    {
        connecting_byte_count = 0;
        if(connecting_frame_count++ >= 200)
        {
            connecting_frame_count = 0;
        }
    }

    if(Power < 20 || 40 < Power || VOUT_f < 3.3)
    {
        Detect_Hook(CONNECTING_TO_CONNECTED_TOE);
    }
    else if(is_TOE_Overtime(CONNECTING_TO_CONNECTED_TOE))
    {
        last_RxStatus = RxStatus;
        RxStatus = RxStatus_Connected;
        connecting_byte_count = 0;
        connecting_frame_count = 0;
    }
    
}

static void Connected_Task(void)
{
    static uint8_t connected_frame_count = 0;
    static uint8_t connected_byte_count = 0;

    SwitchENA_ENB(On);
    if(connected_frame_count < 8)
    {
        SwitchHighOrLowPower(num_0_or_1[SOF>>(connected_frame_count)&1]>>connected_byte_count & 1);
    }
    else if(connected_frame_count < 16)
    {
           
        SwitchHighOrLowPower(num_0_or_1[DOF>>(connected_frame_count - 8)&1]>>connected_byte_count & 1);
    }
    else if(connected_frame_count < 200)
    {
        SwitchHighOrLowPower(0);  
    }

    connected_byte_count++;
    if(connected_byte_count >= 5)
    {
        connected_byte_count = 0;
        if(connected_frame_count++ >= 200)
        {
            connected_frame_count = 0;
        }
    }

    if(Power < 20 || 40 < Power || VOUT_f < 3.3)
    {
        Detect_Hook(CONNECTING_TO_CONNECTED_TOE);
        last_RxStatus = RxStatus;
        RxStatus = RxStatus_Connecting;

        connected_byte_count = 0;
        connected_frame_count = 0;
    }
}

void LowPower(void)
{
    PULSEA_GPIO_Port->BSRR = PULSEA_Pin;
    PULSEB_GPIO_Port->BSRR = PULSEB_Pin;
}

void HighPower(void)
{
    PULSEA_GPIO_Port->BRR = PULSEA_Pin;
    PULSEB_GPIO_Port->BRR = PULSEB_Pin;
}

//BBEN开则电容充电，关则电容不充电
void SwitchBBEN(int On_Off)
{
    if(On_Off)
    {
        CHARGE_EN_GPIO_Port->BSRR = CHARGE_EN_Pin;
    }
    else
    {
        CHARGE_EN_GPIO_Port->BRR = CHARGE_EN_Pin;
    }

}

void SwitchENA_ENB(int On_Off)
{
    if(On_Off)
    {
        ENA_GPIO_Port->BSRR = ENA_Pin;
        ENB_GPIO_Port->BSRR = ENB_Pin;
    }
    else
    {
        ENA_GPIO_Port->BRR = ENA_Pin;
        ENB_GPIO_Port->BRR = ENB_Pin;
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