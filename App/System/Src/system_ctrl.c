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

/* Private variables ---------------------------------------------------------*/
static osThreadId s_led_task_handle = NULL;
static osThreadId s_uart_task_handle = NULL;
static osThreadId s_monitor_task_handle = NULL;
static SystemStatus_t s_last_error = SYSTEM_OK;

/* Private function prototypes -----------------------------------------------*/
static void System_Monitor_Log(const char *msg);

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
        s_uart_task_handle = System_CreateTask("uart_task",
                                               Start_UART_Task,
                                               UART_TASK_PRIORITY,
                                               UART_TASK_STACK_SIZE);
        if (s_uart_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_UART;
            System_Monitor_Log("ERR: create uart_task failed\r\n");
            return s_last_error;
        }
    }
    if (s_monitor_task_handle == NULL) {
        s_monitor_task_handle = System_CreateTask("monitor_task",
                                                  Start_Monitor_Task,
                                                  MONITOR_TASK_PRIORITY,
                                                  MONITOR_TASK_STACK_SIZE);
        if (s_monitor_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_MONITOR;
            System_Monitor_Log("ERR: create monitor_task failed\r\n");
            return s_last_error;
        }
    }
    s_last_error = SYSTEM_OK;
    return s_last_error;
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

void System_RegisterTaskHandles(osThreadId ledTask, osThreadId uartTask, osThreadId monitorTask)
{
    if ((ledTask == NULL) || (uartTask == NULL) || (monitorTask == NULL)) {
        s_last_error = SYSTEM_ERR_NULL_PTR;
        return;
    }
    s_led_task_handle = ledTask;
    s_uart_task_handle = uartTask;
    s_monitor_task_handle = monitorTask;
}

void System_StartMonitor(void)
{
    if (s_monitor_task_handle == NULL) {
        s_monitor_task_handle = System_CreateTask("monitor_task",
                                                  Start_Monitor_Task,
                                                  MONITOR_TASK_PRIORITY,
                                                  MONITOR_TASK_STACK_SIZE * 4U);
    }
}

void System_CheckStackWatermark(void)
{
    TaskStatus_t task_status;
    uint32_t stack_high_watermark;
    char log_buf[256];
    int used = 0;

    if (s_uart_task_handle != NULL) {
        vTaskGetInfo(s_uart_task_handle, &task_status, pdTRUE, eRunning);
        stack_high_watermark = task_status.usStackHighWaterMark;
        used += snprintf(log_buf + used, sizeof(log_buf) - (size_t)used,
                         "UART Stack Watermark: %lu words\r\n", stack_high_watermark);
    } else {
        used += snprintf(log_buf + used, sizeof(log_buf) - (size_t)used,
                         "UART Stack Watermark: invalid handle\r\n");
    }

    if (s_led_task_handle != NULL) {
        vTaskGetInfo(s_led_task_handle, &task_status, pdTRUE, eRunning);
        stack_high_watermark = task_status.usStackHighWaterMark;
        used += snprintf(log_buf + used, sizeof(log_buf) - (size_t)used,
                         "LED Stack Watermark: %lu words\r\n", stack_high_watermark);
    } else {
        used += snprintf(log_buf + used, sizeof(log_buf) - (size_t)used,
                         "LED Stack Watermark: invalid handle\r\n");
    }
    
    if (s_monitor_task_handle != NULL) {
        vTaskGetInfo(s_monitor_task_handle, &task_status, pdTRUE, eRunning);
        stack_high_watermark = task_status.usStackHighWaterMark;
        used += snprintf(log_buf + used, sizeof(log_buf) - (size_t)used,
                         "Monitor Stack Watermark: %lu words\r\n", stack_high_watermark);
    } else {
        used += snprintf(log_buf + used, sizeof(log_buf) - (size_t)used,
                         "Monitor Stack Watermark: invalid handle\r\n");
    }

    if (used > 0) {
        System_Monitor_Log(log_buf);
    }
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

