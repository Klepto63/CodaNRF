#ifndef DRV_HELMET_H
#define DRV_HELMET_H

#include "param_global.h"
#include "drv_acc.h"

typedef enum{
    e_helmetEventBase = 0x20,
    e_helmetEventRxEvent,
    e_helmetEventUnkown
} e_helmetEvent;

void    drv_helmet_init(t_cb cb); //enclenche scan et reception data
uint8_t drv_helmet_acc_read(s_accValue* accValues);
bool    drv_helmet_stream_started();

#endif