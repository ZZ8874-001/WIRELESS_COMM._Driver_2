//**************************************************************************************/
/*
 * **************************************************************************
 * ********************                                  ********************
 * ********************      COPYRIGHT INFORMATION       ********************
 * ********************                                  ********************
 * **************************************************************************
 *                                                                          *
 *                                   _oo8oo_                                *
 *                                  o8888888o                               *
 *                                  88" . "88                               *
 *                                  (| -_- |)                               *
 *                                  0\  =  /0                               *
 *                                ___/'==='\___                             *
 *                              .' \\|     |// '.                           *
 *                             / \\|||  :  |||// \                          *
 *                            / _||||| -:- |||||_ \                         *
 *                           |   | \\\  -  /// |   |                        *
 *                           | \_|  ''\---/''  |_/ |                        *
 *                           \  .-\__  '-'  __/-.  /                        *
 *                         ___'. .'  /--.--\  '. .'___                      *
 *                      ."" '<  '.___\_<|>_/___.'  >' "".                   *
 *                     | | :  `- \`.:`\ _ /`:.`/ -`  : | |                  *
 *                     \  \ `-.   \_ __\ /__ _/   .-` /  /                  *
 *                 =====`-.____`.___ \_____/ ___.`____.-`=====              *
 *                                   `=---=`                                *
 * **************************************************************************
 * ********************                                  ********************
 * ********************                                  ********************
 * ********************         佛祖保佑 永远无BUG        ********************
 * ********************                                  ********************
 * **************************************************************************
 */

#include "wirelessrx.h"

#include "main.h"
#include "gpio.h"

#include "detect_task.h"
#include "bsp_adc.h"
#include "bsp_can.h"
#include "bsp_dwt.h"
#include "bsp_usart.h"
#include "filter32.h"

#define VIN_CONNECTING_TO_CONNECTED 14.0f

#define DEBUG_ENABLE_NO_CAN false

static void Debug_Task(void);
static void Connecting_Task(void);
static void Connected_Task(void);
static void LowPower(void);
static void HighPower(void);
static void SwitchBBEN(int On_Off);
static void SwitchENA_ENB(int On_Off);
static void SwitchHighOrLowPower(uint8_t On_Off);

float Power;
uint16_t tim15_arr = 500;

// 10100110
// 01011001

// 左对齐
uint8_t SOF = 0b01100101;//0是高功率，1是低功率
uint8_t DOF = 0b00000001;
uint8_t num_0_or_1[2] = {
                        0b00011,
                        0b01111
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
    
    last_RxStatus = RxStatus_Disconnected;
    RxStatus = RxStatus_Disconnected;

}

void Transmit_Task(void)
{
    Power = VOUT_f * Current_f;

    if(is_TOE_Overtime(CAN_BIGCUP_RX_TOE))
    {
        bigcup_data_rx.Cap_Voltage_REM = 0;
        bigcup_data_rx.backhome_flag = 0;
        bigcup_data_rx.charge_complete_flag = 0;
        bigcup_data_rx.reset_flag = 0;
    }

    if(is_TOE_Overtime(CAN_MIAO_RX_TOE))
    {
        miao_data_rx.backhome_flag = 0;
        miao_data_rx.reset_flag = 0;
    }

    TIM15->ARR = tim15_arr;
    switch(RxStatus)
    {
    case RxStatus_Debug:
        Debug_Task();
        break;
    case RxStatus_Connecting:
        // GPIOB->BRR = GPIO_PIN_11;
        Tx_Buf.Head = 0xBB;
        tim15_arr = 500;
        TIM15->CCR2 = 0.5 * tim15_arr;
        Connecting_Task();
        break;
    case RxStatus_Connected:
        // GPIOB->BSRR = GPIO_PIN_11;
        Tx_Buf.Head = 0xAA;
        tim15_arr = 800;
        TIM15->CCR2 = 0.2 * tim15_arr;
        Connected_Task();
        break;
    case RxStatus_Disconnected:
        // GPIOB->BRR = GPIO_PIN_11;
        Tx_Buf.Head = 0xBB;

        SwitchENA_ENB(Off);
        HighPower();
        
        tim15_arr = 500;
        TIM15->CCR2 = 0;

        Detect_Hook(CONNECTING_TO_CONNECTED_TOE);

        if(is_TOE_Overtime(ADC1_WATCHDOG1_TOE) 
        && is_TOE_Overtime(ADC1_WATCHDOG2_TOE) 
        && (bigcup_data_rx.backhome_flag || DEBUG_ENABLE_NO_CAN  || miao_data_rx.backhome_flag)
        && !is_TOE_Overtime(USART3_RX_TOE)
        && (VOUT_f <= (VIN_f * BUCK_OUTPUT_OVERVOLT_RELEASE_RATIO)))
        {
            last_RxStatus = RxStatus;
            RxStatus = RxStatus_Connecting;
        }
        break;
    case RxStatus_CurrentError:
        // GPIOB->BSRR = GPIO_PIN_11;
        Tx_Buf.Head = 0xAA;
        SwitchENA_ENB(On);
        tim15_arr = 250;

        PULSEA_GPIO_Port->BSRR = PULSEA_Pin;
        PULSEB_GPIO_Port->BSRR = PULSEB_Pin;

        static uint16_t current_error_count = 0;
        static bool currenterror_flag;
        currenterror_flag = 1;
        
        current_error_count++;
        if(current_error_count > 32768)
        {
            TIM15->CCR2 = 0.5 * tim15_arr;
        }
        else
        {
            TIM15->CCR2 = 0;
        }

        if(bigcup_data_rx.reset_flag || miao_data_rx.reset_flag)
        {
            last_RxStatus = RxStatus;
            RxStatus = RxStatus_Disconnected;
        }
        break;
    default:
        if(!currenterror_flag)
        {
            last_RxStatus = RxStatus;
            RxStatus = RxStatus_Disconnected;
        }
        else
        {
            last_RxStatus = RxStatus;
            RxStatus = RxStatus_CurrentError;
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
    static float first_into_connecting_time_ms = 0;

    if(last_RxStatus != RxStatus_Connecting)
    {
        connecting_byte_count = 0;
        connecting_frame_count = 0;

        first_into_connecting_time_ms = USER_GetTick();

        last_RxStatus = RxStatus;
        RxStatus = RxStatus_Connecting;
    }

    if(connecting_frame_count == 0)
    {
        SwitchENA_ENB(On);
        SwitchHighOrLowPower(num_0_or_1[1]>>connecting_byte_count & 1);
    }
    else if(connecting_frame_count < 8+1)
    {
        SwitchENA_ENB(On);
        SwitchHighOrLowPower(num_0_or_1[SOF>>(connecting_frame_count - 1)&1]>>connecting_byte_count & 1);
    }
    else if(connecting_frame_count < 16+1)
    {
        SwitchENA_ENB(On);
        SwitchHighOrLowPower(num_0_or_1[DOF>>(connecting_frame_count - 9)&1]>>connecting_byte_count & 1);
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

    if(VIN_f < VIN_CONNECTING_TO_CONNECTED)
    {
        Detect_Hook(CONNECTING_TO_CONNECTED_TOE);
        if(DWT_GetTimeline_ms() - first_into_connecting_time_ms > 1000)
        {
            last_RxStatus = RxStatus;
            RxStatus = RxStatus_Disconnected;
        }
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

    static uint8_t aaa_success = 0;

    if(aaa_success < 4)
    {
        if(connected_frame_count == 0)
        {
            SwitchENA_ENB(On);
            SwitchHighOrLowPower(num_0_or_1[1]>>connected_byte_count & 1);
        }
        if(connected_frame_count < 8+1)
        {
            SwitchENA_ENB(On);
            SwitchHighOrLowPower(num_0_or_1[SOF>>(connected_frame_count - 1)&1]>>connected_byte_count & 1);
        }
        else if(connected_frame_count < 16+1)
        {
            SwitchENA_ENB(On);
            SwitchHighOrLowPower(num_0_or_1[DOF>>(connected_frame_count - 9)&1]>>connected_byte_count & 1);
        }
        else if(connected_frame_count < 200)
        {
            SwitchENA_ENB(Off);
            
        }

    }
    else
    {
        aaa_success = 4;
        SwitchENA_ENB(Off);
    }
    

    connected_byte_count++;
    if(connected_byte_count >= 5)
    {
        connected_byte_count = 0;
        if(connected_frame_count++ >= 200)
        {
            connected_frame_count = 0;
            aaa_success++;
        }
    }

    if(bigcup_data_rx.charge_complete_flag)
    {
        last_RxStatus = RxStatus;
        RxStatus = RxStatus_CurrentError;

        connected_byte_count = 0;
        connected_frame_count = 0;
        aaa_success = 0;
    } 

    if (VIN_f >= VIN_CONNECTING_TO_CONNECTED)
    {
        Detect_Hook(CONNECTED_UVLO_TIMEOUT_TOE);
    }
    else if( is_TOE_Overtime(CONNECTED_UVLO_TIMEOUT_TOE) && Current_f < 0.2f)
    {
        Detect_Hook(CONNECTING_TO_CONNECTED_TOE);
        last_RxStatus = RxStatus;
        RxStatus = RxStatus_Connecting;

        connected_byte_count = 0;
        connected_frame_count = 0;
        aaa_success = 0;
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
    if(On_Off == 1){LowPower();}
    else if(On_Off == 0){HighPower();}
    
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