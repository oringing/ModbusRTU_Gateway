// App/System/Src/system_ctrl.c

#include "system_ctrl.h"
#include "driver_uart.h"
#include "error_handler.h"
#include "FreeRTOS.h"
#include "led_task.h"
#include "modbus.h"
#include "monitor_task.h"
#include "stdio.h"
#include "stm32f1xx_hal_iwdg.h"
#include "string.h"
#include "system_config.h"
#include "task.h"
#include "uart.h"
#include "uart_task.h"

// ---- 私有变量（任务句柄+状态标志）----
#if (SYSTEM_USE_IWDG == 1U)
static IWDG_HandleTypeDef hiwdg; // 全局IWDG句柄，仅System_IWDG_Init()写入
#endif

static osThreadId     s_led_task_handle = NULL;      // LED任务句柄
static osThreadId     s_uart_task_handle = NULL;     // UART任务句柄
static osThreadId     s_monitor_task_handle = NULL;  // Monitor任务句柄
static SystemStatus_t s_last_error = SYSTEM_OK;      // 最后一次错误码
static char           s_stack_log_line[SYSTEM_STACK_LOG_BUF_SIZE]; // 栈日志缓冲区（复用避免栈分配）
static bool           s_led_stack_warn_active = false;     // LED栈低水位告警激活标志
static bool           s_uart_stack_warn_active = false;    // UART栈低水位告警激活标志
static bool           s_monitor_stack_warn_active = false; // Monitor栈低水位告警激活标志

// ---- 私有函数声明 ----
static void       System_Monitor_Log(const char* msg);                           // 输出监控日志
static void       System_StopTaskIfRunning(osThreadId* task_handle);            // 停止任务(如果正在运行)
static bool       System_ValidateConfig(void);                                  // 校验配置参数有效性
static bool       System_WaitTaskDeleted(osThreadId task_handle, uint32_t timeout_ms); // 等待任务被删除
static void       System_LogTaskStackWatermark(const char* task_name, osThreadId task_handle); // 打印任务栈水位
static uint32_t   System_GetTaskStackWarnThreshold(const char* task_name);      // 获取任务栈告警阈值
static bool*      System_GetTaskStackWarnFlag(const char* task_name);           // 获取任务栈告警标志指针
static osThreadId System_GetTaskHandleById(SystemTaskId_t task_id);             // 根据ID获取任务句柄
static osPriority System_GetDefaultPriorityById(SystemTaskId_t task_id);        // 根据ID获取默认优先级
static void       System_IWDG_Init(void);                                       // 初始化独立看门狗

// ================= 公共API实现 =================

// 系统初始化：时钟+看门狗+UART驱动（需在调度器启动前调用）
void System_Init(void) {
    System_IWDG_Init();           // 初始化看门狗
    (void)UART_Driver_Init();     // 初始化UART驱动
}

// 系统控制模块初始化：配置校验+系统初始化（校验失败进入安全模式）
void System_Ctrl_Init(void) {
    if (!System_ValidateConfig()) {
        s_last_error = SYSTEM_ERR_CONFIG_INVALID;
        System_Monitor_Log("ERR: invalid system config\r\n");
        ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__);
        Error_Handler();
        return;
    }
    System_Init();
}

// 启动所有任务（LED/UART/Monitor），任一失败自动回滚
SystemStatus_t System_StartTasks(void) {
    // 创建LED任务
    if (s_led_task_handle == NULL) {
        s_led_task_handle =
            System_CreateTask("LED_Task", Start_LED_Task, LED_TASK_PRIORITY, LED_TASK_STACK_SIZE);
        if (s_led_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_LED;
            System_Monitor_Log("ERR: create LED_Task failed\r\n");
            return s_last_error;
        }
    }

    // 创建UART任务
    if (s_uart_task_handle == NULL) {
        s_uart_task_handle = System_CreateTask("UART_Task", Start_UART_Task, UART_TASK_PRIORITY,
                                               UART_TASK_STACK_SIZE);
        if (s_uart_task_handle == NULL) {
            s_last_error = SYSTEM_ERR_TASK_CREATE_UART;
            System_Monitor_Log("ERR: create UART_Task failed, rollback\r\n");
            System_StopTasks();
            return s_last_error;
        }
    }

    // 创建Monitor任务
    if (s_monitor_task_handle == NULL) {
        s_monitor_task_handle = System_CreateTask("Monitor_Task", Start_Monitor_Task,
                                                  MONITOR_TASK_PRIORITY, MONITOR_TASK_STACK_SIZE);
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

// 停止所有任务（顺序：Monitor→UART→LED，先优雅退出后强制终止）
void System_StopTasks(void) {
    System_StopTaskIfRunning(&s_monitor_task_handle);
    System_StopTaskIfRunning(&s_uart_task_handle);
    System_StopTaskIfRunning(&s_led_task_handle);
}

SystemStatus_t System_GetLastError(void) {
    return s_last_error;
}

// 创建FreeRTOS任务（参数校验+封装osThreadCreate）
osThreadId System_CreateTask(const char* name, void (*taskFunc)(void const* argument),
                             osPriority priority, uint32_t stackSize) {
    // 参数校验：拒绝NULL指针/零栈空间/无效优先级
    if (name == NULL || taskFunc == NULL || stackSize == 0U || priority == osPriorityError) {
        return NULL;
    }

    osThreadDef_t thread_def = {
        .name = (char*)name,
        .pthread = taskFunc,
        .tpriority = priority,
        .instances = 1U,
        .stacksize = stackSize
    };

    return osThreadCreate(&thread_def, NULL);
}

void System_DestroyTask(osThreadId taskID) {
    if (taskID != NULL) {
        osThreadTerminate(taskID);
    }
}

// 获取任务栈水位（剩余最小栈空间，单位word）
uint32_t System_GetStackWatermark(osThreadId taskID) {
    if (taskID == NULL) {
        return 0U;
    }
    return (uint32_t)uxTaskGetStackHighWaterMark(taskID);
}

// 检查并打印所有任务栈水位（旧接口，保留兼容）
void System_CheckStackWatermark(void) {
    System_LogTaskStackWatermark("UART", s_uart_task_handle);
    System_LogTaskStackWatermark("LED", s_led_task_handle);
    System_LogTaskStackWatermark("Monitor", s_monitor_task_handle);
}

void System_Check_Stack_Watermark(void) {
    System_CheckStackWatermark();
}

bool System_SetTaskPriority(SystemTaskId_t task_id, osPriority priority) {
    osThreadId task_handle = System_GetTaskHandleById(task_id);
    if (task_handle == NULL) {
        return false;
    }
    return (osThreadSetPriority(task_handle, priority) == osOK);
}

osPriority System_GetTaskPriority(SystemTaskId_t task_id) {
    osThreadId task_handle = System_GetTaskHandleById(task_id);
    if (task_handle == NULL) {
        return osPriorityError;
    }
    return osThreadGetPriority(task_handle);
}

void System_ResetTaskPriorities(void) {
    (void)System_SetTaskPriority(SYSTEM_TASK_LED, System_GetDefaultPriorityById(SYSTEM_TASK_LED));
    (void)System_SetTaskPriority(SYSTEM_TASK_UART, System_GetDefaultPriorityById(SYSTEM_TASK_UART));
    (void)System_SetTaskPriority(SYSTEM_TASK_MONITOR,
                                 System_GetDefaultPriorityById(SYSTEM_TASK_MONITOR));
}

// ================= 私有函数实现 =================

// 通过UART输出日志消息（受SYSTEM_UART_TEXT_LOG_ENABLE宏控制）
static void System_Monitor_Log(const char* msg) {
    if (msg == NULL) {
        return;
    }

#if (SYSTEM_UART_TEXT_LOG_ENABLE == 1U)
    (void)UART_Driver_Send((const uint8_t*)msg, (uint16_t)strlen(msg), 20U);
#else
    (void)msg; // 避免未使用参数警告
#endif
}

// 停止指定任务：调度器运行时请求优雅退出，超时后强制终止
static void System_StopTaskIfRunning(osThreadId* task_handle) {
    if (task_handle == NULL || *task_handle == NULL) {
        return;
    }

    // 调度器运行时，请求任务优雅退出
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (task_handle == &s_led_task_handle) {
            LED_Task_RequestStop();
        } else if (task_handle == &s_uart_task_handle) {
            UART_Task_RequestStop();
        } else if (task_handle == &s_monitor_task_handle) {
            Monitor_Task_RequestStop();
        }

        // 等待任务退出，超时则强制终止
        if (!System_WaitTaskDeleted(*task_handle, SYSTEM_TASK_STOP_TIMEOUT_MS)) {
            System_Monitor_Log("WARN: graceful stop timeout, force terminate\r\n");
            System_DestroyTask(*task_handle);
        }
    } else {
        // 调度器未运行，直接销毁
        System_DestroyTask(*task_handle);
    }

    *task_handle = NULL;
}

// 校验系统配置参数是否在合理范围内（防止配置错误导致崩溃）
static bool System_ValidateConfig(void) {
    char log_buf[128];
    
    // === 编译期检查（宏定义冲突在编译时报错）===
#if (MONITOR_TASK_STACK_SIZE < UART_TASK_STACK_SIZE)
#error "MONITOR_TASK_STACK_SIZE must be >= UART_TASK_STACK_SIZE"
#endif
    
#if (MODBUS_SLAVE_ADDR == 0U || MODBUS_SLAVE_ADDR > 247U)
#error "MODBUS_SLAVE_ADDR must be in range [1, 247]"
#endif

    // === 运行时检查（动态配置或外部输入）===
    
    // LED任务栈大小检查
    if (LED_TASK_STACK_SIZE < LED_TASK_STACK_MIN_WORDS) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: LED_TASK_STACK_SIZE=%u < MIN=%u\r\n",
            (unsigned int)LED_TASK_STACK_SIZE, (unsigned int)LED_TASK_STACK_MIN_WORDS);
        System_Monitor_Log(log_buf);
        return false;
    }
    
    // UART任务栈大小检查
    if (UART_TASK_STACK_SIZE < UART_TASK_STACK_MIN_WORDS) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: UART_TASK_STACK_SIZE=%u < MIN=%u\r\n",
            (unsigned int)UART_TASK_STACK_SIZE, (unsigned int)UART_TASK_STACK_MIN_WORDS);
        System_Monitor_Log(log_buf);
        return false;
    }
    
    // Monitor任务栈大小检查
    if (MONITOR_TASK_STACK_SIZE < MONITOR_TASK_STACK_MIN_WORDS) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: MONITOR_TASK_STACK_SIZE=%u < MIN=%u\r\n",
            (unsigned int)MONITOR_TASK_STACK_SIZE, (unsigned int)MONITOR_TASK_STACK_MIN_WORDS);
        System_Monitor_Log(log_buf);
        return false;
    }
    
    // UART接收缓冲区大小检查
    if (BSP_UART_RX_BUF_SIZE < BSP_UART_RX_BUF_MIN_SIZE) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: BSP_UART_RX_BUF_SIZE=%u < MIN=%u\r\n",
            (unsigned int)BSP_UART_RX_BUF_SIZE, (unsigned int)BSP_UART_RX_BUF_MIN_SIZE);
        System_Monitor_Log(log_buf);
        return false;
    }
    
    // Modbus缓冲区大小检查
    if (MODBUS_BUFFER_SIZE < MODBUS_BUFFER_MIN_SIZE) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: MODBUS_BUFFER_SIZE=%u < MIN=%u\r\n",
            (unsigned int)MODBUS_BUFFER_SIZE, (unsigned int)MODBUS_BUFFER_MIN_SIZE);
        System_Monitor_Log(log_buf);
        return false;
    }

#pragma diag_suppress 111 // 抑制"语句不可达"警告
    // 防御性编程：即使当前配置不为0，也保留此检查，防止未来修改出错
    if (BSP_UART_TX_TIMEOUT < BSP_UART_TX_TIMEOUT_MIN_MS) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: BSP_UART_TX_TIMEOUT=%u < MIN=%u\r\n",
            (unsigned int)BSP_UART_TX_TIMEOUT, (unsigned int)BSP_UART_TX_TIMEOUT_MIN_MS);
        System_Monitor_Log(log_buf);
        return false;
    }
#pragma diag_default 111 // 恢复警告

    // 任务停止超时检查
    if (SYSTEM_TASK_STOP_TIMEOUT_MS < SYSTEM_TASK_STOP_TIMEOUT_MIN_MS) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: SYSTEM_TASK_STOP_TIMEOUT_MS=%u < MIN=%u\r\n",
            (unsigned int)SYSTEM_TASK_STOP_TIMEOUT_MS, (unsigned int)SYSTEM_TASK_STOP_TIMEOUT_MIN_MS);
        System_Monitor_Log(log_buf);
        return false;
    }
    
    // Modbus从机地址范围检查（编译期已检查，此处为运行时防御）
    if (MODBUS_SLAVE_ADDR == 0U || MODBUS_SLAVE_ADDR > 247U) {
        (void)snprintf(log_buf, sizeof(log_buf),
            "CFG FAIL: MODBUS_SLAVE_ADDR=%u out of range [1, 247]\r\n",
            (unsigned int)MODBUS_SLAVE_ADDR);
        System_Monitor_Log(log_buf);
        return false;
    }
    
    return true;
}

// 等待任务被删除（依赖INCLUDE_eTaskGetState宏），超时返回false
static bool System_WaitTaskDeleted(osThreadId task_handle, uint32_t timeout_ms) {
#if (INCLUDE_eTaskGetState == 1)
    uint32_t elapsed = 0U;
    while (elapsed < timeout_ms) {
        eTaskState state = eTaskGetState((TaskHandle_t)task_handle);
        if (state == eDeleted || state == eInvalid) {
            return true; // 任务已删除
        }
        osDelay(1U);
        elapsed++;
    }
    return false; // 超时
#else
    // 任务状态检查不可用时，直接等待超时时间
    (void)task_handle;
    osDelay(timeout_ms);
    return false;
#endif
}

// 打印任务栈水位，并在低于阈值时输出告警（避免重复告警）
static void System_LogTaskStackWatermark(const char* task_name, osThreadId task_handle) {
    uint32_t watermark = 0U;
    uint32_t warn_th = 0U;
    bool*    warn_active = NULL;

    if (task_name == NULL) {
        return;
    }

    warn_active = System_GetTaskStackWarnFlag(task_name);

    // 任务句柄无效
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

    // 打印当前栈水位
    (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line), "%s Stack Watermark: %u words\r\n",
                   task_name, (unsigned int)watermark);
    System_Monitor_Log(s_stack_log_line);

    // 栈水位低于告警阈值
    if ((warn_th > 0U) && (watermark <= warn_th)) {
        // 避免重复告警
        if (warn_active != NULL && *warn_active) {
            return;
        }
        if (warn_active != NULL) {
            *warn_active = true;
        }
        (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                       "WARN: %s stack low watermark <= %u words\r\n", task_name,
                       (unsigned int)warn_th);
        System_Monitor_Log(s_stack_log_line);
    }
    // 栈水位恢复到阈值以上
    else if ((warn_active != NULL) && *warn_active) {
        *warn_active = false;
        (void)snprintf(s_stack_log_line, sizeof(s_stack_log_line),
                       "INFO: %s stack watermark recovered above %u words\r\n", task_name,
                       (unsigned int)warn_th);
        System_Monitor_Log(s_stack_log_line);
    }
}

// 根据任务名获取栈告警阈值
static uint32_t System_GetTaskStackWarnThreshold(const char* task_name) {
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

// 根据任务名获取栈告警标志指针
static bool* System_GetTaskStackWarnFlag(const char* task_name) {
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

// 根据任务ID获取任务句柄
static osThreadId System_GetTaskHandleById(SystemTaskId_t task_id) {
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

// 根据任务ID获取默认优先级
static osPriority System_GetDefaultPriorityById(SystemTaskId_t task_id) {
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

// 初始化独立看门狗（IWDG），LSI约40KHz，超时≈2秒
static void System_IWDG_Init(void) {
#if (SYSTEM_USE_IWDG == 1U)
    hiwdg.Instance = IWDG;
    // 预分频256，计数频率≈156Hz
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    // 重装载值312，超时≈2秒
    hiwdg.Init.Reload = IWDG_RELOAD_VALUE;

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__); // 初始化失败，记录错误
    } else {
        System_Monitor_Log("IWDG initialized successfully\r\n");
    }
#endif
}

// 喂独立看门狗（刷新计数器）
void System_IWDG_Feed(void) {
#if (SYSTEM_USE_IWDG == 1U)
    HAL_IWDG_Refresh(&hiwdg);
#else
    // 看门狗禁用，什么都不做
#endif
}
