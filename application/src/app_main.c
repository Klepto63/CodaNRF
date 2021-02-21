#include <string.h>
#include <stdint.h>
#include "app_main.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "mdw_log.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "drv_acc.h"
#include "queue.h"
#include "drv_helmet.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "app_error.h"

#include "drv_led.h"

s_accValue phareAccValue = {0};

#define QUEUE_LENGTH          20
static TaskHandle_t           mainTaskHandle;
static EventGroupHandle_t     mainFlagsHandle;
static QueueHandle_t          mainQueueHandle;
static EventGroupHandle_t     mainFlagHandle;
typedef enum
{
    k_mainFlagBitsPoweron       = (EventBits_t)0x01,
}e_mainFlagBits;

static void rtos_init         (void);
static void app_main_task     (void *pArg);

static void acc_cb            (uint32_t event, t_time ts_ms, void* pData);
static void hel_cb            (uint32_t event, t_time ts_ms, void* pData);

void app_main_init(void)
{
    drv_bsp_init();
    rtos_init();
    mdw_log_init();
}

void app_main_task(void *pArg)
{
    DEBUG("MAIN : init");

    drv_led_init();
    drv_led_set(e_ledmodeBlinkFast);
    //drv_acc_init((t_cb) acc_cb);
    //drv_helmet_init((t_cb) hel_cb);
    
    vTaskDelay(500);
    //led
    while(1)
    {
        DEBUG("hello");
        vTaskDelay(1000); 
    }
    

    while(1)
    {
        s_message msg;

        BaseType_t ret = xQueueReceive(mainQueueHandle,
                                       &msg,
                                       portMAX_DELAY);
        if(pdFALSE == ret)
        {
            DEBUG("queue receive error");
        }

        if((msg.id & e_helmetEventBase) == e_helmetEventBase)
        {

        }

        if((msg.id & e_accEventBase) == e_accEventBase)
        {

        }
    }
}

          

static void rtos_init(void)
{
    mainFlagsHandle = xEventGroupCreate();
    mainQueueHandle = xQueueCreate(QUEUE_LENGTH, sizeof(s_message));
    ASSERT(NULL != mainQueueHandle);
    BaseType_t xResult = xTaskCreate(app_main_task,
                        "GOV",
                        configMINIMAL_STACK_SIZE + 400,
                        NULL,
                        1,
                        &mainTaskHandle);
    ASSERT(xResult == pdPASS);
    mainFlagHandle = xEventGroupCreate();
}

static void acc_cb(uint32_t event, t_time ts_ms, void *pData)
{
    s_message msg = {0};
    msg.id = event;
    if(NULL != pData)
    {
        msg.data = *(s_pData*)pData;
    }

    BaseType_t xHigherPriorityTaskWoken;
    BaseType_t ret = xQueueSendFromISR( mainQueueHandle,
                                        &msg,
                                        &xHigherPriorityTaskWoken);
    if(pdTRUE != ret)
    {
        DEBUG("ERROR Queue send");
    } 
}

static void hel_cb(uint32_t event, t_time ts_ms, void* pData)
{
    s_message msg = {0};
    msg.id = event;
    if(NULL != pData)
    {
        msg.data = *(s_pData*)pData;
    }

    BaseType_t xHigherPriorityTaskWoken;
    BaseType_t ret = xQueueSendFromISR( mainQueueHandle,
                                        &msg,
                                        &xHigherPriorityTaskWoken);
    if(pdTRUE != ret)
    {
        DEBUG("ERROR Queue send");
    } 
}