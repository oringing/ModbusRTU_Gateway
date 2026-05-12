#ifndef __DRIVER_SERVO_H__
#define __DRIVER_SERVO_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool Servo_Driver_Init(void);
void Servo_Driver_SetAngle(uint8_t channel, uint16_t angle);
void Servo_Driver_SetSpeed(uint8_t channel, uint8_t speed_val);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_SERVO_H__ */
