// App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

// ---- 任务优先级配置（不可修改）----
#define UART_TASK_PRIORITY osPriorityHigh           // UART 任务：Modbus 响应要求快（<1ms）
#define DEVICE_TASK_PRIORITY osPriorityNormal       // Device 任务：LED + 传感器等无实时要求的外设
#define MONITOR_TASK_PRIORITY osPriorityBelowNormal // Monitor任务优先级（低于UART，后台监控）

// ---- 任务栈大小配置（实测数据（2026-05-18）：）----
#define UART_TASK_STACK_SIZE                                                                       \
    160U // UART任务栈(words)，实测峰值82words(高压78words剩余)，安全裕度51%
#define DEVICE_TASK_STACK_SIZE 160U // LED + 传感器 + 数据转换
#define MONITOR_TASK_STACK_SIZE 96U // Monitor任务栈(words)，实测峰值40words，安全裕度58%

// ---- 日志开关与配置（日志使用UART2，PA2 TX / PA3 RX） ----
#define SYSTEM_UART_TEXT_LOG_ENABLE 1U       // 系统测试/报错日志开关，1=启用，0=禁用
#define SYSTEM_STACK_WATERMARK_LOG_ENABLE 0U // 栈水位日志开关，1=启用，0=禁用
#define STACK_CHECK_INTERVAL_SEC 5U // 栈水位检查间隔(s)，定期检查任务栈使用情况（调试时可缩短）

// ---- 超时配置（性能调优）----
#define SYSTEM_TASK_STOP_TIMEOUT_MS 300U // 任务优雅退出等待超时(ms)

#endif /* SYSTEM_CONFIG_H */
