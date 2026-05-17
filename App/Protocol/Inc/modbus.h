//App/Protocol/Inc/modbus.h
#ifndef __MODBUS_H__
#define __MODBUS_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================= Modbus Protocol Constants ================= */
#define MODBUS_SLAVE_ADDR                 (1U)    /**< Modbus 从机地址 */
#define MODBUS_REG_MAX_COUNT             (10U)   /**< 最大寄存器数量 */

/* Function Codes */
#define MODBUS_FUNC_READ_HOLDING_REGS    (0x03U) /**< 读保持寄存器 */
#define MODBUS_FUNC_WRITE_SINGLE_REG     (0x06U) /**< 写单寄存器 */

/* Frame Length Constants */
#define MODBUS_RTU_READ_REQ_LEN          (8U)    /**< 读请求帧固定长度 */
#define MODBUS_RTU_WRITE_SINGLE_REQ_LEN  (8U)    /**< 写单寄存器请求帧固定长度 */
#define MODBUS_RTU_WRITE_MULTI_MIN_LEN   (11U)   /**< 写多寄存器最小帧长度 */
#define MODBUS_CRC_LEN                   (2U)    /**< CRC 校验码长度（字节） */
#define MODBUS_REG_SIZE_BYTES            (2U)    /**< 单个寄存器占用字节数 */

/* Buffer Size */
#define MODBUS_BUFFER_SIZE               (256U)  /**< 接收缓冲区大小 */
#define MODBUS_RESPONSE_BUFFER_SIZE      (256U)  /**< 发送缓冲区大小 */
#define MODBUS_EXCEPTION_RESPONSE_SIZE   (5U)    /**< 异常响应帧长度 */

/* Timing */
#define MODBUS_TX_TIMEOUT_MS             (5U)    /**< 发送超时时间 (ms) */
#define MODBUS_POLL_INTERVAL             (10U)   /**< 轮询间隔 (ms) */

/* Exception Codes */
#define MODBUS_EX_ILLEGAL_FUNCTION       (0x01U) /**< 非法功能码 */
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS   (0x02U) /**< 非法数据地址 */
#define MODBUS_EX_ILLEGAL_DATA_VALUE     (0x03U) /**< 非法数据值 */

/* ================= Modbus RTU Frame Field Indexes ================= */
/* Request Frame Indexes (通用) */
#define MODBUS_REQ_ADDR_IDX              (0U)    /**< 从机地址索引 */
#define MODBUS_REQ_FUNC_CODE_IDX         (1U)    /**< 功能码索引 */

/* Read Holding Registers Request (0x03) */
#define MODBUS_REQ_START_ADDR_HIGH_IDX   (2U)    /**< 起始地址高字节 */
#define MODBUS_REQ_START_ADDR_LOW_IDX    (3U)    /**< 起始地址低字节 */
#define MODBUS_REQ_REG_COUNT_HIGH_IDX    (4U)    /**< 寄存器数量高字节 */
#define MODBUS_REQ_REG_COUNT_LOW_IDX     (5U)    /**< 寄存器数量低字节 */

/* Write Single Register Request (0x06) */
#define MODBUS_REQ_REG_ADDR_HIGH_IDX     (2U)    /**< 寄存器地址高字节 */
#define MODBUS_REQ_REG_ADDR_LOW_IDX      (3U)    /**< 寄存器地址低字节 */
#define MODBUS_REQ_REG_VALUE_HIGH_IDX    (4U)    /**< 寄存器值高字节 */
#define MODBUS_REQ_REG_VALUE_LOW_IDX     (5U)    /**< 寄存器值低字节 */

/* Response Frame Indexes */
#define MODBUS_RESP_ADDR_IDX             (0U)    /**< 响应帧从机地址 */
#define MODBUS_RESP_FUNC_CODE_IDX        (1U)    /**< 响应帧功能码 */
#define MODBUS_RESP_BYTE_COUNT_IDX       (2U)    /**< 响应帧字节计数 */
#define MODBUS_RESP_DATA_START_IDX       (3U)    /**< 响应帧数据起始位置 */

/* Exception Response Frame Indexes */
#define MODBUS_EX_RESP_ADDR_IDX          (0U)    /**< 异常响应从机地址 */
#define MODBUS_EX_RESP_FUNC_CODE_IDX     (1U)    /**< 异常响应功能码（原功能码|0x80） */
#define MODBUS_EX_RESP_EX_CODE_IDX       (2U)    /**< 异常响应错误码 */

/* Max Limits */
#define MODBUS_MAX_READ_REGS             (125U)  /**< 最大读取寄存器数量 */

/* ================= Modbus Register Address Map ================= */
/* Frozen holding-register map for the first 0x06 implementation round.
 * External devices are not connected yet, so sensor/status slots still expose
 * demo defaults. Only the servo target register is writable for now. */
#define MODBUS_REG_ADDR_SENSOR_TEMP      (0U)    /**< 传感器温度寄存器地址 */
#define MODBUS_REG_ADDR_SENSOR_HUMIDITY  (1U)    /**< 传感器湿度寄存器地址 */
#define MODBUS_REG_ADDR_DEVICE_STATUS    (2U)    /**< 设备状态寄存器地址（只读） */
#define MODBUS_REG_ADDR_SYSTEM_FLAGS     (3U)    /**< 系统标志寄存器地址 */
#define MODBUS_REG_ADDR_SERVO_TARGET     (4U)    /**< 舵机目标角度寄存器地址（180° 舵机） */
#define MODBUS_REG_ADDR_SERVO_SPEED      (5U)    /**< 舵机速度寄存器地址（360° 舵机） */
#define MODBUS_REG_ADDR_RESERVED_BEGIN   (6U)    /**< 保留区域起始地址 */
#define MODBUS_REG_ADDR_RESERVED_END     (9U)    /**< 保留区域结束地址 */

typedef void (*ModbusRegisterOnChange_t)(uint16_t old_value, uint16_t new_value);

void Modbus_Init(void);
void Modbus_Process(void);
bool Modbus_ReadHoldingRegister(uint16_t addr, uint16_t *value);
bool Modbus_WriteHoldingRegister(uint16_t addr, uint16_t value);
bool Modbus_RegisterOnChange(uint16_t addr, ModbusRegisterOnChange_t on_change);
bool Modbus_InternalWriteRegister(uint16_t addr, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */
