//App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

/* 任务栈大小定义（单位：words） */
#define LED_TASK_STACK_SIZE              64   /* 实测使用26words */
#define UART_TASK_STACK_SIZE             128  /* 实测使用48words */
#define MONITOR_TASK_STACK_SIZE          240  /* 实测使用72words*/

/* 任务优先级定义 */
#define LED_TASK_PRIORITY                osPriorityLow
#define UART_TASK_PRIORITY               osPriorityNormal
#define MONITOR_TASK_PRIORITY            osPriorityBelowNormal

/* 系统监控相关 */
#define STACK_WATERMARK_CHECK_INTERVAL   300U // 300 * 100ms = 30s
#define STACK_WATERMARK_LOG_DELAY        100U

/* 栈水位预警阈值（单位：words，低于该值触发告警） */
#define LED_STACK_WM_WARN_WORDS          24U
#define UART_STACK_WM_WARN_WORDS         48U
#define MONITOR_STACK_WM_WARN_WORDS      96U

/* 任务优雅退出等待超时（ms） */
#define SYSTEM_TASK_STOP_TIMEOUT_MS      300U

#endif /* SYSTEM_CONFIG_H */
