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
#if (SYSTEM_USE_IWDG == 1U)
static IWDG_HandleTypeDef hiwdg;  // 全局IWDG句柄
#endif

static osThreadId s_led_task_handle = NULL;
static osThreadId s_uart_task_handle = NULL;
static osThreadId s_monitor_task_handle = NULL;
static SystemStatus_t s_last_error = SYSTEM_OK;
static char s_stack_log_line[SYSTEM_STACK_LOG_BUF_SIZE];
static bool s_led_stack_warn_active = false;
static bool s_uart_stack_warn_active = false;
static bool s_monitor_stack_warn_active = false;

/* Private function prototypes -----------------------------------------------*/
static void System_Monitor_Log(const char *msg);
static void System_StopTaskIfRunning(osThreadId *task_handle);
static bool System_ValidateConfig(void);
static bool System_WaitTaskDeleted(osThreadId task_handle, uint32_t timeout_ms);
static void System_LogTaskStackWatermark(const char *task_name, osThreadId task_handle);
static uint32_t System_GetTaskStackWarnThreshold(const char *task_name);
static bool *System_GetTaskStackWarnFlag(const char *task_name);
static osThreadId System_GetTaskHandleById(SystemTaskId_t task_id);
static osPriority System_GetDefaultPriorityById(SystemTaskId_t task_id);
static void System_IWDG_Init(void);

/**
  * @brief  Initialize system control module
  * @param  None
  * @retval None
  */
void System_Init(void)
{
    // 初始化看门狗
    System_IWDG_Init();
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
    if (name == NULL || taskFunc == NULL || stackSize == 0U || priority == osPriorityError) {
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

bool System_SetTaskPriority(SystemTaskId_t task_id, osPriority priority)
{
    osThreadId task_handle = System_GetTaskHandleById(task_id);
    if (task_handle == NULL) {
        return false;
    }
    return (osThreadSetPriority(task_handle, priority) == osOK);
}

osPriority System_GetTaskPriority(SystemTaskId_t task_id)
{
    osThreadId task_handle = System_GetTaskHandleById(task_id);
    if (task_handle == NULL) {
        return osPriorityError;
    }
    return osThreadGetPriority(task_handle);
}

void System_ResetTaskPriorities(void)
{
    (void)System_SetTaskPriority(SYSTEM_TASK_LED, System_GetDefaultPriorityById(SYSTEM_TASK_LED));
    (void)System_SetTaskPriority(SYSTEM_TASK_UART, System_GetDefaultPriorityById(SYSTEM_TASK_UART));
    (void)System_SetTaskPriority(SYSTEM_TASK_MONITOR, System_GetDefaultPriorityById(SYSTEM_TASK_MONITOR));
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

#if (SYSTEM_UART_TEXT_LOG_ENABLE == 1U)
    (void)UART_Driver_Send((const uint8_t *)msg, (uint16_t)strlen(msg), 20U);
#else
    (void)msg;
#endif
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
    if (LED_TASK_STACK_SIZE < LED_TASK_STACK_MIN_WORDS) {
        System_Monitor_Log("CFG FAIL: LED_TASK_STACK_SIZE < LED_TASK_STACK_MIN_WORDS\r\n");
        return false;
    }
    if (UART_TASK_STACK_SIZE < UART_TASK_STACK_MIN_WORDS) {
        System_Monitor_Log("CFG FAIL: UART_TASK_STACK_SIZE < UART_TASK_STACK_MIN_WORDS\r\n");
        return false;
    }
    if (MONITOR_TASK_STACK_SIZE < MONITOR_TASK_STACK_MIN_WORDS) {
        System_Monitor_Log("CFG FAIL: MONITOR_TASK_STACK_SIZE < MONITOR_TASK_STACK_MIN_WORDS\r\n");
        return false;
    }
    if (MONITOR_TASK_STACK_SIZE < UART_TASK_STACK_SIZE) {
        System_Monitor_Log("CFG FAIL: MONITOR_TASK_STACK_SIZE < UART_TASK_STACK_SIZE\r\n");
        return false;
    }
    if (BSP_UART_RX_BUF_SIZE < BSP_UART_RX_BUF_MIN_SIZE) {
        System_Monitor_Log("CFG FAIL: BSP_UART_RX_BUF_SIZE < BSP_UART_RX_BUF_MIN_SIZE\r\n");
        return false;
    }
    if (MODBUS_BUFFER_SIZE < MODBUS_BUFFER_MIN_SIZE) {
        System_Monitor_Log("CFG FAIL: MODBUS_BUFFER_SIZE < MODBUS_BUFFER_MIN_SIZE\r\n");
        return false;
    }
    
#pragma diag_suppress 111  // 抑制"语句不可达"警告
    // 此检查用于验证配置的有效性，即使当前配置不为0，也应该保留此检查
    // 以防止未来修改配置时出现错误（防御性编程）
    if (BSP_UART_TX_TIMEOUT < BSP_UART_TX_TIMEOUT_MIN_MS) {
        System_Monitor_Log("CFG FAIL: BSP_UART_TX_TIMEOUT == 0\r\n");
        return false;
    }
#pragma diag_default 111   // 恢复警告
    
    if (SYSTEM_TASK_STOP_TIMEOUT_MS < SYSTEM_TASK_STOP_TIMEOUT_MIN_MS) {
        System_Monitor_Log("CFG FAIL: SYSTEM_TASK_STOP_TIMEOUT_MS < SYSTEM_TASK_STOP_TIMEOUT_MIN_MS\r\n");
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
    uint32_t watermark = 0U;
    uint32_t warn_th = 0U;
    bool *warn_active = NULL;

    if (task_name == NULL) {
        return;
    }

    warn_active = System_GetTaskStackWarnFlag(task_name);

    if (task_handle == NULL) {
        if (warn_active != NULL) {
            *warn_active = false;
        }
        (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                       "%s Stack Watermark: invalid handle\r\n", task_name);
        System_Monitor_Log(s_stack_log_line);
        return;
    }

    watermark = (uint32_t)uxTaskGetStackHighWaterMark(task_handle);
    warn_th = System_GetTaskStackWarnThreshold(task_name);

    /* Use %u for uint32_t on most embedded ARM GCC targets to avoid format warnings */
    (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                   "%s Stack Watermark: %u words\r\n",
                   task_name,
                   (unsigned int)watermark);
    System_Monitor_Log(s_stack_log_line);

    if ((warn_th > 0U) && (watermark <= warn_th)) {
        if (warn_active != NULL && *warn_active) {
            return;
        }
        if (warn_active != NULL) {
            *warn_active = true;
        }
        (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                       "WARN: %s stack low watermark <= %u words\r\n",
                       task_name,
                       (unsigned int)warn_th);
        System_Monitor_Log(s_stack_log_line);
    } else if ((warn_active != NULL) && *warn_active) {
        *warn_active = false;
        (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                       "INFO: %s stack watermark recovered above %u words\r\n",
                       task_name,
                       (unsigned int)warn_th);
        System_Monitor_Log(s_stack_log_line);
    }
}

static uint32_t System_GetTaskStackWarnThreshold(const char *task_name)
{
    if (task_name == NULL) {
        return 0U;
    }
    if (strcmp(task_name, "LED") == 0) {
        return LED_STACK_WM_WARN_WORDS;
    }
    if (strcmp(task_name, "UART") == 0) {
        return UART_STACK_WM_WARN_WORDS;
    }
    if (strcmp(task_name, "Monitor") == 0) {
        return MONITOR_STACK_WM_WARN_WORDS;
    }
    return 0U;
}

static bool *System_GetTaskStackWarnFlag(const char *task_name)
{
    if (task_name == NULL) {
        return NULL;
    }
    if (strcmp(task_name, "LED") == 0) {
        return &s_led_stack_warn_active;
    }
    if (strcmp(task_name, "UART") == 0) {
        return &s_uart_stack_warn_active;
    }
    if (strcmp(task_name, "Monitor") == 0) {
        return &s_monitor_stack_warn_active;
    }
    return NULL;
}

static osThreadId System_GetTaskHandleById(SystemTaskId_t task_id)
{
    switch (task_id) {
        case SYSTEM_TASK_LED:
            return s_led_task_handle;
        case SYSTEM_TASK_UART:
            return s_uart_task_handle;
        case SYSTEM_TASK_MONITOR:
            return s_monitor_task_handle;
        default:
            return NULL;
    }
}

static osPriority System_GetDefaultPriorityById(SystemTaskId_t task_id)
{
    switch (task_id) {
        case SYSTEM_TASK_LED:
            return LED_TASK_PRIORITY;
        case SYSTEM_TASK_UART:
            return UART_TASK_PRIORITY;
        case SYSTEM_TASK_MONITOR:
            return MONITOR_TASK_PRIORITY;
        default:
            return osPriorityError;
    }
}

/**
  * @brief  Initialize IWDG (Independent Watchdog)
  * @param  None
  * @retval None
  */
static void System_IWDG_Init(void)
{
#if (SYSTEM_USE_IWDG == 1U)
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
#else
    /* IWDG disabled, do nothing */
#endif
}
