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
