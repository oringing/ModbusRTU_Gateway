//App/System/Inc/system_ctrl.h
#ifndef __SYSTEM_CTRL_H__
#define __SYSTEM_CTRL_H__

#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>


/* 看门狗配置 */
#define SYSTEM_USE_IWDG                     1U      /* 1U启用硬件看门狗，0U禁用 */
#define IWDG_RELOAD_VALUE                   312U   /* 看门狗重载值，配合LSI约40KHz， 约2秒超时（超时时间 = IWDG_RELOAD_VALUE * 256 / 40000）≈2 */
#define IWDG_WINDOW_VALUE                   4095U   /* 窗口值，这里设为最大值，禁用窗口功能 */

/* 配置合理性校验阈值（最小值约束） */
#define LED_TASK_STACK_MIN_WORDS            48U    /* LED 任务最小栈大小 */
#define UART_TASK_STACK_MIN_WORDS           96U    /* UART 任务最小栈大小 */
#define MONITOR_TASK_STACK_MIN_WORDS        128U   /* Monitor 任务最小栈大小 */
#define BSP_UART_RX_BUF_MIN_SIZE            256U   /* UART 接收缓冲区最小尺寸 */
#define MODBUS_BUFFER_MIN_SIZE              256U   /* Modbus 缓冲区最小尺寸 */
#define BSP_UART_TX_TIMEOUT_MIN_MS          1U     /* UART 发送超时最小值（不能为0） */
#define SYSTEM_TASK_STOP_TIMEOUT_MIN_MS     100U   /* 任务停止超时最小值 */
#define SYSTEM_STACK_LOG_BUF_SIZE           (80U)   /**< 栈水位日志缓冲区大小 */


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_OK = 0, 
    SYSTEM_ERR_NULL_PTR, 
    SYSTEM_ERR_CONFIG_INVALID, 
    SYSTEM_ERR_TASK_CREATE_LED, 
    SYSTEM_ERR_TASK_CREATE_UART,  
    SYSTEM_ERR_TASK_CREATE_MONITOR   
} SystemStatus_t;

// === 新增：任务 ID 枚举 ===
typedef enum {
    SYSTEM_TASK_UART,
    SYSTEM_TASK_LED,
    SYSTEM_TASK_MONITOR,
    SYSTEM_TASK_MAX
} SystemTaskId_t;

// === 接口声明 ===
bool System_SetTaskPriority(SystemTaskId_t task_id, osPriority priority);
osPriority System_GetTaskPriority(SystemTaskId_t task_id);
void System_ResetTaskPriorities(void);
void System_IWDG_Feed(void);
void System_Init(void);
void System_Ctrl_Init(void);
SystemStatus_t System_StartTasks(void);
void System_StopTasks(void);
SystemStatus_t System_GetLastError(void);
osThreadId System_CreateTask(const char *name,void (*taskFunc)(void const *argument),osPriority priority,uint32_t stackSize);
void System_DestroyTask(osThreadId taskID);
uint32_t System_GetStackWatermark(osThreadId taskID);
void System_Check_Stack_Watermark(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CTRL_H__ */
