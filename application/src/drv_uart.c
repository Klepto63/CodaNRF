#include <stdint.h>
#include "drv_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_drv_uart.h"
#include "mdw_log.h"
#include "bsp.h"

#define UART_RXBUFFER_SIZE 30

typedef enum {
    drvUar_readReady        = (EventBits_t)0x0001,
    drvUar_writeFinished    = (EventBits_t)0x0002
}e_drv_uart_communicationFlags;

static const uint8_t        rxBuffer[UART_RXBUFFER_SIZE];
static const nrf_drv_uart_t uartInstance = NRF_DRV_UART_INSTANCE(0);
static EventGroupHandle_t   uartFlagsHandle;
static SemaphoreHandle_t    uartMutexHandle;




static void uart_cb (void)
{
  DEBUG("Uart callback");
}

void drv_uart_init(void)
{

    uartFlagsHandle = xEventGroupCreate();
    ASSERT(NULL != uartFlagsHandle);
    uartMutexHandle = xSemaphoreCreateMutex();
    ASSERT(NULL != uartMutexHandle);

    nrf_gpio_cfg_input(RX_DBG_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_output(TX_DBG_PIN);
    nrf_gpio_pin_set(TX_DBG_PIN);

    nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG;
    config.pseltxd      = TX_DBG_PIN;
    config.pselrxd      = RX_DBG_PIN;
    config.use_easy_dma = false;
    ret_code_t errCode = nrf_drv_uart_init(&uartInstance, &config, NULL);

    if(NRF_SUCCESS != errCode)
    {
        DEBUG("Init uart fail");
    }
}



void drv_uart_send(uint8_t *pData, uint8_t size)
{
    ret_code_t errCode = nrf_drv_uart_tx(&uartInstance, pData, size);
}

void uart_event_callback(nrf_drv_uart_event_t *p_event, void *p_context)
{
    UNUSED_PARAMETER(p_context);
    switch(p_event->type)
    {
        case  NRF_DRV_UART_EVT_ERROR :
            //DEBUG("Uart com error");
            break;
        case  NRF_DRV_UART_EVT_RX_DONE : 
            if(0 < p_event->data.rxtx.bytes)
            {
             //   dataCb(p_event->data.rxtx.p_data, p_event->data.rxtx.bytes);
            }
            break;
        case   NRF_DRV_UART_EVT_TX_DONE:
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;
            xResult = xEventGroupSetBitsFromISR(uartFlagsHandle,
                                                drvUar_writeFinished,
                                                &xHigherPriorityTaskWoken);
            if( xResult != pdTRUE )
            {
                DEBUG("Flag error");
            }
            if(pdTRUE == xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
            break;
        default :
            DEBUG("Unknown event");
    }
}
