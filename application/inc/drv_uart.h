#ifndef DRV_UART_H
#define DRV_UART_H

#include <stdbool.h>



void drv_uart_init (void);
void drv_uart_send (uint8_t *pData, uint8_t size);

#endif
