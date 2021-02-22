#ifndef DRV_BSP_H
#define DRV_BSP_H

#include "param_global.h"

#define I2C_SCL             6
#define I2C_SDA             7
#define SCK_PIN             8
#define BNO_INT             9
#define BNO_RESETn          29
#define LED_PIN             18 //2


#define RX_DBG_PIN              8
#define TX_DBG_PIN              6





#define LOG(ERR_CODE)                                       \
    do                                                      \
    {                                                       \
        NRF_LOG_INFO((ERR_CODE));                           \
        NRF_LOG_FLUSH();                                    \
    }while(0);

typedef enum{
    e_bspEventBase = 0x10,
    e_bspEventBattDown,
    e_bspEventBattUp,
    e_bspEventBattUnkown
} e_bspEvent;

void  drv_bsp_init();

#endif

