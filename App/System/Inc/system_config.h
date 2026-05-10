//App/System/Inc/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cmsis_os.h"

/* 任务栈大小定义（单位：words） */
#define LED_TASK_STACK_SIZE              64   /* 空闲/高压均使用28words，预留1.3倍剩余量 */
#define UART_TASK_STACK_SIZE             125   /* 0.1s/帧高压峰值使用54words，预留1.3倍剩余量 */
#define MONITOR_TASK_STACK_SIZE          165  /* 空闲/高压均使用74words，预留1.3倍剩余量 */

/* 任务优先级定义 */
#define LED_TASK_PRIORITY                osPriorityLow
#define UART_TASK_PRIORITY               osPriorityNormal
#define MONITOR_TASK_PRIORITY            osPriorityBelowNormal

/* 系统监控相关 */
#define STACK_WATERMARK_CHECK_INTERVAL   10U // 栈水位检查间隔（单位：秒，实际间隔 = 10 × 1000ms）
#define STACK_WATERMARK_LOG_DELAY        1000U // 任务循环延迟（单位：ms）

/* 看门狗配置 */
#define SYSTEM_USE_IWDG                  1U      /* 1U启用硬件看门狗，0U禁用 */
#define IWDG_RELOAD_VALUE                312U   /* 看门狗重载值，配合LSI约40KHz， 约2秒超时（超时时间 = IWDG_RELOAD_VALUE * 256 / 40000）≈2 */
#define IWDG_WINDOW_VALUE                4095U   /* 窗口值，这里设为最大值，禁用窗口功能 */

/* 任务栈水位、看门狗初始化信息打印开关 */
#define SYSTEM_UART_TEXT_LOG_ENABLE      1U    

/* 栈水位预警阈值（单位：words，低于该值触发告警） */
#define LED_STACK_WM_WARN_WORDS          24U
#define UART_STACK_WM_WARN_WORDS         48U
#define MONITOR_STACK_WM_WARN_WORDS      96U

/* 任务退出等待超时（ms） */
#define SYSTEM_TASK_STOP_TIMEOUT_MS      300U

#endif /* SYSTEM_CONFIG_H */
