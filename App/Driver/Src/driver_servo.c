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

    if (speed_val < 127U) {
        /* 反转区：线性映射 0->2500us, 126->1500us */
        pulse_us = 2500U - (uint16_t)(((uint32_t)speed_val * 1000U) / 127U);
    } else if (speed_val > 127U) {
        /* 正转区：线性映射 128->1500us, 255->500us */
        pulse_us = 1500U - (uint16_t)(((uint32_t)(speed_val - 128U) * 1000U) / 127U);
    } else {
        /* 中位：停止 */
        pulse_us = 1500U;
    }

    /* 调用底层 PWM 设置函数（需确保 BSP 层支持微秒级输入） */
    /* 注意：这里我们暂时复用 SetAngle，但传入的是计算后的脉宽对应的“伪角度” */
    /* 更好的做法是在 BSP 层增加一个直接设置脉宽的接口 */
    BSP_Servo_SetPulseWidth(channel, pulse_us);
}
