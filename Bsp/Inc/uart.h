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

// --- 串口配置 ---
#define BSP_UART_RX_BUF_SIZE    256U       // 串口接收缓冲区大小
#define BSP_UART_TX_TIMEOUT     5U         // 串口发送超时(ms)

// --- 错误日志配置 ---
#define UART_ERROR_LOG_THROTTLE_MS        1000U
#define UART_ERROR_STREAK_RECOVER_TH      3U
#define UART_IRQ_REENTRY_RECOVER_TH       8U
#define UART_MAX_RECOVERY_RETRY           10U   // 最大恢复重试次数，超限则判定硬件故障

// --- 错误分级处理阈值 (P1-004) ---
#define UART_FE_RECOVER_STREAK_TH         3U    // FE/NE 连续错误次数阈值（触发恢复）
#define UART_PE_RECOVER_STREAK_TH         5U    // PE 连续错误次数阈值（触发恢复）
#define UART_ERROR_CLASSIFY_FAULT_TH      10U   // 单类错误累计次数阈值（标记硬件故障）

// 函数声明
void BSP_UART_Init(void);
void BSP_UART_PrintString(const char *str);
bool BSP_UART_Send(const uint8_t *data, uint16_t len, uint32_t timeout);
bool BSP_UART_IsFrameReady(void);
uint16_t BSP_UART_ReadFrame(uint8_t *buffer, uint16_t max_len);
void BSP_UART_IRQHandler(void);

#endif /* __UART_H__ */
