#ifndef DW3_TYPES_H
#define DW3_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAIN_TASK_STACK_SIZE        (450) //Max use avec cb 1552    -> 450*4 = 1800
#define TRANSFER_TASK_STACK_SIZE    (300) //Max use avec cb 1472    -> 425*4 = 1700
#define BNO_TASK_STACK_SIZE         (250) //Max use avec cb 608     -> 250*4 = 1000
#define LOG_TASK_STACK_SIZE         (300) //Max use avec cb 990     -> 300*4 = 1200

#define MAIN_TASK_PRIORITY        (1)
#define TRANSFER_TASK_PRIORITY    (1)
#define BNO_TASK_PRIORITY         (2)
#define LOG_TASK_PRIORITY         (1)


#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof(x[0]))

typedef uint64_t    t_time;

typedef struct
{
    uint8_t     *pData;
    size_t      size;
}s_pData;

typedef struct
{
    uint32_t    id;
    t_time      ts_ms;
    void        *pData;
}s_queueMsg;

typedef struct
{
    uint8_t     adr;
    uint8_t     data;
}s_reg;

typedef struct
{
    uint32_t    id;
    t_time      ts_ms;
    s_pData     data;
}s_message;

typedef struct
{
    void        *pData;
    uint32_t    numData;
}s_dataPtr;

typedef void        (*t_cb)(uint32_t event, t_time ts_ms, void *pData);
typedef void        *t_handle;

#ifdef DWILEN3_TEST
#include "pc_port.h"
#endif

#endif
