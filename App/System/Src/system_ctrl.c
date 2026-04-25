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

/* Private variables ---------------------------------------------------------*/
static osThreadId s_led_task_handle = NULL;
static osThreadId s_uart_task_handle = NULL;
static osThreadId s_monitor_task_handle = NULL;
static SystemStatus_t s_last_error = SYSTEM_OK;
static char s_stack_log_line[80];

/* Private function prototypes -----------------------------------------------*/
static void System_Monitor_Log(const char *msg);
static void System_StopTaskIfRunning(osThreadId *task_handle);
static bool System_ValidateConfig(void);
static bool System_WaitTaskDeleted(osThreadId task_handle, uint32_t timeout_ms);
static void System_LogTaskStackWatermark(const char *task_name, osThreadId task_handle);

/**
  * @brief  Initialize system control module
  * @param  None
  * @retval None
  */
void System_Init(void)
{
    (void)UART_Driver_Init();
}

void System_Ctrl_Init(void)
{
    if (!System_ValidateConfig()) {
        s_last_error = SYSTEM_ERR_CONFIG_INVALID;
        System_Monitor_Log("ERR: invalid system config\r\n");
        ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__);
        Error_Handler();
        return;
    }
    System_Init();
}

SystemStatus_t System_StartTasks(void)
{
    if (s_led_task_handle == NULL) {
        s_led_task_handle = System_CreateTask("LED_Task",
                                              Start_LED_Task,
                                              LED_TASK_PRIORITY,
                                              LED_TASK_STACK_SIZE);
        if (s_led_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_LED;
            System_Monitor_Log("ERR: create LED_Task failed\r\n");
            return s_last_error;
        }
    }
    if (s_uart_task_handle == NULL) {
        s_uart_task_handle = System_CreateTask("UART_Task",
                                               Start_UART_Task,
                                               UART_TASK_PRIORITY,
                                               UART_TASK_STACK_SIZE);
        if (s_uart_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_UART;
            System_Monitor_Log("ERR: create UART_Task failed, rollback\r\n");
            System_StopTasks();
            return s_last_error;
        }
    }
    if (s_monitor_task_handle == NULL) {
        s_monitor_task_handle = System_CreateTask("Monitor_Task",
                                                  Start_Monitor_Task,
                                                  MONITOR_TASK_PRIORITY,
                                                  MONITOR_TASK_STACK_SIZE);
        if (s_monitor_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_MONITOR;
            System_Monitor_Log("ERR: create Monitor_Task failed, rollback\r\n");
            System_StopTasks();
            return s_last_error;
        }
    }
    s_last_error = SYSTEM_OK;
    return s_last_error;
}

void System_StopTasks(void)
{
    System_StopTaskIfRunning(&s_monitor_task_handle);
    System_StopTaskIfRunning(&s_uart_task_handle);
    System_StopTaskIfRunning(&s_led_task_handle);
}

SystemStatus_t System_GetLastError(void)
{
    return s_last_error;
}

osThreadId System_CreateTask(const char *name,
                             void (*taskFunc)(void const *argument),
                             osPriority priority,
                             uint32_t stackSize)
{
    if (taskFunc == NULL) {
        return NULL;
    }

    osThreadDef_t thread_def = {
        .name = (char *)name,
        .pthread = taskFunc,
        .tpriority = priority,
        .instances = 1U,
        .stacksize = stackSize
    };

    return osThreadCreate(&thread_def, NULL);
}

void System_DestroyTask(osThreadId taskID)
{
    if (taskID != NULL) {
        osThreadTerminate(taskID);
    }
}

uint32_t System_GetStackWatermark(osThreadId taskID)
{
    if (taskID == NULL) {
        return 0U;
    }
    return (uint32_t)uxTaskGetStackHighWaterMark(taskID);
}

void System_CheckStackWatermark(void)
{
    System_LogTaskStackWatermark("UART", s_uart_task_handle);
    System_LogTaskStackWatermark("LED", s_led_task_handle);
    System_LogTaskStackWatermark("Monitor", s_monitor_task_handle);
}

void System_Check_Stack_Watermark(void)
{
    System_CheckStackWatermark();
}

/**
  * @brief  Log message via UART
  * @param  msg: message string to send
  * @retval None
  */
static void System_Monitor_Log(const char *msg)
{
    if (msg == NULL) {
        return;
    }
    (void)UART_Driver_Send((const uint8_t *)msg, (uint16_t)strlen(msg), 20U);
}

static void System_StopTaskIfRunning(osThreadId *task_handle)
{
    if (task_handle == NULL || *task_handle == NULL) {
        return;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (task_handle == &s_led_task_handle) {
            LED_Task_RequestStop();
        } else if (task_handle == &s_uart_task_handle) {
            UART_Task_RequestStop();
        } else if (task_handle == &s_monitor_task_handle) {
            Monitor_Task_RequestStop();
        }

        if (!System_WaitTaskDeleted(*task_handle, SYSTEM_TASK_STOP_TIMEOUT_MS)) {
            System_Monitor_Log("WARN: graceful stop timeout, force terminate\r\n");
            System_DestroyTask(*task_handle);
        }
    } else {
        System_DestroyTask(*task_handle);
    }

    *task_handle = NULL;
}

static bool System_ValidateConfig(void)
{
    if (LED_TASK_STACK_SIZE < 48U) {
        System_Monitor_Log("CFG FAIL: LED_TASK_STACK_SIZE < 48\r\n");
        return false;
    }
    if (UART_TASK_STACK_SIZE < 96U) {
        System_Monitor_Log("CFG FAIL: UART_TASK_STACK_SIZE < 96\r\n");
        return false;
    }
    if (MONITOR_TASK_STACK_SIZE < 192U) {
        System_Monitor_Log("CFG FAIL: MONITOR_TASK_STACK_SIZE < 192\r\n");
        return false;
    }
    if (MONITOR_TASK_STACK_SIZE < UART_TASK_STACK_SIZE) {
        System_Monitor_Log("CFG FAIL: MONITOR_TASK_STACK_SIZE < UART_TASK_STACK_SIZE\r\n");
        return false;
    }
    if (BSP_UART_RX_BUF_SIZE < 256U) {
        System_Monitor_Log("CFG FAIL: BSP_UART_RX_BUF_SIZE < 256\r\n");
        return false;
    }
    if (MODBUS_BUFFER_SIZE < 256U) {
        System_Monitor_Log("CFG FAIL: MODBUS_BUFFER_SIZE < 256\r\n");
        return false;
    }
    
#pragma diag_suppress 111  // 抑制"语句不可达"警告
    // 此检查用于验证配置的有效性，即使当前配置不为0，也应该保留此检查
    // 以防止未来修改配置时出现错误（防御性编程）
    if (BSP_UART_TX_TIMEOUT == 0U) {
        System_Monitor_Log("CFG FAIL: BSP_UART_TX_TIMEOUT == 0\r\n");
        return false;
    }
#pragma diag_default 111   // 恢复警告
    
    if (SYSTEM_TASK_STOP_TIMEOUT_MS < 100U) {
        System_Monitor_Log("CFG FAIL: SYSTEM_TASK_STOP_TIMEOUT_MS < 100\r\n");
        return false;
    }
    return true;
}

static bool System_WaitTaskDeleted(osThreadId task_handle, uint32_t timeout_ms)
{
#if (INCLUDE_eTaskGetState == 1)
    uint32_t elapsed = 0U;
    while (elapsed < timeout_ms) {
        eTaskState state = eTaskGetState((TaskHandle_t)task_handle);
        if (state == eDeleted || state == eInvalid) {
            return true;
        }
        osDelay(1U);
        elapsed++;
    }
    return false;
#else
    /* If task state inspection is not available, just wait for the specified timeout.
     * The caller will force terminate if this returns false. */
    (void)task_handle;
    osDelay(timeout_ms);
    return false;
#endif
}

static void System_LogTaskStackWatermark(const char *task_name, osThreadId task_handle)
{
    if (task_name == NULL) {
        return;
    }

    if (task_handle == NULL) {
        (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                       "%s Stack Watermark: invalid handle\r\n", task_name);
        System_Monitor_Log(s_stack_log_line);
        return;
    }

    /* Use %u for uint32_t on most embedded ARM GCC targets to avoid format warnings */
    (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                   "%s Stack Watermark: %u words\r\n",
                   task_name,
                   (unsigned int)uxTaskGetStackHighWaterMark(task_handle));
    System_Monitor_Log(s_stack_log_line);
}
