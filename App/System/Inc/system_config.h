//App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

/* 任务栈大小定义（单位：words） */
#define LED_TASK_STACK_SIZE              64
#define UART_TASK_STACK_SIZE             256
#define MONITOR_TASK_STACK_SIZE          256

/* 任务优先级定义 */
#define LED_TASK_PRIORITY                osPriorityLow
#define UART_TASK_PRIORITY               osPriorityHigh
#define MONITOR_TASK_PRIORITY            osPriorityBelowNormal

/* 系统监控相关 */
#define STACK_WATERMARK_CHECK_INTERVAL   50
#define STACK_WATERMARK_LOG_DELAY        100

#endif /* SYSTEM_CONFIG_H */
