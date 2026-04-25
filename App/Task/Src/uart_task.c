// App/Src/task/uart_task.c
#include "uart_task.h"
#include "uart.h" 
#include "modbus.h"
#include "cmsis_os.h"

/* 瀹氫箟浠诲姟鍙ユ焺鍙橀噺锛屾敼涓洪潤鎬侊紝鍙湪姝ゆ枃浠跺唴鍙 */
static osThreadId uart_task_handler;

/* 瀹炵幇鑾峰彇浠诲姟鍙ユ焺鐨勫嚱鏁?*/
osThreadId GetUartTaskHandle(void)
{
    return uart_task_handler;
}

void Start_UART_Task(void const * argument)
{
    uart_task_handler = osThreadGetId();

    while (1)
    {
        Modbus_Process();
        osDelay(1);
    }
}


