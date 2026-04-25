// App/Src/task/uart_task.c
#include "uart_task.h"
#include "uart.h" 
#include "modbus.h"
#include "cmsis_os.h"

// --- UART task ---
static volatile uint8_t s_uart_task_stop = 0U;

void UART_Task_RequestStop(void)
{
    s_uart_task_stop = 1U;
}

void Start_UART_Task(void const * argument)
{
    (void)argument;
    s_uart_task_stop = 0U;

    while (s_uart_task_stop == 0U)
    {
        Modbus_Process();
        osDelay(1);
    }
    osThreadTerminate(NULL);
}


