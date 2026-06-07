// App/Driver/Inc/driver_led.h
#ifndef __DRIVER_LED_H__
#define __DRIVER_LED_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED 模式定义
typedef enum {
    LED_MODE_OFF = 0,   // 常灭
    LED_MODE_ON,        // 常亮
    LED_MODE_HEARTBEAT, // 心跳模式（1Hz 翻转）
    LED_MODE_ERROR_1HZ, // 错误模式（1Hz 闪烁）
    LED_MODE_ERROR_2HZ, // 错误模式（2Hz 闪烁）
    LED_MODE_ERROR_4HZ, // 错误模式（4Hz 快速闪烁）
    LED_MODE_FAULT      // 故障模式（常亮，不可清除）
} LED_Mode_t;

/**
 * @brief   初始化 LED 驱动层
 */
void LED_Driver_Init(void);

/**
 * @brief   设置 LED 模式（线程安全）
 * @param   mode 目标模式
 * @note    高优先级模式（如 FAULT）不能被低优先级覆盖
 */
void LED_Driver_SetMode(LED_Mode_t mode);

/**
 * @brief   获取当前 LED 模式
 */
LED_Mode_t LED_Driver_GetMode(void);

/**
 * @brief   LED 任务处理函数（需周期性调用）
 * @note    在 LED_Task 中调用，或在 Monitor_Task 中调用
 * @warning 调用频率建议 50-100ms
 */
void LED_Driver_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_LED_H__ */
