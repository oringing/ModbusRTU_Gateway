#ifndef __DRIVER_SERVO_H__
#define __DRIVER_SERVO_H__

#include <stdbool.h>
#include <stdint.h>

/* Modbus 业务配置参数（舵机控制范围等） */
#define MODBUS_SERVO_TARGET_MIN (0U)      /**< 舵机目标角度最小值 */
#define MODBUS_SERVO_TARGET_MAX (180U)    /**< 舵机目标角度最大值 */
#define MODBUS_SERVO_SPEED_MIN (0U)       /**< 舵机速度最小值 */
#define MODBUS_SERVO_SPEED_MAX (255U)     /**< 舵机速度最大值 */
#define MODBUS_SERVO_SPEED_NEUTRAL (127U) /**< 360° 舵机停止点 */

#define SERVO_360_PULSE_STOP_US (1500U)
#define SERVO_360_PULSE_REV_MAX_US (2500U) /* Full reverse speed */
#define SERVO_360_PULSE_FWD_MAX_US (500U)  /* Full forward speed */
#define SERVO_360_SPEED_RANGE (127U)       /* Half of 0-255 range */
#define SERVO_360_PULSE_DELTA_US (1000U)   /* Delta from neutral to max speed */

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
