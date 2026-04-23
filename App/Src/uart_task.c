// App/Src/uart_task.c
#include "uart_task.h"
#include "uart.h" 
#include "modbus.h" // 引入协议层
#include "cmsis_os.h"
#include <stdio.h>

/* 定义任务句柄变量，改为静态，只在此文件内可见 */
static osThreadId uart_task_handler;

/* 实现获取任务句柄的函数 */
osThreadId GetUartTaskHandle(void)
{
    return uart_task_handler;
}

void Start_UART_Task(void const * argument)
{
    while (1)
    {
        Modbus_Process();
        osDelay(1);
    }
}

