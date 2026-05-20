// App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

// ---- 任务优先级配置（不可修改）----
#define LED_TASK_PRIORITY      osPriorityLow        // LED任务优先级（最低）
#define UART_TASK_PRIORITY     osPriorityNormal     // UART任务优先级（正常，需及时响应Modbus请求）
#define MONITOR_TASK_PRIORITY  osPriorityBelowNormal // Monitor任务优先级（低于正常，后台监控）

// ---- 任务栈大小配置（实测数据（2026-05-18）：）----
#define LED_TASK_STACK_SIZE    64U   // LED任务栈(words)，实测峰值28words，安全裕度56%
#define UART_TASK_STACK_SIZE   160U  // UART任务栈(words)，实测峰值82words(高压78words剩余)，安全裕度51%
#define MONITOR_TASK_STACK_SIZE 96U  // Monitor任务栈(words)，实测峰值40words，安全裕度58%

// ---- 栈水位日志开关 ----
#define SYSTEM_STACK_WATERMARK_LOG_ENABLE 0U  // 栈水位日志开关，1=启用，0=禁用（生产环境建议关闭）

// ---- 系统日志开关 ----
#define SYSTEM_UART_TEXT_LOG_ENABLE 1U  // UART文本日志总开关，1=启用所有UART日志，0=禁用（生产环境可关闭以节省带宽）

// ---- 超时配置（性能调优）----
#define SYSTEM_TASK_STOP_TIMEOUT_MS 300U // 任务优雅退出等待超时(ms)

#endif /* SYSTEM_CONFIG_H */
