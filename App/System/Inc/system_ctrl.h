//App/System/Inc/system_ctrl.h
#ifndef __SYSTEM_CTRL_H__
#define __SYSTEM_CTRL_H__

#include "cmsis_os.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_OK = 0,
    SYSTEM_ERR_NULL_PTR,
    SYSTEM_ERR_TASK_CREATE_LED,
    SYSTEM_ERR_TASK_CREATE_UART,
    SYSTEM_ERR_TASK_CREATE_MONITOR
} SystemStatus_t;

void System_Init(void);
void System_Ctrl_Init(void);
SystemStatus_t System_StartTasks(void);
SystemStatus_t System_GetLastError(void);

osThreadId System_CreateTask(const char *name,
                             void (*taskFunc)(void const *argument),
                             osPriority priority,
                             uint32_t stackSize);
void System_DestroyTask(osThreadId taskID);
uint32_t System_GetStackWatermark(osThreadId taskID);
void System_RegisterTaskHandles(osThreadId ledTask, osThreadId uartTask, osThreadId monitorTask);

void System_StartMonitor(void);
void System_CheckStackWatermark(void);
void System_Check_Stack_Watermark(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CTRL_H__ */
