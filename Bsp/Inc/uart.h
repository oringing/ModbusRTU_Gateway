// Bsp/Inc/uart.h
#ifndef __UART_H__
#define __UART_H__

#include "stm32f1xx_hal.h"
#include <stdbool.h>

// ---- 硬件配置（不可修改）----
// 注意：目前USART1同时用于Modbus RTU通信和调试日志打印
// USART2(PA2/PA3)已预留，以便未来扩展使用
#define BSP_UART_BAUDRATE (115200U)  // UART波特率，需与主机PLC保持一致
#define BSP_UART_INSTANCE USART1     // UART实例（USART1）
#define BSP_UART_GPIO_PORT GPIOA     // UART GPIO端口（PA9/PA10）
#define BSP_UART1_TX_PIN GPIO_PIN_9  // USART1 TX引脚（PA9）
#define BSP_UART1_RX_PIN GPIO_PIN_10 // USART1 RX引脚（PA10）

// USART2预留配置（未来扩展）
// #define BSP_UART2_BAUDRATE    (115200U)
// #define BSP_UART2_INSTANCE    USART2
// #define BSP_UART2_GPIO_PORT   GPIOA
// #define BSP_UART2_TX_PIN      GPIO_PIN_2  // PA2
// #define BSP_UART2_RX_PIN      GPIO_PIN_3  // PA3

// ---- 缓冲区配置（可调整）----
#define BSP_UART_RX_BUF_SIZE (256U) // UART接收缓冲区大小(字节)，需≥Modbus最大帧长
#define BSP_UART_TX_BUF_SIZE (256U) // UART发送缓冲区大小(字节)
#define BSP_UART_TX_TIMEOUT (5U)    // UART发送超时(ms)，避免阻塞任务调度

// ---- 故障恢复配置（性能调优）----
#define UART_SUCCESS_RX_RECOVER_COUNT (5U) // 连续成功接收次数判定为故障恢复
#define UART_MAX_RECOVERY_RETRY (10U)      // 故障恢复最大重试次数
#define UART_RECOVERY_RETRY_DELAY_MS (10U) // 每次重试之间的延时(ms)

// ---- 故障指示配置（调试用）----
#define UART_FAULT_BLINK_COUNT (5U)         // 故障指示LED闪烁次数
#define UART_FAULT_BLINK_INTERVAL_MS (100U) // 故障指示LED闪烁间隔(ms)

// ---- 日志限流配置（防止刷屏）----
#define UART_LOG_THROTTLE_MS (1000U)       // 错误日志打印限流时间(ms)
#define UART_ERROR_LOG_THROTTLE_MS (1000U) // 错误日志打印限流时间(ms) - 兼容旧命名

// ---- 错误分级处理阈值（协议容错）----
#define UART_ERROR_STREAK_RECOVER_TH (10U) // 总错误连续次数阈值（触发恢复）
#define UART_FE_RECOVER_STREAK_TH (3U)     // FE/NE连续错误次数阈值（触发恢复）
#define UART_PE_RECOVER_STREAK_TH (5U)     // PE连续错误次数阈值（触发恢复）
#define UART_IRQ_REENTRY_RECOVER_TH (100U) // IRQ重入计数阈值（触发恢复）

/**
 * @brief   初始化UART1硬件及中断
 * @warning 需在FreeRTOS启动前调用，内部会开启NVIC中断
 */
void BSP_UART_Init(void);

/**
 * @brief   通过UART发送字符串（调试用）
 * @param   str 待发送的字符串指针，必须以'\0'结尾
 * @warning 同步阻塞发送，不建议在实时任务中频繁调用
 */
void BSP_UART_PrintString(const char* str);

/**
 * @brief   发送数据帧（Modbus响应使用）
 * @param   data 待发送数据指针
 * @param   len 数据长度(字节)，范围[1, BSP_UART_TX_BUF_SIZE]
 * @param   timeout 超时时间(ms)
 * @return  true=发送成功, false=超时或硬件错误
 */
bool BSP_UART_Send(const uint8_t* data, uint16_t len, uint32_t timeout);

/**
 * @brief   检查是否有完整帧就绪
 * @return  true=有完整帧待读取, false=无数据或正在接收
 * @warning 需在UART任务中周期性调用，建议间隔≤10ms
 */
bool BSP_UART_IsFrameReady(void);

/**
 * @brief   读取已就绪的完整帧
 * @param   buffer 输出缓冲区指针
 * @param   max_len 缓冲区最大容量(字节)
 * @return  实际读取的字节数，0表示无数据或参数错误
 * @warning 读取后自动清除帧就绪标志，确保原子性
 */
uint16_t BSP_UART_ReadFrame(uint8_t* buffer, uint16_t max_len);

/**
 * @brief   UART1中断服务入口（IDLE中断+RX/TX回调分发）
 * @warning 由stm32f1xx_it.c中的USART1_IRQHandler调用，禁止手动调用
 */
void BSP_UART_IRQHandler(void);

#endif /* __UART_H__ */
