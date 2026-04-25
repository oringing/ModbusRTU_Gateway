// App/Src/task/led_task.c
#include "led_task.h"
#include "led.h" // 浣跨敤 BSP 鎻愪緵鐨勬帴鍙?
#include "cmsis_os.h"

void Start_LED_Task(void const * argument)
{
    for(;;)
    {
        BSP_LED_Toggle();
        osDelay(500);
    }
}


