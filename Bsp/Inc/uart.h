// Bsp/Inc/uart.h
#ifndef __UART_H__
#define __UART_H__

#include "stm32f1xx_hal.h"
#include <stdbool.h> 

/* ================= UART Hardware Constants ================= */
/* 
 * Note: Currently only USART1 is used for both Modbus RTU and debug logging.
 * USART2 (PA2/PA3) is reserved for future expansion if needed.
 */

/* USART1 Configuration (Modbus RTU + Debug Log) */
#define BSP_UART_BAUDRATE       (115200U)   /**< UART 波特率 */
#define BSP_UART_INSTANCE       USART1      /**< UART 实例（USART1） */
#define BSP_UART_GPIO_PORT      GPIOA       /**< UART GPIO 端口（PA9/PA10） */
#define BSP_UART1_TX_PIN        GPIO_PIN_9  /**< USART1 TX 引脚（PA9） */
#define BSP_UART1_RX_PIN        GPIO_PIN_10 /**< USART1 RX 引脚（PA10） */

/* USART2 Reserved Configuration (Future Expansion) */
// #define BSP_UART2_BAUDRATE    (115200U)
// #define BSP_UART2_INSTANCE    USART2
// #define BSP_UART2_GPIO_PORT   GPIOA
// #define BSP_UART2_TX_PIN      GPIO_PIN_2  // PA2
// #define BSP_UART2_RX_PIN      GPIO_PIN_3  // PA3

#define BSP_UART_RX_BUF_SIZE    (256U)      /**< UART 接收缓冲区大小 */
#define BSP_UART_TX_BUF_SIZE    (256U)      /**< UART 发送缓冲区大小 */
#define BSP_UART_TX_TIMEOUT     (5U)        /**< UART 发送超时 (ms) */

/* ================= UART Driver Configuration (BSP Layer) ================= */
/* 故障恢复配置 */
#define UART_SUCCESS_RX_RECOVER_COUNT    (5U)   /**< 连续成功接收多少次判定为故障恢复 */
#define UART_MAX_RECOVERY_RETRY          (10U)  /**< 故障恢复最大重试次数 */
#define UART_RECOVERY_RETRY_DELAY_MS     (10U)  /**< 每次重试之间的延时 (ms) */

/* 故障指示配置 */
#define UART_FAULT_BLINK_COUNT           (5U)   /**< 故障指示LED闪烁次数 */
#define UART_FAULT_BLINK_INTERVAL_MS     (100U) /**< 故障指示LED闪烁间隔 (ms) */

/* 日志限流配置 */
#define UART_LOG_THROTTLE_MS             (1000U) /**< 错误日志打印限流时间 (ms) */
#define UART_ERROR_LOG_THROTTLE_MS       (1000U) /**< 错误日志打印限流时间 (ms) - 兼容旧命名 */

/* 错误分级处理阈值 */
#define UART_ERROR_STREAK_RECOVER_TH     (10U)  /**< 总错误连续次数阈值（触发恢复） */
#define UART_FE_RECOVER_STREAK_TH        (3U)   /**< FE/NE 连续错误次数阈值（触发恢复） */
#define UART_PE_RECOVER_STREAK_TH        (5U)   /**< PE 连续错误次数阈值（触发恢复） */
#define UART_IRQ_REENTRY_RECOVER_TH      (100U) /**< IRQ 重入计数阈值（触发恢复） */

/* 函数声明 */
void BSP_UART_Init(void);
void BSP_UART_PrintString(const char *str);
bool BSP_UART_Send(const uint8_t *data, uint16_t len, uint32_t timeout);
bool BSP_UART_IsFrameReady(void);
uint16_t BSP_UART_ReadFrame(uint8_t *buffer, uint16_t max_len);
void BSP_UART_IRQHandler(void);

#endif /* __UART_H__ */
