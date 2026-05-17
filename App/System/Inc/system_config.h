//App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

/* 任务优先级定义 */
#define LED_TASK_PRIORITY                osPriorityLow
#define UART_TASK_PRIORITY               osPriorityNormal
#define MONITOR_TASK_PRIORITY            osPriorityBelowNormal

/* 任务栈大小定义（单位：words） */
#define LED_TASK_STACK_SIZE              64   /* 空闲/高压均使用28words，预留1.3倍剩余量 */
#define UART_TASK_STACK_SIZE             125   /* 0.1s/帧高压峰值使用54words，预留1.3倍剩余量 */
#define MONITOR_TASK_STACK_SIZE          165  /* 空闲/高压均使用74words，预留1.3倍剩余量 */

/* 任务栈水位、看门狗初始化信息打印开关 */
#define SYSTEM_UART_TEXT_LOG_ENABLE      0U    

/* 任务退出等待超时（ms） */
#define SYSTEM_TASK_STOP_TIMEOUT_MS      300U

#endif /* SYSTEM_CONFIG_H */
