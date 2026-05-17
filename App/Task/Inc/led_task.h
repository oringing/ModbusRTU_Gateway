// App/Task/Inc/led_task.h
#ifndef __LED_TASK_H__
#define __LED_TASK_H__

#include "cmsis_os.h"

#define LED_TASK_TOGGLE_INTERVAL_MS 500U

#ifdef __cplusplus
extern "C" {
#endif

void Start_LED_Task(void const* argument);
void LED_Task_RequestStop(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_TASK_H__ */
