// App/Task/Inc/monitor_task.h
#ifndef __MONITOR_TASK_H__
#define __MONITOR_TASK_H__

// ---- 任务周期配置（性能调优）----
#define MONITOR_TASK_BASE_DELAY 10U   // 监控任务基础循环周期(ms)
#define HEARTBEAT_INTERVAL_MS 1000U   // 心跳翻转间隔(ms)，产生系统活动指示
#define STACK_CHECK_INTERVAL_SEC 60U  // 栈水位检查间隔(s)，定期检查任务栈使用情况
#define IWDG_FEED_INTERVAL_MS 500U    // 看门狗喂狗间隔(ms)，须<IWDG超时时间(2秒)

// ---- 栈水位预警阈值（单位：words，低于该值触发告警）----
#define LED_STACK_WM_WARN_WORDS 24U      // LED任务栈告警阈值
#define UART_STACK_WM_WARN_WORDS 48U     // UART任务栈告警阈值
#define MONITOR_STACK_WM_WARN_WORDS 96U  // Monitor任务栈告警阈值

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"

/**
 * @brief   Monitor任务入口函数（心跳+喂狗+栈监控）
 * @param   argument 任务参数（未使用）
 * @warning 调用Monitor_Task_RequestStop()后会自动退出并销毁任务
 */
void Start_Monitor_Task(void const* argument);

/**
 * @brief   请求Monitor任务停止（优雅退出）
 * @warning 设置停止标志后，任务会在当前循环周期结束后退出
 */
void Monitor_Task_RequestStop(void);

#ifdef __cplusplus
}
#endif

#endif /* __MONITOR_TASK_H__ */
