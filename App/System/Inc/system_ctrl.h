//App/System/Inc/system_ctrl.h
#ifndef __SYSTEM_CTRL_H__
#define __SYSTEM_CTRL_H__

#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>


/* 看门狗配置 */
#define SYSTEM_USE_IWDG                  1U      /* 1U启用硬件看门狗，0U禁用 */
#define IWDG_RELOAD_VALUE                312U   /* 看门狗重载值，配合LSI约40KHz， 约2秒超时（超时时间 = IWDG_RELOAD_VALUE * 256 / 40000）≈2 */
#define IWDG_WINDOW_VALUE                4095U   /* 窗口值，这里设为最大值，禁用窗口功能 */

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
void System_CheckStackWatermark(void);
void System_Check_Stack_Watermark(void);
bool System_SetTaskPriority(SystemTaskId_t task_id, osPriority priority);
osPriority System_GetTaskPriority(SystemTaskId_t task_id);
void System_ResetTaskPriorities(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CTRL_H__ */
