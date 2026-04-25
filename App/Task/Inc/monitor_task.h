//App/Task/Inc/monitor_task.h
#ifndef __MONITOR_TASK_H__
#define __MONITOR_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* Exported functions prototypes ---------------------------------------------*/
void Start_Monitor_Task(void const * argument);
void Monitor_Task_RequestStop(void);
void Check_Stack_Watermark(void);

#ifdef __cplusplus
}
#endif

#endif /* __MONITOR_TASK_H__ */
