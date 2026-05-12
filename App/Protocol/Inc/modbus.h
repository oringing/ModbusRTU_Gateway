//App/Protocol/Inc/modbus.h
#ifndef __MODBUS_H__
#define __MODBUS_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_SLAVE_ADDR                 1
#define MODBUS_REG_MAX_COUNT             10
#define MODBUS_FUNC_READ_HOLDING_REGS    0x03
#define MODBUS_FUNC_WRITE_SINGLE_REG     0x06
#define MODBUS_RTU_READ_REQ_LEN          8U
#define MODBUS_RTU_WRITE_SINGLE_REQ_LEN  8U
#define MODBUS_RTU_WRITE_MULTI_MIN_LEN   11U
#define MODBUS_MAX_READ_REGS             125
#define MODBUS_BUFFER_SIZE               256
#define MODBUS_RESPONSE_BUFFER_SIZE      256
#define MODBUS_EXCEPTION_RESPONSE_SIZE   5
#define MODBUS_TX_TIMEOUT_MS             5U
#define MODBUS_POLL_INTERVAL             10

#define MODBUS_EX_ILLEGAL_FUNCTION       0x01U
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS   0x02U
#define MODBUS_EX_ILLEGAL_DATA_VALUE     0x03U

/* Frozen holding-register map for the first 0x06 implementation round.
 * External devices are not connected yet, so sensor/status slots still expose
 * demo defaults. Only the servo target register is writable for now. */
#define MODBUS_REG_ADDR_SENSOR_TEMP      0U
#define MODBUS_REG_ADDR_SENSOR_HUMIDITY  1U
#define MODBUS_REG_ADDR_DEVICE_STATUS    2U
#define MODBUS_REG_ADDR_SYSTEM_FLAGS     3U
#define MODBUS_REG_ADDR_SERVO_TARGET     4U  /* 180° 舵机角度 */
#define MODBUS_REG_ADDR_SERVO_SPEED      5U  /* 360° 舵机速度/方向 */
#define MODBUS_REG_ADDR_RESERVED_BEGIN   6U
#define MODBUS_REG_ADDR_RESERVED_END     9U

#define MODBUS_SERVO_TARGET_MIN          0U
#define MODBUS_SERVO_TARGET_MAX          180U
#define MODBUS_SERVO_SPEED_MIN           0U
#define MODBUS_SERVO_SPEED_MAX           255U
#define MODBUS_SERVO_SPEED_NEUTRAL       127U /* 360° 舵机停止点 */

typedef void (*ModbusRegisterOnChange_t)(uint16_t old_value, uint16_t new_value);

void Modbus_Init(void);
void Modbus_Process(void);
bool Modbus_ReadHoldingRegister(uint16_t addr, uint16_t *value);
bool Modbus_WriteHoldingRegister(uint16_t addr, uint16_t value);
bool Modbus_RegisterOnChange(uint16_t addr, ModbusRegisterOnChange_t on_change);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */
