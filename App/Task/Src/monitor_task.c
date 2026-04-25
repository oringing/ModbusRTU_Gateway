// App/Src/task/monitor_task.c
/* Includes ------------------------------------------------------------------*/
#include "monitor_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "system_ctrl.h"
#include "system_config.h"

/**
  * @brief  Monitor task function
  * @param  argument: pointer that is passed to the function as start argument
  * @retval None
  */
void Start_Monitor_Task(void const * argument)
{
    uint32_t check_counter = 0;
    
    for(;;)
    {
        check_counter++;
        if(check_counter >= STACK_WATERMARK_CHECK_INTERVAL) {  // 姣忛殧5绉掓娴嬩竴娆★紙鍋囪osDelay(100)锛?
            System_Check_Stack_Watermark();  // 璋冪敤绯荤粺缁熶竴鐨勭洃鎺ф帴鍙?
            check_counter = 0;
        }
        
        osDelay(STACK_WATERMARK_LOG_DELAY);  // 寤惰繜100ms
    }
}

