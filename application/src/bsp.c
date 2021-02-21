#include "bsp.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_power.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "param_global.h"
#include "nrf_drv_lpcomp.h" 
#include "izn_param.h"
#include "nrf_drv_lpcomp.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "mdw_log.h"

void drv_bsp_init ()
{
    nrf_drv_clock_init();
    nrf_drv_power_config_t config = {.dcdcen = 1};
    nrf_drv_power_init(&config);
    nrf_drv_gpiote_init();
    nrf_gpio_cfg_output(BNO_RESETn);
    nrf_gpio_pin_set(BNO_RESETn);
}
