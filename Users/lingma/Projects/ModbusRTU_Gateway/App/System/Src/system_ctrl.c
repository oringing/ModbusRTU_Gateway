//App/System/Src/system_ctrl.c

#include "system_ctrl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "stdio.h"
#include "driver_uart.h"
#include "led_task.h"
#include "uart_task.h"
#include "monitor_task.h"
#include "system_config.h"
#include "modbus.h"
#include "uart.h"
#include "error_handler.h"
#include "stm32f1xx_hal_iwdg.h"

/* Private variables ---------------------------------------------------------*/
static IWDG_HandleTypeDef hiwdg;  // 全局IWDG句柄
static osThreadId s_led_task_handle = NULL;
static osThreadId s_uart_task_handle = NULL;
static osThreadId s_monitor_task_handle = NULL;
static SystemStatus_t s_last_error = SYSTEM_OK;
static char s_stack_log_line[80];
static bool s_led_stack_warn_active = false;
static bool s_uart_stack_warn_active = false;
static bool s_monitor_stack_warn_active = false;


/**
  * @brief  Initialize IWDG (Independent Watchdog)
  * @param  None
  * @retval None
  */
static void System_IWDG_Init(void)
{
#if (SYSTEM_USE_IWDG == 1U)
    // 使用全局的hiwdg句柄，而不是重新声明局部变量
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;  // 预分频系数256，假设LSI 40KHz，则计数频率约156Hz
    hiwdg.Init.Reload = IWDG_RELOAD_VALUE;      // 重装载值，约2秒超时

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        // 看门狗初始化失败，记录错误
        ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__);
    }
    else
    {
        // 看门狗初始化成功
        System_Monitor_Log("IWDG initialized successfully\r\n");
    }
#else
    // 看门狗未启用
    System_Monitor_Log("IWDG disabled by configuration\r\n");
#endif
}

/**
  * @brief  Feed IWDG (Refresh Independent Watchdog)
  * @param  None
  * @retval None
  */
void System_IWDG_Feed(void)
{
#if (SYSTEM_USE_IWDG == 1U)
    HAL_IWDG_Refresh(&hiwdg);
#endif
}
