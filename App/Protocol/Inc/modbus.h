// App/Protocol/Inc/modbus.h
#ifndef __MODBUS_H__
#define __MODBUS_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- 配置宏（可修改）----
#define MODBUS_SLAVE_ADDR (1U)     // Modbus从机地址，可通过拨码开关或NVRAM覆盖
#define MODBUS_REG_MAX_COUNT (10U) // 最大寄存器数量，受RAM限制

// ---- 协议常量（不可修改）----
// 功能码
#define MODBUS_FUNC_READ_HOLDING_REGS (0x03U) // 读保持寄存器功能码
#define MODBUS_FUNC_WRITE_SINGLE_REG (0x06U)  // 写单寄存器功能码

// 帧长度常量
#define MODBUS_RTU_READ_REQ_LEN (8U)         // 读请求帧固定长度(地址+功能码+起始地址+数量+CRC)
#define MODBUS_RTU_WRITE_SINGLE_REQ_LEN (8U) // 写单寄存器请求帧固定长度
#define MODBUS_RTU_WRITE_MULTI_MIN_LEN (11U) // 写多寄存器最小帧长度(预留)
#define MODBUS_CRC_LEN (2U)                  // CRC校验码长度（字节）
#define MODBUS_REG_SIZE_BYTES (2U)           // 单个寄存器占用字节数

// 缓冲区大小
#define MODBUS_BUFFER_SIZE (256U)           // 接收缓冲区大小，需大于最大帧长度
#define MODBUS_RESPONSE_BUFFER_SIZE (256U)  // 发送缓冲区大小，需容纳最大响应帧
#define MODBUS_EXCEPTION_RESPONSE_SIZE (5U) // 异常响应帧长度(地址+功能码|0x80+错误码+CRC)

// 超时配置
#define MODBUS_TX_TIMEOUT_MS (5U)  // 发送超时时间(ms)，避免阻塞任务调度
#define MODBUS_POLL_INTERVAL (10U) // 轮询间隔(ms)，须匹配主机PLC频率

// Modbus异常码
#define MODBUS_EX_ILLEGAL_FUNCTION (0x01U)     // 非法功能码
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS (0x02U) // 非法数据地址
#define MODBUS_EX_ILLEGAL_DATA_VALUE (0x03U)   // 非法数据值

// ---- 可调参数（性能调优）----
#define MODBUS_MAX_READ_REGS (125U) // 单次最大读取寄存器数量，Modbus协议上限

// ================= Modbus RTU 帧字段索引 =================
// 请求帧通用索引
#define MODBUS_REQ_ADDR_IDX (0U)      // 从机地址索引
#define MODBUS_REQ_FUNC_CODE_IDX (1U) // 功能码索引

// 读保持寄存器请求(0x03)
#define MODBUS_REQ_START_ADDR_HIGH_IDX (2U) // 起始地址高字节
#define MODBUS_REQ_START_ADDR_LOW_IDX (3U)  // 起始地址低字节
#define MODBUS_REQ_REG_COUNT_HIGH_IDX (4U)  // 寄存器数量高字节
#define MODBUS_REQ_REG_COUNT_LOW_IDX (5U)   // 寄存器数量低字节

// 写单寄存器请求(0x06)
#define MODBUS_REQ_REG_ADDR_HIGH_IDX (2U)  // 寄存器地址高字节
#define MODBUS_REQ_REG_ADDR_LOW_IDX (3U)   // 寄存器地址低字节
#define MODBUS_REQ_REG_VALUE_HIGH_IDX (4U) // 寄存器值高字节
#define MODBUS_REQ_REG_VALUE_LOW_IDX (5U)  // 寄存器值低字节

// 响应帧索引
#define MODBUS_RESP_ADDR_IDX (0U)       // 响应帧从机地址
#define MODBUS_RESP_FUNC_CODE_IDX (1U)  // 响应帧功能码
#define MODBUS_RESP_BYTE_COUNT_IDX (2U) // 响应帧字节计数
#define MODBUS_RESP_DATA_START_IDX (3U) // 响应帧数据起始位置

// 异常响应帧索引
#define MODBUS_EX_RESP_ADDR_IDX (0U)      // 异常响应从机地址
#define MODBUS_EX_RESP_FUNC_CODE_IDX (1U) // 异常响应功能码(原功能码|0x80)
#define MODBUS_EX_RESP_EX_CODE_IDX (2U)   // 异常响应错误码
#define MODBUS_CRC_LOW_BYTE_IDX (3U)      // CRC低字节索引（适用于固定长度短帧）
#define MODBUS_CRC_HIGH_BYTE_IDX (4U)     // CRC高字节索引（适用于固定长度短帧）

// ---- 地址映射表（与硬件图同步）----
#define MODBUS_REG_ADDR_SENSOR_TEMP (0U)     // AHT20温度寄存器地址(预留，待接入)
#define MODBUS_REG_ADDR_SENSOR_HUMIDITY (1U) // AHT20湿度寄存器地址(预留，待接入)
#define MODBUS_REG_ADDR_SENSOR_PRESSURE (2U) // BMP280气压寄存器地址(预留，待接入)
#define MODBUS_REG_ADDR_DEVICE_STATUS (3U)   // 系统状态寄存器(心跳+故障标志，只读)
#define MODBUS_REG_ADDR_SERVO_TARGET (4U)    // 180°舵机目标角度寄存器地址(可写，单位:度)
#define MODBUS_REG_ADDR_SERVO_SPEED (5U)     // 360°舵机速度寄存器地址(可写，范围:0-255)
#define MODBUS_REG_ADDR_RESERVED_BEGIN (6U)  // 保留区域起始地址
#define MODBUS_REG_ADDR_RESERVED_END (9U)    // 保留区域结束地址

typedef void (*ModbusRegisterOnChange_t)(uint16_t old_value, uint16_t new_value);

/**
 * @brief   初始化Modbus协议栈及寄存器映射
 * @warning 必须在FreeRTOS启动后调用，依赖osMutexCreate
 */
void Modbus_Init(void);

/**
 * @brief   周期性处理Modbus接收数据并分发功能码
 * @warning 需在UART任务中周期性调用，建议间隔≤10ms
 */
void Modbus_Process(void);

/**
 * @brief   读取保持寄存器当前值
 * @param   addr 寄存器地址，范围[0, MODBUS_REG_MAX_COUNT-1]
 * @param   value 输出参数，存储读取到的寄存器值
 * @return  true-读取成功；false-地址越界或互斥锁获取失败
 */
bool Modbus_ReadHoldingRegister(uint16_t addr, uint16_t* value);

/**
 * @brief   写入保持寄存器并触发变化回调
 * @param   addr 寄存器地址，范围[0, MODBUS_REG_MAX_COUNT-1]
 * @param   value 待写入的寄存器值
 * @return  true-写入成功；false-地址越界/只读寄存器/互斥锁获取失败
 */
bool Modbus_WriteHoldingRegister(uint16_t addr, uint16_t value);

/**
 * @brief   注册寄存器值变化回调函数
 * @param   addr 寄存器地址，范围[0, MODBUS_REG_MAX_COUNT-1]
 * @param   on_change 回调函数指针，值为NULL时清除回调
 * @return  true-注册成功；false-地址越界或互斥锁获取失败
 */
bool Modbus_RegisterOnChange(uint16_t addr, ModbusRegisterOnChange_t on_change);

/**
 * @brief   内部写入接口，绕过只读检查
 * @param   addr 寄存器地址，范围[0, MODBUS_REG_MAX_COUNT-1]
 * @param   value 待写入的寄存器值
 * @return  true-写入成功；false-地址越界或互斥锁获取失败
 * @warning 仅供系统任务使用（如心跳更新），外部禁止调用
 */
bool Modbus_InternalWriteRegister(uint16_t addr, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */
