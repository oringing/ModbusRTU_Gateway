// App/Src/led_task.c
#include "led_task.h"
#include "led.h" // 使用 BSP 提供的接口
#include "cmsis_os.h"

void Start_LED_Task(void const * argument)
{
    for(;;)
    {
        BSP_LED_Toggle();
        osDelay(500);
    }
}

