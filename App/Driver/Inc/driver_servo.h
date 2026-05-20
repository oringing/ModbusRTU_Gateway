#ifndef __DRIVER_SERVO_H__
#define __DRIVER_SERVO_H__

#include <stdbool.h>
#include <stdint.h>

// ---- 180°舵机业务配置（可调整）----
#define MODBUS_SERVO_TARGET_MIN (0U)    // 角度最小值(度)
#define MODBUS_SERVO_TARGET_MAX (180U)  // 角度最大值(度)

// ---- 360°舵机业务配置（可调整）----
#define MODBUS_SERVO_SPEED_MIN (0U)        // 速度最小值
#define MODBUS_SERVO_SPEED_MAX (255U)      // 速度最大值
#define MODBUS_SERVO_SPEED_NEUTRAL (127U)  // 停止点（中位值）

// ---- 360°舵机死区保护参数（防止临界值抖动）----
#define MODBUS_SERVO_DEADZONE_RANGE (10U)   // 死区范围（±10档，对应约±80μs PWM变化）
#define MODBUS_SERVO_SPEED_REV_LIMIT (117U)  // 反转有效下限（127-10），低于此值才执行反转
#define MODBUS_SERVO_SPEED_FWD_LIMIT (137U)  // 正转有效上限（127+10），高于此值才执行正转

// ---- 360°舵机PWM脉宽映射参数（标准SG90规格）----
#define SERVO_360_PULSE_STOP_US (1500U)    // 停止脉宽(us)，对应中位1.5ms
#define SERVO_360_PULSE_REV_MAX_US (2500U) // 最大反转脉宽(us)，对应2.5ms
#define SERVO_360_PULSE_FWD_MAX_US (500U)  // 最大正转脉宽(us)，对应0.5ms
#define SERVO_360_SPEED_RANGE (127U)       // 速度中位值（用于线性映射分母）
#define SERVO_360_PULSE_DELTA_US (1000U)   // 脉宽变化量(us)，中位到极值的差值

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   初始化舵机驱动层（调用BSP层TIM3 PWM初始化）
 * @return  true=成功
 */
bool Servo_Driver_Init(void);

/**
 * @brief   设置舵机目标角度（仅通道1-180°舵机有效）
 * @param   channel 通道号，1=PA6(180°), 2=PA7(360°忽略此参数)
 * @param   angle 目标角度(度)，范围[0, 180]
 * @warning 通道2为连续旋转舵机，应使用Servo_Driver_SetSpeed控制速度
 */
void Servo_Driver_SetAngle(uint8_t channel, uint16_t angle);

/**
 * @brief   设置360°舵机速度（线性映射到PWM脉宽）
 * @param   channel 通道号，1=PA6, 2=PA7
 * @param   speed_val 速度值，范围[0, 255]
 *          - 0-126: 反转（0=最快反转，126=慢速反转）
 *          - 127: 停止
 *          - 128-255: 正转（128=慢速正转，255=最快正转）
 * @warning 内部自动映射到500-2500us脉宽范围
 */
void Servo_Driver_SetSpeed(uint8_t channel, uint8_t speed_val);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_SERVO_H__ */
