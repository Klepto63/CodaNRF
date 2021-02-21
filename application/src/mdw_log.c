#include "mdw_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log.h"
#include "sdk_config.h"
#include "SEGGER_RTT.h"

#define BUFFER_SIZE_NUM         30
#define STRLOG_SIZE             64
#define STR_TIME_SIZE           10
#define STR_LINE_SIZE           4
#define NUM_MAX_CALLBACK        5
#define TX_BUFFER_SIZE          128
#define CHANNEL_NAME_SIZE       5

typedef struct {
    struct {
        t_cb    cb;
        char    cde;
    }handler[NUM_MAX_CALLBACK];
    int idx;
}s_callback;

typedef struct {
    char        str[STRLOG_SIZE];
}s_logBuffer;

typedef struct {
    uint8_t buffer[TX_BUFFER_SIZE];
    char    name[CHANNEL_NAME_SIZE];
}s_channel;

static              TaskHandle_t    taskHandle;
static volatile     uint8_t         idx1 = 0;
static volatile     uint8_t         idx2 = 0;
static              s_logBuffer     strLogBuffer[BUFFER_SIZE_NUM];
static              s_callback      callbackList = {0};

static void log_task    (void *pArg);
static void read_task   (void *pArg);
static void str_log     (uint8_t type, char *pMsg, uint16_t lineNum, char *pFileName, uint32_t ts_s, char *pStrLog);

void mdw_log_init(void)
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    ASSERT(NRF_SUCCESS == ret);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    BaseType_t xResult = xTaskCreate(   log_task,
                                        "LOG",
                                        LOG_TASK_STACK_SIZE,
                                        NULL,
                                        LOG_TASK_PRIORITY,
                                        &taskHandle);
    ASSERT(xResult == pdPASS);
}

void mdw_log(uint8_t type, char *pMsg, uint16_t lineNum, char *pFileName)
{
    if(idx1 != ((idx2 + 1) % BUFFER_SIZE_NUM))
    {
        CRITICAL_REGION_ENTER();
        char strChain[STRLOG_SIZE] = {0};
        uint32_t ts_s = 0;
        str_log(type, pMsg,lineNum, pFileName, ts_s,strChain);
        strncpy(strLogBuffer[idx2].str, strChain, sizeof(strLogBuffer[idx2].str));
        idx2 = (idx2 + 1) % BUFFER_SIZE_NUM;
        CRITICAL_REGION_EXIT();
    }
}

static void str_log(uint8_t type, char *pMsg, uint16_t lineNum, char *pFileName, uint32_t ts_s, char *pStrLog)
{
    char strLog[STRLOG_SIZE]= {0};
    char strTime[STR_TIME_SIZE] = {0};
    char strlineNum[STR_LINE_SIZE] = {0};
    static char strFileName[STR_FILENAME_SIZE];
    char strType;
    sprintf(strTime,"%d",ts_s);
    uint8_t nbPadd = (uint8_t)(STR_TIME_SIZE - strlen(strTime));
    for(int i = 0; i <= nbPadd ; i++)
    {
        sprintf(strLog + i,"%d",0);
    }

    snprintf(strLog + nbPadd,strlen(strTime)+1,"%u",ts_s);
    sprintf(strlineNum,"%d",lineNum);
    nbPadd = (uint8_t)(STR_LINE_SIZE - strlen(strlineNum));
    for(int i = 0; i <= nbPadd ; i++)
    {
        sprintf(strlineNum + i,"%d",0);
    }

    snprintf(strlineNum + nbPadd,STR_LINE_SIZE - nbPadd + 1,"%u",lineNum);
    if(0 != pFileName)
    {
        snprintf(strFileName, STR_FILENAME_SIZE, "%s", strrchr(pFileName,'\\') + 5);
    }

    snprintf(strLog + STR_TIME_SIZE, STRLOG_SIZE-STR_TIME_SIZE, "[%s_%s]:%s",strFileName,strlineNum,pMsg);
    memcpy(pStrLog,strLog,STRLOG_SIZE);
}

static void log_task(void *pArg)
{
    while(1)
    {
        if(idx1 != idx2)
        {
            while(idx1 != idx2)
            {
                NRF_LOG_INFO("%s",strLogBuffer[idx1].str);
                NRF_LOG_FLUSH();
                vTaskDelay(10);
                idx1 = (idx1 + 1) % BUFFER_SIZE_NUM;
            }
        }  
        NRF_LOG_FLUSH();
        vTaskDelay(10);
    }
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_WARNING("System reset");
    NRF_LOG_FINAL_FLUSH();
    NVIC_SystemReset();
}
