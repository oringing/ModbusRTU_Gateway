// App/Src/task/led_task.c
#include "led_task.h"
#include "cmsis_os.h"
#include "led.h"

static volatile uint8_t s_led_task_stop = 0U; // 停止标志，任务内循环检查

void LED_Task_RequestStop(void) {
    s_led_task_stop = 1U;
}

// LED任务主循环：周期性翻转LED，收到停止标志后优雅退出
void Start_LED_Task(void const* argument) {
    (void)argument;
    s_led_task_stop = 0U;
    while (s_led_task_stop == 0U) {
        BSP_LED_Toggle();
        osDelay(LED_TASK_TOGGLE_INTERVAL_MS);
    }
    BSP_LED_Off(); // 退出前关闭LED
    osThreadTerminate(NULL);
}
