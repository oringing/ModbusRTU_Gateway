// App/Src/task/monitor_task.c
/* Includes ------------------------------------------------------------------*/
#include "monitor_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "system_ctrl.h"
#include "system_config.h"
#include "modbus.h"

static volatile uint8_t s_monitor_task_stop = 0U;

void Start_Monitor_Task(void const * argument)
{
    /* USER CODE BEGIN Monitor_Task */
    uint32_t last_heartbeat_tick = 0;
    uint32_t last_feed_tick = 0;
    uint32_t last_stack_check_tick = 0;

    /* 初始化时间戳 */
    last_heartbeat_tick = osKernelSysTick();
    last_feed_tick = osKernelSysTick();
    last_stack_check_tick = osKernelSysTick();

    /* Infinite loop */
    for (;;)
    {
        uint32_t current_tick = osKernelSysTick();

        /* 1. 心跳逻辑 (每秒翻转一次低 4 位) */
        if (current_tick - last_heartbeat_tick >= HEARTBEAT_INTERVAL_MS) {
            uint16_t current_val = 0;
            if (Modbus_ReadHoldingRegister(MODBUS_REG_ADDR_DEVICE_STATUS, &current_val)) {
                uint16_t new_val = current_val ^ 0x000FU;
                Modbus_InternalWriteRegister(MODBUS_REG_ADDR_DEVICE_STATUS, new_val);
            }
            last_heartbeat_tick = current_tick;
        }

        /* 2. 喂狗操作 (每 500ms 喂一次) */
        if (current_tick - last_feed_tick >= IWDG_FEED_INTERVAL_MS) {
            System_IWDG_Feed();
            last_feed_tick = current_tick;
        }

        /* 3. 栈水位检查 (每 60s 检查一次) */
        if (current_tick - last_stack_check_tick >= (STACK_CHECK_INTERVAL_SEC * 1000)) {
            System_Check_Stack_Watermark();
            last_stack_check_tick = current_tick;
        }

        /* 基础循环延时，保持任务活跃并让出 CPU */
        osDelay(MONITOR_TASK_BASE_DELAY);
    }
    /* USER CODE END Monitor_Task */
}

void Monitor_Task_RequestStop(void)
{
    s_monitor_task_stop = 1U;
}
