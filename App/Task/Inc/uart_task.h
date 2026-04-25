// App/Inc/uart_task.h
#ifndef __UART_TASK_H
#define __UART_TASK_H

#include "cmsis_os.h"

/* 移除任务句柄的外部声明，改为通过函数获取 */
/* extern osThreadId uart_task_handler; */

/* 提供获取任务句柄的函数接口 */
osThreadId GetUartTaskHandle(void);

void Start_UART_Task(void const * argument);

#endif /* __UART_TASK_H */
