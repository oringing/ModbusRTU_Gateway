#ifndef __DRIVER_SERVO_H__
#define __DRIVER_SERVO_H__

#include <stdint.h>
#include <stdbool.h>

/* Modbus 业务配置参数（舵机控制范围等） */
#define MODBUS_SERVO_TARGET_MIN          (0U)    /**< 舵机目标角度最小值 */
#define MODBUS_SERVO_TARGET_MAX          (180U)  /**< 舵机目标角度最大值 */
#define MODBUS_SERVO_SPEED_MIN           (0U)    /**< 舵机速度最小值 */
#define MODBUS_SERVO_SPEED_MAX           (255U) /**< 舵机速度最大值 */
#define MODBUS_SERVO_SPEED_NEUTRAL       (127U)  /**< 360° 舵机停止点 */

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
