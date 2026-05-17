// App/System/Inc/system_ctrl.h
#ifndef __SYSTEM_CTRL_H__
#define __SYSTEM_CTRL_H__

#include "cmsis_os.h"
#include <stdbool.h>
#include <stdint.h>

// ---- 看门狗配置（系统稳定性）----
#define SYSTEM_USE_IWDG 1U              // 1=启用硬件看门狗, 0=禁用（调试时可关闭）
#define IWDG_RELOAD_VALUE 312U          // 看门狗重载值，LSI约40KHz，超时≈2秒
// 计算公式：超时时间 = IWDG_RELOAD_VALUE * 256 / 40000 ≈ 2秒
#define IWDG_WINDOW_VALUE 4095U         // 窗口值(最大值=禁用窗口功能)

// ---- 配置合理性校验阈值（最小值约束，防止配置错误导致崩溃）----
#define LED_TASK_STACK_MIN_WORDS 48U        // LED任务最小栈大小(words)
#define UART_TASK_STACK_MIN_WORDS 96U       // UART任务最小栈大小(words)
#define MONITOR_TASK_STACK_MIN_WORDS 128U   // Monitor任务最小栈大小(words)
#define BSP_UART_RX_BUF_MIN_SIZE 256U       // UART接收缓冲区最小尺寸(bytes)
#define MODBUS_BUFFER_MIN_SIZE 256U         // Modbus缓冲区最小尺寸(bytes)
#define BSP_UART_TX_TIMEOUT_MIN_MS 1U       // UART发送超时最小值(ms)，不能为0
#define SYSTEM_TASK_STOP_TIMEOUT_MIN_MS 100U // 任务停止超时最小值(ms)
#define SYSTEM_STACK_LOG_BUF_SIZE 80U       // 栈水位日志缓冲区大小(bytes)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   系统状态码枚举
 */
typedef enum {
    SYSTEM_OK = 0,                 // 成功
    SYSTEM_ERR_NULL_PTR,           // 空指针错误
    SYSTEM_ERR_CONFIG_INVALID,     // 配置无效
    SYSTEM_ERR_TASK_CREATE_LED,    // LED任务创建失败
    SYSTEM_ERR_TASK_CREATE_UART,   // UART任务创建失败
    SYSTEM_ERR_TASK_CREATE_MONITOR // Monitor任务创建失败
} SystemStatus_t;

/**
 * @brief   任务ID枚举（用于优先级动态调整接口）
 */
typedef enum {
    SYSTEM_TASK_UART,     // UART任务
    SYSTEM_TASK_LED,      // LED任务
    SYSTEM_TASK_MONITOR,  // Monitor任务
    SYSTEM_TASK_MAX       // 任务总数(用于数组大小)
} SystemTaskId_t;

/**
 * @brief   动态设置任务优先级
 * @param   task_id 任务ID
 * @param   priority 目标优先级
 * @return  true=成功, false=任务句柄无效
 */
bool System_SetTaskPriority(SystemTaskId_t task_id, osPriority priority);

/**
 * @brief   获取任务当前优先级
 * @param   task_id 任务ID
 * @return  当前优先级，任务不存在时返回osPriorityError
 */
osPriority System_GetTaskPriority(SystemTaskId_t task_id);

/**
 * @brief   重置所有任务优先级为默认值
 * @warning 调用后恢复system_config.h中定义的初始优先级
 */
void System_ResetTaskPriorities(void);

/**
 * @brief   喂独立看门狗（刷新计数器）
 * @warning 需在Monitor任务中周期性调用，间隔<IWDG超时时间
 */
void System_IWDG_Feed(void);

/**
 * @brief   系统初始化（时钟+看门狗+UART驱动）
 * @warning 需在FreeRTOS调度器启动前调用
 */
void System_Init(void);

/**
 * @brief   系统控制模块初始化（配置校验+系统初始化）
 * @warning 配置校验失败时进入安全模式
 */
void System_Ctrl_Init(void);

/**
 * @brief   启动所有任务（LED/UART/Monitor）
 * @return  SYSTEM_OK=成功，其他=首个失败的任务错误码
 * @warning 任一任务创建失败会自动回滚已创建的任务
 */
SystemStatus_t System_StartTasks(void);

/**
 * @brief   停止所有任务（顺序：Monitor→UART→LED）
 * @warning 先请求优雅退出，超时后强制终止
 */
void System_StopTasks(void);

/**
 * @brief   获取最后一次系统错误码
 * @return  最近发生的系统错误
 */
SystemStatus_t System_GetLastError(void);

/**
 * @brief   创建FreeRTOS任务
 * @param   name 任务名称
 * @param   taskFunc 任务入口函数
 * @param   priority 任务优先级
 * @param   stackSize 栈大小(words)
 * @return  任务句柄，失败返回NULL
 * @warning 参数校验失败（NULL指针/零栈空间/无效优先级）时直接返回NULL
 */
osThreadId System_CreateTask(const char* name, void (*taskFunc)(void const* argument),
                             osPriority priority, uint32_t stackSize);

/**
 * @brief   销毁指定任务
 * @param   taskID 任务句柄
 * @warning taskID=NULL时无操作
 */
void System_DestroyTask(osThreadId taskID);

/**
 * @brief   获取任务栈水位（剩余最小栈空间）
 * @param   taskID 任务句柄
 * @return  剩余栈空间(words)，taskID=NULL时返回0
 * @warning 返回值越小表示栈使用越接近上限
 */
uint32_t System_GetStackWatermark(osThreadId taskID);

/**
 * @brief   检查并打印所有任务栈水位（含低水位告警）
 * @warning 低于阈值时输出告警日志，恢复时输出恢复日志
 */
void System_Check_Stack_Watermark(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CTRL_H__ */
