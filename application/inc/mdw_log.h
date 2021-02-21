#ifndef MDW_LOG_H
#define MDW_LOG_H

#include <string.h>
#include <stdint.h>
#include "param_global.h"
#include "stdio.h"

#define STR_MSG_SIZE            18 //?
#define STR_FILENAME_SIZE       4 + 1

typedef struct __attribute__((packed, aligned(1)))
{
    uint64_t        tms;
    uint8_t         type;
    uint16_t        line;
    uint8_t         fileName[STR_FILENAME_SIZE];
    uint8_t         msg[STR_MSG_SIZE];
}s_error;

void mdw_log(uint8_t idx, char *pMsg, uint16_t lineNum, char *pFileName);

#define DEBUG(MSG)                                                              \
    do                                                                          \
    {                                                                           \
        mdw_log(0, MSG, (uint16_t)__LINE__, (char*)__FILE__);          \
    }while(0);

#define DEBUG_LOG(message, args...)                             \
    do{                                                         \
        char str[32];                                           \
        snprintf(str, sizeof(str), message, args);              \
        DEBUG(str)                                              \
    }while(0);

void    mdw_log_init            (void);

#endif
