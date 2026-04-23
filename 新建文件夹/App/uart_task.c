// App/Src/uart_task.c
#include "uart_task.h"
#include "uart.h" 
#include "modbus.h" // 引入协议层
#include "cmsis_os.h"

void Start_UART_Task(void const * argument)
{
    // 初始化协议层
    Modbus_Init();
    BSP_UART_PrintString("UART Task Running\r\n");
    for(;;)
    {        
        // 核心循环：不断检查并处理 MODBUS 请求
        Modbus_Process();

        // 短暂延时，让出 CPU 给 LED 任务等其他低优先级任务
        osDelay(10); 
    }
    
}

