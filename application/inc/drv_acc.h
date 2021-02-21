#ifndef DRV_ACC_H
#define DRV_ACC_H

typedef struct 
{
    float accPitch;
    float accHeading;
    float accRoll;
}s_accValue;

typedef enum{
    e_accEventBase = 0x30,
    e_accEventRxEvent,
    e_accEventUnkown
} e_accEvent;


void   drv_acc_init    (t_cb cb);

void   drv_acc_test    (t_cb cb);

void   drv_acc_read   (s_accValue* accValue);

void  drv_acc_enable_stream(bool en);

#endif