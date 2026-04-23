// Bsp/Inc/led.h
#ifndef __LED_H__
#define __LED_H__

#include "stm32f1xx_hal.h"

// 定义 LED 引脚
#define LED_C13_GPIO_Port GPIOC
#define LED_C13_Pin GPIO_PIN_13

// 函数声明
void BSP_LED_Init(void);
void BSP_LED_Toggle(void);
void BSP_LED_On(void);
void BSP_LED_Off(void);

#endif /* __LED_H__ */

