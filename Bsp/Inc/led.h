// Bsp/Inc/led.h
#ifndef __LED_H__
#define __LED_H__

#include "stm32f1xx_hal.h"

// ---- 硬件引脚定义（与电路图同步）----
#define LED_C13_GPIO_Port GPIOC // LED连接的GPIO端口（PC13）
#define LED_C13_Pin GPIO_PIN_13 // LED连接的GPIO引脚（PC13，低电平点亮）

/**
 * @brief   初始化LED引脚（PC13）
 * @warning 需在系统启动时调用一次
 */
void BSP_LED_Init(void);

/**
 * @brief   切换LED状态（亮→灭或灭→亮）
 */
void BSP_LED_Toggle(void);

/**
 * @brief   点亮LED（输出低电平）
 */
void BSP_LED_On(void);

/**
 * @brief   熄灭LED（输出高电平）
 */
void BSP_LED_Off(void);

#endif /* __LED_H__ */
