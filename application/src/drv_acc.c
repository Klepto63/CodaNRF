#include "param_global.h"
#include "drv_acc.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "mdw_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
#include "nrf_drv_twi.h"
#include "bsp.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

#define BNO055_OPR_MODE_ADDR 	    0X3D
#define BNO055_PWR_MODE_ADDR 	    0X3E
#define BNO055_SYS_TRIGGER_ADDR     0X3F
#define BNO055_TEMP_SOURCE_ADDR	    0X40
#define POWER_MODE_NORMAL	        0X00
#define POWER_MODE_LOWPOWER	        0X01
#define POWER_MODE_SUSPEND	        0X02
#define OPERATION_MODE_CONFIG       0X00
#define OPERATION_MODE_ACCONLY      0X01
#define OPERATION_MODE_MAGONLY      0X02
#define OPERATION_MODE_GYRONLY      0X03
#define OPERATION_MODE_ACCMAG       0X04
#define OPERATION_MODE_ACCGYRO      0X05
#define OPERATION_MODE_MAGGYRO      0X06
#define OPERATION_MODE_AMG          0X07
#define OPERATION_MODE_IMUPLUS      0X08
#define OPERATION_MODE_COMPASS      0X09
#define OPERATION_MODE_M4G          0X0A
#define OPERATION_MODE_NDOF_FMC_OFF 0X0B
#define OPERATION_MODE_NDOF 		0X0C

#define BNO055_ADDRESS              0x28
#define TWI_INSTANCE_ID             1

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);
static volatile bool m_xfer_done = false;
static t_cb                 event_cb;
static EventGroupHandle_t   flagHandle;
static TaskHandle_t         taskHandle;
static void rtos_init (void);
static void acc_task  (void *pArg);
static uint8_t TxBuffer[2] = {0};
static uint8_t RxBuffer[6] = {0};
bool streamAccEnabled = true; //false si helmet present


static uint8_t read_reg(uint8_t reg_address)
{
    m_xfer_done = false;
    TxBuffer[0] = reg_address;


    //DEBUG("start read reg");

    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, BNO055_ADDRESS, TxBuffer, 1, true);
    APP_ERROR_CHECK(err_code);


    //DEBUG("TX OK, w f a");

    while(!m_xfer_done)
    {
        vTaskDelay(10);
    }


    //DEBUG("waf   OK ");


    m_xfer_done = false;
    err_code = nrf_drv_twi_rx(&m_twi, BNO055_ADDRESS, RxBuffer, 1);
    APP_ERROR_CHECK(err_code);


          //DEBUG("RX   OK waf");

    while(!m_xfer_done)
    {
        vTaskDelay(10);
    }

       //DEBUG("xfer 2   OK !!!");


    return RxBuffer[0];

}

static void write_reg(uint8_t reg_address, uint8_t reg_value)
{
    m_xfer_done = false;
    TxBuffer[0] = reg_address;
    TxBuffer[1] = reg_value;
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, BNO055_ADDRESS, TxBuffer, 2, false);
    APP_ERROR_CHECK(err_code);
    while(!m_xfer_done){}
}

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    //DEBUG("TWI HANDLE");

    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                //DEBUG("ACC NRF_DRV_TWI_EVT_DONE ");
            }
            m_xfer_done = true;
            break;

        case NRF_DRV_TWI_EVT_ADDRESS_NACK : 
        {
          DEBUG("ACC NACK :-(");
        }

        default:
            break;
    }
}

void drv_acc_init(t_cb cb)
{
    event_cb = cb;
    nrf_gpio_pin_clear(BNO_RESETn);
    vTaskDelay(2);
    nrf_gpio_pin_set(BNO_RESETn);
    vTaskDelay(800);

    ret_code_t err_code;
    const nrf_drv_twi_config_t twi_lm75b_config = {
       .scl                = I2C_SCL,
       .sda                = I2C_SDA,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = true,
       .hold_bus_uninit    = true
    };
    err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
    nrf_drv_twi_enable(&m_twi);
    DEBUG_LOG("acc init.. : %d", err_code); 
    //rtos_init();
    vTaskDelay(10);
    write_reg(BNO055_PWR_MODE_ADDR, POWER_MODE_NORMAL);
    vTaskDelay(10);
    write_reg(BNO055_SYS_TRIGGER_ADDR, 0x40);
    vTaskDelay(10);
    write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_NDOF);
    vTaskDelay(10);
}

void drv_acc_read (s_accValue* accValue)
{
    //todo apply matrixRotation :
    /*
    [ 1 0 0 ]
    [ 0 1 0 ]
    [ 0 0 1 ] 
    */
    m_xfer_done = false;
    TxBuffer[0] = 0x1A;
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, BNO055_ADDRESS, TxBuffer, 1, true);
    APP_ERROR_CHECK(err_code);
    while(!m_xfer_done){}
    m_xfer_done = false;
    err_code = nrf_drv_twi_rx(&m_twi, BNO055_ADDRESS, RxBuffer, 6);
    APP_ERROR_CHECK(err_code);
    uint8_t msb,lsb;
    int16_t val;
    lsb = RxBuffer[0];
    msb = RxBuffer[1];
    val = (msb<<8)+lsb;
    accValue->accHeading = ((float)val)/16.0;  //0..360°
    lsb = RxBuffer[2];
    msb = RxBuffer[3];
    val = (msb<<8)+lsb;
    accValue->accRoll = ((float)val)/16.0;  //-180..180°
    lsb = RxBuffer[4];
    msb = RxBuffer[5];
    val = (msb<<8)+lsb;
    accValue->accPitch = ((float)val)/16.0;  //-90..90
    while(!m_xfer_done){}
}

static void rtos_init (void)
{
    flagHandle  = xEventGroupCreate();
    if(NULL == flagHandle)
    {
        DEBUG("falg handle init err !");
    }
    BaseType_t xResult = xTaskCreate (     acc_task,
                                          "acc",
                                           BNO_TASK_STACK_SIZE,
                                           NULL,
                                           BNO_TASK_PRIORITY,
                                           &taskHandle);
    ASSERT(xResult == pdPASS);
}
void drv_acc_test(t_cb cb)
{
    uint8_t v = 0;
    while(1)
    {
        v = read_reg(0x02); //id = 50
        vTaskDelay(1000);
    }
}

void drv_acc_enable_stream(bool en)
{
    streamAccEnabled = en;
}

//todo optimiser l'archi
static void acc_task(void *pArg)
{
    DEBUG("*** Acc task ***");
    vTaskDelay(2000);
    while(1)
    {
        vTaskDelay(200);
        if(streamAccEnabled)
        {
            event_cb(e_accEventRxEvent, 0, 0); 
        }   
    
    }
}

