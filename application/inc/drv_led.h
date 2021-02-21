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

void drv_led_init(void);
void drv_led_set (e_ledmode ledmode);

#endif