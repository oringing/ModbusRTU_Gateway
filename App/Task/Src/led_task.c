// App/Src/task/led_task.c
#include "led_task.h"
#include "led.h" // 浣跨敤 BSP 鎻愪緵鐨勬帴鍙?
#include "cmsis_os.h"

static volatile uint8_t s_led_task_stop = 0U;

void LED_Task_RequestStop(void)
{
    s_led_task_stop = 1U;
}

void Start_LED_Task(void const * argument)
{
    (void)argument;
    s_led_task_stop = 0U;
    while (s_led_task_stop == 0U)
    {
        BSP_LED_Toggle();
        osDelay(LED_TASK_TOGGLE_INTERVAL_MS);
    }
    BSP_LED_Off();
    osThreadTerminate(NULL);
}


