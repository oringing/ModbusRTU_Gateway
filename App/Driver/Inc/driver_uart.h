//App/Driver/Inc/driver_uart.h
#ifndef __DRIVER_UART_H__
#define __DRIVER_UART_H__

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Driver 层不需要额外的配置，所有 UART 配置已在 uart.h (BSP 层) 中定义 */

/* 函数声明 */
bool UART_Driver_Init(void);
bool UART_Driver_Send(const uint8_t *data, uint16_t len, uint32_t timeout);
uint16_t UART_Driver_Receive(uint8_t *buffer, uint16_t max_len, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_UART_H__ */
