//Bsp/Inc/servo.h
#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- PWM定时器配置（不可修改）----
#define SERVO_TIM_INSTANCE TIM3           // 舵机PWM定时器实例（TIM3）
#define SERVO_TIM_PRESCALER 71U           // 预分频系数，使计数器频率为1MHz（72MHz/(71+1)）
#define SERVO_TIM_PERIOD 19999U           // 20ms周期(72MHz/(19999+1)/(71+1)=50Hz)

// ---- 通道1硬件引脚（PA6，180°舵机）----
#define SERVO_CH1_GPIO_PORT GPIOA         // 通道1 GPIO端口
#define SERVO_CH1_GPIO_PIN GPIO_PIN_6     // 通道1 GPIO引脚（PA6）
#define SERVO_CH1_CHANNEL TIM_CHANNEL_1   // 通道1 TIM通道

// ---- 通道2硬件引脚（PA7，360°连续旋转舵机）----
#define SERVO_CH2_GPIO_PORT GPIOA         // 通道2 GPIO端口
#define SERVO_CH2_GPIO_PIN GPIO_PIN_7     // 通道2 GPIO引脚（PA7）
#define SERVO_CH2_CHANNEL TIM_CHANNEL_2   // 通道2 TIM通道

// ---- 舵机脉宽参数（标准SG90规格）----
#define SERVO_PULSE_MIN_US (500U)         // 最小脉宽(us)，对应0°或全速反转
#define SERVO_PULSE_MAX_US (2500U)        // 最大脉宽(us)，对应180°或全速正转
#define SERVO_PULSE_NEUTRAL_US (1500U)    // 中位脉宽(us)，对应90°或停止

// ---- 角度映射参数（仅用于180°舵机）----
#define SERVO_ANGLE_MIN (0U)              // 最小角度(度)
#define SERVO_ANGLE_MAX (180U)            // 最大角度(度)
#define SERVO_PULSE_RANGE_US (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US) // 脉宽范围(us)

/**
 * @brief   初始化TIM3 PWM输出（通道1-PA6，通道2-PA7）
 * @warning 需在FreeRTOS启动前调用
 */
void BSP_Servo_Init(void);

/**
 * @brief   设置舵机目标角度（仅通道1-180°舵机有效）
 * @param   channel 通道号，1=PA6(180°), 2=PA7(360°忽略此参数)
 * @param   angle 目标角度(度)，范围[0, 180]
 * @warning 通道2为连续旋转舵机，应使用BSP_Servo_SetPulseWidth控制速度
 */
void BSP_Servo_SetAngle(uint8_t channel, uint16_t angle);

/**
 * @brief   直接设置PWM脉宽（通用接口）
 * @param   channel 通道号，1=PA6, 2=PA7
 * @param   pulse_us 脉宽(us)，范围[500, 2500]
 * @warning 脉宽超出范围会被自动钳位到边界值
 */
void BSP_Servo_SetPulseWidth(uint8_t channel, uint16_t pulse_us);

#ifdef __cplusplus
}
#endif

#endif /* __SERVO_H__ */
