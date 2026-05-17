// App/Src/task/uart_task.c
#include "uart_task.h"
#include "cmsis_os.h"
#include "modbus.h"
#include "uart.h"

static volatile uint8_t s_uart_task_stop = 0U; // 停止标志，任务内循环检查

void UART_Task_RequestStop(void) {
    s_uart_task_stop = 1U;
}

// UART任务主循环：处理Modbus协议，无帧就绪时才延时（避免轮询延迟）
void Start_UART_Task(void const* argument) {
    (void)argument;
    s_uart_task_stop = 0U;

    while (s_uart_task_stop == 0U) {
        Modbus_Process();
        // 帧已就绪时不延时，立即处理下一帧（优化背靠背请求场景）
        if (!BSP_UART_IsFrameReady()) {
            osDelay(1U);
        }
    }
    osThreadTerminate(NULL);
}
