#include "driver_servo.h"
#include "servo.h"

bool Servo_Driver_Init(void) {
    BSP_Servo_Init();
    return true;
}

void Servo_Driver_SetAngle(uint8_t channel, uint16_t angle) {
    BSP_Servo_SetAngle(channel, angle);
}

// 360°舵机速度映射：线性转换速度值到PWM脉宽（0-255 → 500-2500us）
void Servo_Driver_SetSpeed(uint8_t channel, uint8_t speed_val) {
    uint16_t pulse_us;

    // 死区保护：强制临界值归零，避免机械抖动
    if (speed_val > MODBUS_SERVO_SPEED_REV_LIMIT && speed_val < MODBUS_SERVO_SPEED_FWD_LIMIT) {
        speed_val = MODBUS_SERVO_SPEED_NEUTRAL;  // 进入死区，强制停止
    }

    if (speed_val < MODBUS_SERVO_SPEED_NEUTRAL) {
        // 反转区：线性映射 0→2500us(最快反转), 126→1500us(慢速反转)
        pulse_us =
            SERVO_360_PULSE_REV_MAX_US -
            (uint16_t)(((uint32_t)speed_val * SERVO_360_PULSE_DELTA_US) / SERVO_360_SPEED_RANGE);
    } else if (speed_val > MODBUS_SERVO_SPEED_NEUTRAL) {
        // 正转区：线性映射 128→1500us(慢速正转), 255→500us(最快正转)
        pulse_us = SERVO_360_PULSE_STOP_US -
                   (uint16_t)(((uint32_t)(speed_val - (MODBUS_SERVO_SPEED_NEUTRAL + 1U)) *
                               SERVO_360_PULSE_DELTA_US) /
                              SERVO_360_SPEED_RANGE);
    } else {
        // 中位：停止（1500us）
        pulse_us = SERVO_360_PULSE_STOP_US;
    }

    BSP_Servo_SetPulseWidth(channel, pulse_us);
}
