#ifndef DRV_LED_H8
#define DRV_LED_H8

typedef enum
{
    e_ledmodeUnkown = 0,
    e_ledmodeOff, 
    e_ledmodeOn,
    e_ledmodeBlinkSlow,
    e_ledmodeBlinkFast
}e_ledmode;


typedef enum
{
    e_fsmModeUnkown = 0,
    e_fsmModeNotConnected,
    e_fsmModeConnected
}e_fsmMode;

void drv_led_init(void);
void drv_led_set (e_ledmode ledmode);


void drv_led_set_mode(e_fsmMode fsmMode);




#endif