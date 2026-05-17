#include "driver_servo.h"
#include "servo.h"

bool Servo_Driver_Init(void)
{
    BSP_Servo_Init();
    return true;
}

void Servo_Driver_SetAngle(uint8_t channel, uint16_t angle)
{
    BSP_Servo_SetAngle(channel, angle);
}

/**
 * @brief 设置 360° 连续旋转舵机的速度与方向
 * @param channel: 通道索引 (1 或 2)
 * @param speed_val: 速度值 (0~255)
 * 
 * 映射逻辑：
 * - 127: 停止 (1.5ms)
 * - 0~126: 反转加速 (2.5ms ~ 1.5ms)
 * - 128~255: 正转加速 (1.5ms ~ 0.5ms)
 */
void Servo_Driver_SetSpeed(uint8_t channel, uint8_t speed_val)
{
    uint16_t pulse_us;

    if (speed_val < MODBUS_SERVO_SPEED_NEUTRAL) {
        /* 反转区：线性映射 0->2500us, 126->1500us */
        pulse_us = SERVO_360_PULSE_REV_MAX_US - (uint16_t)(((uint32_t)speed_val * SERVO_360_PULSE_DELTA_US) / SERVO_360_SPEED_RANGE);
    } else if (speed_val > MODBUS_SERVO_SPEED_NEUTRAL) {
        /* 正转区：线性映射 128->1500us, 255->500us */
        pulse_us = SERVO_360_PULSE_STOP_US - (uint16_t)(((uint32_t)(speed_val - (MODBUS_SERVO_SPEED_NEUTRAL + 1U)) * SERVO_360_PULSE_DELTA_US) / SERVO_360_SPEED_RANGE);
    } else {
        /* 中位：停止 */
        pulse_us = SERVO_360_PULSE_STOP_US;
    }

    /* 调用底层 PWM 设置函数 */
    BSP_Servo_SetPulseWidth(channel, pulse_us);
}
