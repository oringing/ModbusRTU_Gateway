// App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

// ---- 任务优先级配置（不可修改）----
#define UART_TASK_PRIORITY osPriorityHigh           // UART 任务：Modbus 响应要求快（<1ms）
#define DEVICE_TASK_PRIORITY osPriorityNormal       // Device 任务：LED + 传感器等无实时要求的外设
#define MONITOR_TASK_PRIORITY osPriorityBelowNormal // Monitor任务优先级（低于UART，后台监控）

// ---- 任务栈大小配置（实测UART1接收100帧/s峰值数据（2026-06-05）：）----
#define UART_TASK_STACK_SIZE   128U     // UART任务栈(words)实际58 words，裕度55% (高压下剩余70，使用58)
#define DEVICE_TASK_STACK_SIZE  240U     // Device任务栈(words)实际156 words，裕度35% (高压下剩余84，使用156)
#define MONITOR_TASK_STACK_SIZE 128U     // Monitor任务栈(words)实际78 words，裕度39% (高压下剩余50，使用78)

// ---- 日志开关与配置（日志使用UART2，PA2 TX / PA3 RX） ----
#define SYSTEM_UART_TEXT_LOG_ENABLE 1U       // 系统测试/报错日志开关，1=启用，0=禁用
#define SYSTEM_STACK_WATERMARK_LOG_ENABLE 0U // 栈水位日志开关，1=启用，0=禁用
#define STACK_CHECK_INTERVAL_SEC 3U // 栈水位检查间隔(s)，定期检查任务栈使用情况（调试时可缩短）

// ---- 超时配置（性能调优）----
#define SYSTEM_TASK_STOP_TIMEOUT_MS 300U // 任务退出等待超时(ms)

#endif /* SYSTEM_CONFIG_H */
