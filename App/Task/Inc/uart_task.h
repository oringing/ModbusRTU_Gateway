// App/Inc/uart_task.h
#ifndef __UART_TASK_H
#define __UART_TASK_H

#include "cmsis_os.h"

void UART_Task_RequestStop(void);
void Start_UART_Task(void const* argument);

#endif /* __UART_TASK_H */
