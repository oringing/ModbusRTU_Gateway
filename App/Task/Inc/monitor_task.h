//App/Task/Inc/monitor_task.h
#ifndef __MONITOR_TASK_H__
#define __MONITOR_TASK_H__

/* 系统监控相关 */
#define MONITOR_TASK_BASE_DELAY        10U // 监控任务基础循环周期 (单位: ms)
#define HEARTBEAT_INTERVAL_MS          1000U // 心跳翻转间隔 (单位: ms)
#define STACK_CHECK_INTERVAL_SEC       60U // 栈水位检查间隔 (单位: s)
#define IWDG_FEED_INTERVAL_MS          500U // 看门狗喂狗间隔 (单位: ms)

/* 栈水位预警阈值（单位：words，低于该值触发告警） */
#define LED_STACK_WM_WARN_WORDS          24U
#define UART_STACK_WM_WARN_WORDS         48U
#define MONITOR_STACK_WM_WARN_WORDS      96U

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* Exported functions prototypes ---------------------------------------------*/
void Start_Monitor_Task(void const * argument);
void Monitor_Task_RequestStop(void);
void Check_Stack_Watermark(void);

#ifdef __cplusplus
}
#endif

#endif /* __MONITOR_TASK_H__ */
