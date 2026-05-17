#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 舵机 PWM 配置 */
#define SERVO_TIM_INSTANCE TIM3
#define SERVO_TIM_PRESCALER 71U /* 预分频系数，使计数器频率为 1MHz */
#define SERVO_TIM_PERIOD 19999U /* 20ms 周期 (72MHz / (19999+1) / (71+1) = 50Hz) */

/* 通道 1: PA6 */
#define SERVO_CH1_GPIO_PORT GPIOA
#define SERVO_CH1_GPIO_PIN GPIO_PIN_6
#define SERVO_CH1_CHANNEL TIM_CHANNEL_1

/* 通道 2: PA7 */
#define SERVO_CH2_GPIO_PORT GPIOA
#define SERVO_CH2_GPIO_PIN GPIO_PIN_7
#define SERVO_CH2_CHANNEL TIM_CHANNEL_2

/* 舵机脉宽范围 (实际根据舵机规格调整) */
#define SERVO_PULSE_MIN_US (500U)
#define SERVO_PULSE_MAX_US (2500U)
#define SERVO_PULSE_NEUTRAL_US (1500U)

/* 角度到脉宽映射参数 */
#define SERVO_ANGLE_MIN (0U)
#define SERVO_ANGLE_MAX (180U)
#define SERVO_PULSE_RANGE_US (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)

void BSP_Servo_Init(void);
void BSP_Servo_SetAngle(uint8_t channel, uint16_t angle);
void BSP_Servo_SetPulseWidth(uint8_t channel, uint16_t pulse_us);

#ifdef __cplusplus
}
#endif

#endif /* __SERVO_H__ */
