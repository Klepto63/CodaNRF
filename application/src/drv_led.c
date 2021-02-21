#include <string.h>
#include <stdint.h>
#include "bsp.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "mdw_log.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "queue.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "nrf_drv_pwm.h"
#include "drv_led.h"

#define PERDIOD_SLOW_MS 1000 //5
#define PERDIOD_FAST_MS 200

static TimerHandle_t  timerHandle;
static void	timer_cb (TimerHandle_t timer);

static e_ledmode currentledmode = e_ledmodeUnkown;

void drv_led_init(void)
{
    currentledmode = e_ledmodeOff;

    nrf_gpio_cfg_output(LED_PIN);
    nrf_gpio_pin_set(LED_PIN);

    timerHandle = xTimerCreate( "LED",
                                pdMS_TO_TICKS(PERDIOD_SLOW_MS),
                                pdTRUE,
                                NULL,
                                timer_cb);
    if(NULL == timerHandle)
    {
        DEBUG("timer creation ERROR");
    }
}

void drv_led_set(e_ledmode ledmode)
{
    switch(ledmode)
    {
        case e_ledmodeOff:
        {
            nrf_gpio_pin_clear(LED_PIN);
            xTimerStop(timerHandle,0);
            break;
        } 
        case e_ledmodeOn:
        {
            nrf_gpio_pin_set(LED_PIN);
            xTimerStop(timerHandle,0);
            break;
        }
        case e_ledmodeBlinkSlow:
        {
            xTimerChangePeriod(timerHandle, pdMS_TO_TICKS(PERDIOD_SLOW_MS), 0);
            nrf_gpio_pin_set(LED_PIN);
            break;
        }
        case e_ledmodeBlinkFast :
        {
            xTimerChangePeriod(timerHandle, pdMS_TO_TICKS(PERDIOD_FAST_MS), 0);
            nrf_gpio_pin_set(LED_PIN);
            break;
        }
        default : 
        {
            break;
        }
    }
}

static void timer_cb(TimerHandle_t timer)
{
    nrf_gpio_pin_toggle(LED_PIN);
    UNUSED_PARAMETER(timer);
}
