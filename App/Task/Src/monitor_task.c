// App/Src/task/monitor_task.c
/* Includes ------------------------------------------------------------------*/
#include "monitor_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "system_ctrl.h"
#include "system_config.h"

static volatile uint8_t s_monitor_task_stop = 0U;

void Monitor_Task_RequestStop(void)
{
    s_monitor_task_stop = 1U;
}

/**
  * @brief  Monitor task function
  * @param  argument: pointer that is passed to the function as start argument
  * @retval None
  */
void Start_Monitor_Task(void const * argument)
{
    (void)argument;
    uint32_t check_counter = 0;

    s_monitor_task_stop = 0U;
    
    while (s_monitor_task_stop == 0U)
    {
        check_counter++;
        if (check_counter >= STACK_WATERMARK_CHECK_INTERVAL) {
            System_Check_Stack_Watermark();
            check_counter = 0;
        }
        
        // 喂狗操作 - 确保系统正常运行
        System_IWDG_Feed();
        
        osDelay(STACK_WATERMARK_LOG_DELAY);
    }
    osThreadTerminate(NULL);
}

