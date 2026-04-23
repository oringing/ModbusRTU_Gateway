// Bsp/Inc/uart.h
#ifndef __UART_H__
#define __UART_H__

#include "stm32f1xx_hal.h"
#include "main.h" 
#include <stdbool.h> 

// --- 硬件配置宏定义 ---
#define BSP_UART_BAUDRATE       115200
#define BSP_UART_INSTANCE       USART1
#define BSP_UART_TX_PIN         GPIO_PIN_9
#define BSP_UART_RX_PIN         GPIO_PIN_10
#define BSP_UART_GPIO_PORT      GPIOA

#define BSP_UART_RX_BUF_SIZE    128
#define BSP_UART_TX_TIMEOUT     100

// 函数声明
void BSP_UART_Init(void);
void BSP_UART_PrintString(const char *str);
bool BSP_UART_Send(const uint8_t *data, uint16_t len, uint32_t timeout);
bool BSP_UART_IsFrameReady(void);
uint16_t BSP_UART_ReadFrame(uint8_t *buffer, uint16_t max_len);
// 新增：BSP 层提供的中断服务入口
void BSP_UART_IRQHandler(void);

#endif /* __UART_H__ */

