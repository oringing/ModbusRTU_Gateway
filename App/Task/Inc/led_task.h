// App/Task/Inc/led_task.h
#ifndef __LED_TASK_H__
#define __LED_TASK_H__

#include "cmsis_os.h"

#define LED_TASK_TOGGLE_INTERVAL_MS 500U // LED翻转间隔(ms)，产生1Hz闪烁频率

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   LED任务入口函数（周期性翻转LED状态）
 * @param   argument 任务参数（未使用）
 * @warning 调用LED_Task_RequestStop()后会自动退出并销毁任务
 */
void Start_LED_Task(void const* argument);

/**
 * @brief   请求LED任务停止（优雅退出）
 * @warning 设置停止标志后，任务会在当前翻转周期结束后退出
 */
void LED_Task_RequestStop(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_TASK_H__ */
