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

#define PWM_STEP_COUNT 36

e_fsmMode fsmMode;

static e_ledmode currentledmode = e_ledmodeUnkown;
static void	pwm_cb		            (nrf_drv_pwm_evt_type_t event_type);
static nrf_drv_pwm_t				pwmInstance	= NRF_DRV_PWM_INSTANCE(PWM0_INSTANCE_INDEX);
static nrf_drv_pwm_handler_t		pwmHandler	= NULL;

void play_not_connected()
{
    uint8_t intensity_percent = 100;
    static nrf_pwm_values_common_t fade_in_out_values[2 * PWM_STEP_COUNT];
    uint16_t value = 0;
    uint16_t step  = PWM_DEFAULT_CONFIG_TOP_VALUE / PWM_STEP_COUNT;
    uint16_t offset = (PWM_DEFAULT_CONFIG_TOP_VALUE * (100 - intensity_percent)) / 100;
    for (int i = 0; i < PWM_STEP_COUNT; ++i)
    {
        //value									  += (step * intensity_percent) / 100;
        //fade_in_out_values[PWM_STEP_COUNT + i]  = (value + offset) ;
        //fade_in_out_values[i]					  = (PWM_DEFAULT_CONFIG_TOP_VALUE - value) ;

        value									  += (step * intensity_percent) / 100;
        fade_in_out_values[PWM_STEP_COUNT + i]    = (value + offset) ;
        fade_in_out_values[i]					  = 0 ;

    }
    if(NULL != pwmHandler)
    {
        pwmHandler = NULL;
        nrf_drv_pwm_uninit(&pwmInstance);
    }
    pwmHandler = pwm_cb;
    nrf_drv_pwm_config_t pwmConfig = NRF_DRV_PWM_DEFAULT_CONFIG;
    pwmConfig.output_pins[0] = LED_PIN;
    ret_code_t ret = nrf_drv_pwm_init(&pwmInstance, &pwmConfig, pwmHandler);
    if(NRF_SUCCESS != ret)
    {
        DEBUG("nrf_drv_pwm_init error");
    }

    nrf_pwm_sequence_t const seq =
    {
        .values.p_common = fade_in_out_values,
        .length          = NRF_PWM_VALUES_LENGTH(fade_in_out_values),
        .repeats         = 7,
        .end_delay       = 0
    };
    nrf_drv_pwm_simple_playback(&pwmInstance, &seq, 5, 0); //repeat =5
}


void drv_led_set_mode(e_fsmMode fsmMode)
{

}

void drv_led_init(void)
{
    fsmMode = e_fsmModeNotConnected;
    nrf_gpio_cfg_output(LED_PIN);
    nrf_gpio_pin_set(LED_PIN);
    play_not_connected();//start notconencted
}

void pwm_cb(nrf_drv_pwm_evt_type_t event_type)
{
	DEBUG("pwb cb");
    if(fsmMode == e_fsmModeNotConnected)
    {
        play_not_connected();   
    }
}

void drv_led_set(e_ledmode ledmode)
{
    switch(ledmode)
    {
        case e_ledmodeOff:
        {
            nrf_gpio_pin_clear(LED_PIN);
            break;
        } 
        case e_ledmodeOn:
        {
            nrf_gpio_pin_set(LED_PIN);
            break;
        }
        case e_ledmodeBlinkSlow:
        {
            nrf_gpio_pin_set(LED_PIN);
            break;
        }
        case e_ledmodeBlinkFast :
        {
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
