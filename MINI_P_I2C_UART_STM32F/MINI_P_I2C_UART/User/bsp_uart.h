#ifndef _BSP_UART_H
#define _BSP_UART_H

#ifdef __cplusplus
 extern "C" {
#endif 

#define UART_PC        (&huart2)

#include "usart.h"
void BspUartInit(void);
void BspUartRxIdleCallback(UART_HandleTypeDef *huart);
void User_HAL_UART_IRQHandler(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif
#endif
