// App/Src/task/monitor_task.c
/* Includes ------------------------------------------------------------------*/
#include "monitor_task.h"
#include "FreeRTOS.h"
#include "modbus.h"
#include "system_config.h"
#include "system_ctrl.h"
#include "task.h"

static volatile uint8_t s_monitor_task_stop = 0U; // 停止标志，任务内循环检查

// Monitor任务主循环：心跳更新+喂狗+栈水位监控（三合一后台任务）
void Start_Monitor_Task(void const* argument) {
    uint32_t last_heartbeat_tick = 0;
    uint32_t last_feed_tick = 0;
    uint32_t last_stack_check_tick = 0;

    // 初始化时间戳（使用系统滴答计数器）
    last_heartbeat_tick = osKernelSysTick();
    last_feed_tick = osKernelSysTick();
    last_stack_check_tick = osKernelSysTick();

    // 清除停止标志（任务启动时确保可运行）
    s_monitor_task_stop = 0U;

    for (;;) {
        // 检查停止标志，实现优雅退出
        if (s_monitor_task_stop != 0U) {
            break; // 跳出循环，执行清理逻辑
        }

        uint32_t current_tick = osKernelSysTick();

        // 1. 心跳逻辑：每秒翻转DEVICE_STATUS寄存器低4位（系统活动指示）
        if (current_tick - last_heartbeat_tick >= HEARTBEAT_INTERVAL_MS) {
            uint16_t current_val = 0;
            if (Modbus_ReadHoldingRegister(MODBUS_REG_ADDR_DEVICE_STATUS, &current_val)) {
                uint16_t new_val = current_val ^ 0x000FU;
                Modbus_InternalWriteRegister(MODBUS_REG_ADDR_DEVICE_STATUS, new_val);
            }
            last_heartbeat_tick = current_tick;
        }

        // 2. 喂狗操作：每500ms刷新IWDG计数器（超时2秒，安全裕度4倍）
        if (current_tick - last_feed_tick >= IWDG_FEED_INTERVAL_MS) {
            System_IWDG_Feed();
            last_feed_tick = current_tick;
        }

        // 3. 栈水位检查：每60秒打印一次任务栈使用情况（低于阈值时告警）
        if (current_tick - last_stack_check_tick >= (STACK_CHECK_INTERVAL_SEC * 1000)) {
            System_Check_Stack_Watermark();
            last_stack_check_tick = current_tick;
        }

        // 基础循环延时，保持任务活跃并让出CPU
        osDelay(MONITOR_TASK_BASE_DELAY);
    }

}

void Monitor_Task_RequestStop(void) {
    s_monitor_task_stop = 1U;
}
