#ifndef WIRELESSRX_H
#define WIRELESSRX_H

#define On 1
#define Off 0

#define BUCK_OUTPUT_OVERVOLT_RELEASE_RATIO 0.90f
// #define Buck_OUTPUT_OVERVOLT_DISABLE_RATIO 1.01f

enum RxStatus_t
{
    RxStatus_Unknow = 0,
    RxStatus_Connecting ,
    RxStatus_Connected ,
    RxStatus_CurrentError ,
    RxStatus_Disconnected ,
    RxStatus_Debug ,
};

extern enum RxStatus_t RxStatus;
extern enum RxStatus_t last_RxStatus;

void WirelessInit(void);
void IntLimiter(int num,int min,int max);
void Transmit_Task(void);

#endif