// App/Protocol/Src/modbus.c
#include "modbus.h"
#include "cmsis_os.h"
#include "driver_servo.h"
#include "driver_uart.h"
#include "task.h"
#include "uart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint8_t   modbus_rx_buffer[MODBUS_BUFFER_SIZE];          // 接收缓冲区，仅UART中断写入
static uint8_t   modbus_tx_buffer[MODBUS_RESPONSE_BUFFER_SIZE]; // 发送缓冲区，任务间共享需互斥保护
static osMutexId s_modbus_reg_mutex = NULL;                     // 寄存器数组互斥锁，保护holding_regs并发访问

typedef struct {
    uint16_t                 value;         // 当前值
    uint16_t                 default_value; // 默认值（上电复位值）
    bool                     read_only;     // 只读标志，true时禁止Modbus写操作
    ModbusRegisterOnChange_t on_change;     // 值变化回调，仅在值改变时触发
} ModbusRegister_t;

static ModbusRegister_t holding_regs[MODBUS_REG_MAX_COUNT]; // 保持寄存器数组，受s_modbus_reg_mutex保护
//（舵机寄存器默认值超出合法范围, 强制主机首次写入合法角度后舵机才能使用）
static const uint16_t s_default_regs[] = {
    0x0000U,  // 对应地址 0x0000: AHT20 温度（预留）
    0x1000U,  // 对应地址 0x0001: AHT20 湿度（预留）
    0x2000U,  // 对应地址 0x0002: BMP280 气压（预留）
    0x3000U,  // 对应地址 0x0003: 系统状态寄存器（心跳+标志位）
    0x4000U,   // 对应地址 0x0004: 180° 舵机目标角度
    0x5000U   // 对应地址 0x0005: 360° 舵机速度/方向
};
//剩余地址0x0007-0x0009寄存器值默认为0x0000

// 内部函数声明
static bool Modbus_ValidateFrame(const uint8_t* frame, uint16_t frame_len, uint8_t* func_code);
static void Modbus_HandleReadHoldingRegs(const uint8_t* frame, uint16_t frame_len);
static void Modbus_BuildReadResponse(uint16_t start_addr, uint16_t reg_count);
static void Modbus_SendException(uint8_t func_code, uint8_t exception_code);
static void Modbus_EnsureRegisterMutex(void);
static bool Modbus_LockRegisters(void);
static void Modbus_UnlockRegisters(void);

static void Modbus_OnRegisterChanged(uint16_t old_value, uint16_t new_value);
static void Modbus_OnServoTargetChanged(uint16_t old_value, uint16_t new_value);
static void Modbus_OnServoSpeedChanged(uint16_t old_value, uint16_t new_value);

void Modbus_Init(void) {
    Modbus_EnsureRegisterMutex();

    for (uint16_t i = 0U; i < MODBUS_REG_MAX_COUNT; i++) {
        holding_regs[i].value = 0U;
        holding_regs[i].default_value = 0U;
        holding_regs[i].read_only = false;
        holding_regs[i].on_change = Modbus_OnRegisterChanged;
    }

    for (uint16_t i = 0U; i < (uint16_t)(sizeof(s_default_regs) / sizeof(s_default_regs[0])) &&
                          i < MODBUS_REG_MAX_COUNT;
         i++) {
        holding_regs[i].value = s_default_regs[i];
        holding_regs[i].default_value = s_default_regs[i];
    }

    holding_regs[MODBUS_REG_ADDR_DEVICE_STATUS].read_only = true;

    Modbus_RegisterOnChange(MODBUS_REG_ADDR_SERVO_TARGET, Modbus_OnServoTargetChanged);
    Modbus_RegisterOnChange(MODBUS_REG_ADDR_SERVO_SPEED, Modbus_OnServoSpeedChanged);

    Servo_Driver_Init();
}

static void Modbus_OnRegisterChanged(uint16_t old_value, uint16_t new_value) {
    (void)old_value;
    (void)new_value;
}

// 懒加载创建互斥锁，避免FreeRTOS启动顺序依赖
static void Modbus_EnsureRegisterMutex(void) {
    if (s_modbus_reg_mutex != NULL) {
        return;
    }

    osMutexDef(ModbusRegisterMutex);
    s_modbus_reg_mutex = osMutexCreate(osMutex(ModbusRegisterMutex));
}

// 获取寄存器互斥锁，失败返回false防止死锁
static bool Modbus_LockRegisters(void) {
    Modbus_EnsureRegisterMutex();

    if (s_modbus_reg_mutex == NULL) {
        return false;
    }

    return (osMutexWait(s_modbus_reg_mutex, osWaitForever) == osOK);
}

static void Modbus_UnlockRegisters(void) {
    if (s_modbus_reg_mutex != NULL) {
        osMutexRelease(s_modbus_reg_mutex);
    }
}

// CRC16直接计算法，多项式0xA001，节省Flash空间（查表法需512B ROM）
static uint16_t CalcCRC16(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0U; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0U; j < 8U; j++) {
            if ((crc & 0x0001U) != 0U) {
                crc >>= 1U;
                crc ^= 0xA001U;
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

// 构建并发送异常响应帧（5字节：地址+功能码|0x80+错误码+CRC）
static void Modbus_SendException(uint8_t func_code, uint8_t exception_code) {
    uint8_t error_response[MODBUS_EXCEPTION_RESPONSE_SIZE];
    error_response[MODBUS_EX_RESP_ADDR_IDX] = MODBUS_SLAVE_ADDR;
    error_response[MODBUS_EX_RESP_FUNC_CODE_IDX] = (uint8_t)(func_code | 0x80U);
    error_response[MODBUS_EX_RESP_EX_CODE_IDX] = exception_code;

    uint16_t error_crc = CalcCRC16(error_response, 3U);
    error_response[MODBUS_CRC_LOW_BYTE_IDX] = (uint8_t)(error_crc & 0xFFU);
    error_response[MODBUS_CRC_HIGH_BYTE_IDX] = (uint8_t)((error_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(error_response, MODBUS_EXCEPTION_RESPONSE_SIZE, BSP_UART_TX_TIMEOUT);
}

// 验证帧合法性：地址匹配、最小长度、CRC校验（小端序）
static bool Modbus_ValidateFrame(const uint8_t* frame, uint16_t frame_len, uint8_t* func_code) {
    if (frame == NULL || func_code == NULL) {
        return false;
    }

    if (frame_len < MODBUS_RTU_READ_REQ_LEN) {
        return false;
    }

    if (frame[MODBUS_REQ_ADDR_IDX] != MODBUS_SLAVE_ADDR) {
        return false;
    }

    uint16_t received_crc =
        (uint16_t)((uint16_t)frame[frame_len - 1U] << 8U) | (uint16_t)frame[frame_len - 2U];
    uint16_t calculated_crc = CalcCRC16(frame, (uint16_t)(frame_len - MODBUS_CRC_LEN));
    if (received_crc != calculated_crc) {
        return false;
    }

    *func_code = frame[MODBUS_REQ_FUNC_CODE_IDX];
    return true;
}

// 构建0x03响应帧：填充寄存器数据（大端序）并附加CRC
static void Modbus_BuildReadResponse(uint16_t start_addr, uint16_t reg_count) {
    if (!Modbus_LockRegisters()) {
        Modbus_SendException(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    modbus_tx_buffer[MODBUS_RESP_ADDR_IDX] = MODBUS_SLAVE_ADDR;
    modbus_tx_buffer[MODBUS_RESP_FUNC_CODE_IDX] = MODBUS_FUNC_READ_HOLDING_REGS;
    modbus_tx_buffer[MODBUS_RESP_BYTE_COUNT_IDX] = (uint8_t)(reg_count * MODBUS_REG_SIZE_BYTES);

    for (uint16_t i = 0U; i < reg_count; i++) {
        uint16_t reg = holding_regs[start_addr + i].value;
        modbus_tx_buffer[MODBUS_RESP_DATA_START_IDX + (i * MODBUS_REG_SIZE_BYTES)] =
            (uint8_t)((reg >> 8U) & 0xFFU);
        modbus_tx_buffer[MODBUS_RESP_DATA_START_IDX + 1U + (i * MODBUS_REG_SIZE_BYTES)] =
            (uint8_t)(reg & 0xFFU);
    }

    Modbus_UnlockRegisters();

    uint16_t resp_data_len =
        (uint16_t)(MODBUS_RESP_DATA_START_IDX + (reg_count * MODBUS_REG_SIZE_BYTES));
    uint16_t resp_crc = CalcCRC16(modbus_tx_buffer, resp_data_len);
    modbus_tx_buffer[resp_data_len] = (uint8_t)(resp_crc & 0xFFU);
    modbus_tx_buffer[resp_data_len + 1U] = (uint8_t)((resp_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(modbus_tx_buffer, (uint16_t)(resp_data_len + MODBUS_CRC_LEN),
                           BSP_UART_TX_TIMEOUT);
}

// 处理0x03请求：解析地址数量，边界检查后调用响应构建
static void Modbus_HandleReadHoldingRegs(const uint8_t* frame, uint16_t frame_len) {
    if (frame == NULL || frame_len != MODBUS_RTU_READ_REQ_LEN) {
        return;
    }

    uint16_t start_addr = (uint16_t)((uint16_t)frame[MODBUS_REQ_START_ADDR_HIGH_IDX] << 8U) |
                          frame[MODBUS_REQ_START_ADDR_LOW_IDX];
    uint16_t reg_count = (uint16_t)((uint16_t)frame[MODBUS_REQ_REG_COUNT_HIGH_IDX] << 8U) |
                         frame[MODBUS_REQ_REG_COUNT_LOW_IDX];

    if ((reg_count == 0U) || (reg_count > MODBUS_MAX_READ_REGS)) {
        Modbus_SendException(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    // 防止整数溢出和数组越界
    if (start_addr >= MODBUS_REG_MAX_COUNT || reg_count > MODBUS_REG_MAX_COUNT ||
        (uint32_t)start_addr + (uint32_t)reg_count > (uint32_t)MODBUS_REG_MAX_COUNT) {
        Modbus_SendException(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    Modbus_BuildReadResponse(start_addr, reg_count);
}

// 处理0x06请求：解析地址值，调用公共写接口并回显响应
static void Modbus_HandleWriteSingleReg(const uint8_t* frame, uint16_t frame_len) {
    if (frame == NULL || frame_len != MODBUS_RTU_WRITE_SINGLE_REQ_LEN) {
        return;
    }

    uint16_t reg_addr = (uint16_t)((uint16_t)frame[MODBUS_REQ_REG_ADDR_HIGH_IDX] << 8U) |
                        frame[MODBUS_REQ_REG_ADDR_LOW_IDX];
    uint16_t reg_value = (uint16_t)((uint16_t)frame[MODBUS_REQ_REG_VALUE_HIGH_IDX] << 8U) |
                         frame[MODBUS_REQ_REG_VALUE_LOW_IDX];

    if (reg_addr >= MODBUS_REG_MAX_COUNT) {
        Modbus_SendException(MODBUS_FUNC_WRITE_SINGLE_REG, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    if (!Modbus_WriteHoldingRegister(reg_addr, reg_value)) {
        Modbus_SendException(MODBUS_FUNC_WRITE_SINGLE_REG, MODBUS_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    // 0x06成功响应：原样回显请求帧（Modbus规范要求）
    modbus_tx_buffer[MODBUS_RESP_ADDR_IDX] = MODBUS_SLAVE_ADDR;
    modbus_tx_buffer[MODBUS_RESP_FUNC_CODE_IDX] = MODBUS_FUNC_WRITE_SINGLE_REG;
    modbus_tx_buffer[MODBUS_REQ_REG_ADDR_HIGH_IDX] = (uint8_t)((reg_addr >> 8U) & 0xFFU);
    modbus_tx_buffer[MODBUS_REQ_REG_ADDR_LOW_IDX] = (uint8_t)(reg_addr & 0xFFU);
    modbus_tx_buffer[MODBUS_REQ_REG_VALUE_HIGH_IDX] = (uint8_t)((reg_value >> 8U) & 0xFFU);
    modbus_tx_buffer[MODBUS_REQ_REG_VALUE_LOW_IDX] = (uint8_t)(reg_value & 0xFFU);

    uint16_t resp_crc =
        CalcCRC16(modbus_tx_buffer, MODBUS_RTU_WRITE_SINGLE_REQ_LEN - MODBUS_CRC_LEN);
    modbus_tx_buffer[MODBUS_RTU_WRITE_SINGLE_REQ_LEN - 2U] = (uint8_t)(resp_crc & 0xFFU);
    modbus_tx_buffer[MODBUS_RTU_WRITE_SINGLE_REQ_LEN - 1U] = (uint8_t)((resp_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(modbus_tx_buffer, MODBUS_RTU_WRITE_SINGLE_REQ_LEN, BSP_UART_TX_TIMEOUT);
}

bool Modbus_ReadHoldingRegister(uint16_t addr, uint16_t* value) {
    if (value == NULL || addr >= MODBUS_REG_MAX_COUNT) {
        return false;
    }

    if (!Modbus_LockRegisters()) {
        return false;
    }

    *value = holding_regs[addr].value;
    Modbus_UnlockRegisters();
    return true;
}

// 原子性写入寄存器，解锁后执行回调避免死锁
bool Modbus_WriteHoldingRegister(uint16_t addr, uint16_t value) {
    uint16_t                 old_value = 0U;
    ModbusRegisterOnChange_t on_change = NULL;

    if (addr >= MODBUS_REG_MAX_COUNT) {
        return false;
    }

    // 舵机目标角度范围校验（0-180度）
    if (addr == MODBUS_REG_ADDR_SERVO_TARGET && value > MODBUS_SERVO_TARGET_MAX) {
        return false;
    }

    // 舵机速度范围校验（0-255）
    if (addr == MODBUS_REG_ADDR_SERVO_SPEED && value > MODBUS_SERVO_SPEED_MAX) {
        return false;
    }

    if (!Modbus_LockRegisters()) {
        return false;
    }

    if (holding_regs[addr].read_only) {
        Modbus_UnlockRegisters();
        return false;
    }

    old_value = holding_regs[addr].value;
    holding_regs[addr].value = value;
    on_change = holding_regs[addr].on_change;
    Modbus_UnlockRegisters();

    if (on_change != NULL && old_value != value) {
        on_change(old_value, value);
    }
    return true;
}

// 内部写入接口，绕过只读检查（仅供系统任务更新心跳等只读寄存器）
bool Modbus_InternalWriteRegister(uint16_t addr, uint16_t value) {
    if (addr >= MODBUS_REG_MAX_COUNT) {
        return false;
    }

    if (!Modbus_LockRegisters()) {
        return false;
    }

    holding_regs[addr].value = value;
    Modbus_UnlockRegisters();

    return true;
}

bool Modbus_RegisterOnChange(uint16_t addr, ModbusRegisterOnChange_t on_change) {
    if (addr >= MODBUS_REG_MAX_COUNT) {
        return false;
    }

    if (!Modbus_LockRegisters()) {
        return false;
    }

    holding_regs[addr].on_change = on_change;
    Modbus_UnlockRegisters();
    return true;
}

// 周期性处理接收数据，需在UART任务中调用（建议间隔≤10ms）
void Modbus_Process(void) {
    uint16_t rx_len = UART_Driver_Receive(modbus_rx_buffer, (uint16_t)sizeof(modbus_rx_buffer), 0U);
    uint8_t  func_code = 0U;

    if (rx_len == 0U) {
        return;
    }

    if (!Modbus_ValidateFrame(modbus_rx_buffer, rx_len, &func_code)) {
        return;
    }

    switch (func_code) {
    case MODBUS_FUNC_READ_HOLDING_REGS:
        Modbus_HandleReadHoldingRegs(modbus_rx_buffer, rx_len);
        break;
    case MODBUS_FUNC_WRITE_SINGLE_REG:
        Modbus_HandleWriteSingleReg(modbus_rx_buffer, rx_len);
        break;
    default:
        Modbus_SendException(func_code, MODBUS_EX_ILLEGAL_FUNCTION);
        break;
    }
}

// 舵机目标角度回调（通道1-PA6，180°舵机，范围0-180度）
static void Modbus_OnServoTargetChanged(uint16_t old_value, uint16_t new_value) {
    (void)old_value;
    Servo_Driver_SetAngle(1U, new_value);
}

// 360°舵机速度回调（通道2-PA7，连续旋转，范围0-255）
static void Modbus_OnServoSpeedChanged(uint16_t old_value, uint16_t new_value) {
    (void)old_value;
    // 死区保护已在驱动层实现，此处直接调用
    Servo_Driver_SetSpeed(2U, (uint8_t)new_value);
}
