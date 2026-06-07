// App/Task/Inc/device_task.h
#ifndef __DEVICE_TASK_H__
#define __DEVICE_TASK_H__

#include "cmsis_os.h"

// ---- 设备任务周期配置（可修改）----
#define DEVICE_TASK_BASE_DELAY_MS 50U     // 基础循环周期(ms)，让出CPU
#define DEVICE_LED_INTERVAL_MS 500U       // LED翻转间隔(ms)，产生1Hz闪烁
#define DEVICE_SENSOR_INTERVAL_MS 1000U   // 传感器读取间隔(ms)，匹配Modbus刷新周期
#define DEVICE_LED_UPDATE_INTERVAL_MS 50U // LED 更新间隔（与基础周期一致）

#define DEVICE_SENSOR_DEBUG_LOG_ENABLE 1    // 传感器测量数据日志开关，1=启用，0=禁用
#define DEVICE_SENSOR_LOG_EVERY_N_READS 5  // 传感器调试日志打印间隔（每秒读取1次，每N次读取后打印一次）


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   设备任务入口，管理LED闪烁和传感器数据采集
 * @param   argument 任务参数（未使用）
 * @warning 调用Device_Task_RequestStop()后自动退出
 */
void Start_Device_Task(void const* argument);

/**
 * @brief   请求设备任务停止
 * @warning 设置停止标志，任务会在当前周期结束后退出
 */
void Device_Task_RequestStop(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_TASK_H__ */
