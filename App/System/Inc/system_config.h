// App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

// ---- 任务优先级配置（不可修改）----
#define LED_TASK_PRIORITY      osPriorityLow        // LED任务优先级（最低）
#define UART_TASK_PRIORITY     osPriorityNormal     // UART任务优先级（正常，需及时响应Modbus请求）
#define MONITOR_TASK_PRIORITY  osPriorityBelowNormal // Monitor任务优先级（低于正常，后台监控）

// ---- 任务栈大小配置（基于实测水位+1.3倍安全裕度）----
#define LED_TASK_STACK_SIZE    64U   // LED任务栈(words)，空闲/高压均使用28words，剩余量≥56%
#define UART_TASK_STACK_SIZE   128U  // UART任务栈(words)，0.1s/帧高压峰值使用54words，剩余量≥57%
#define MONITOR_TASK_STACK_SIZE 128U // Monitor任务栈(words)，空闲/高压均使用66words，剩余量≥50%

// ---- 栈水位日志开关 ----
#define SYSTEM_STACK_WATERMARK_LOG_ENABLE 1U  // 栈水位日志开关，1=启用，0=禁用（生产环境建议关闭）

// ---- 系统日志开关 ----
#define SYSTEM_UART_TEXT_LOG_ENABLE 1U  // UART文本日志总开关，1=启用所有UART日志，0=禁用（生产环境可关闭以节省带宽）

// ---- 超时配置（性能调优）----
#define SYSTEM_TASK_STOP_TIMEOUT_MS 300U // 任务优雅退出等待超时(ms)

#endif /* SYSTEM_CONFIG_H */
