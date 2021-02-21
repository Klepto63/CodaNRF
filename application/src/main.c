#include "FreeRTOS.h"
#include "task.h"
#include "app_main.h"

int main(void)
{
    app_main_init();

    // Start FreeRTOS scheduler.
    vTaskStartScheduler();

    while (true)
    {
        APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}
