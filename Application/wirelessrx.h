#ifndef WIRELESSRX_H
#define WIRELESSRX_H

#define On 1
#define Off 0

enum RxStatus_t
{
    RxStatus_Unknow = 0,
    RxStatus_Connecting = 1,
    RxStatus_Disconnected = 2,
    RxStatus_Connected = 3,
    RxStatus_Debug = 4,
};

extern enum RxStatus_t RxStatus;

void WirelessInit();
void IntLimiter(int num,int min,int max);
void Transmit_Task();

#endif