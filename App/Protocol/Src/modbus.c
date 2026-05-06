// App/Protocol/Src/modbus.c
#include "uart.h"
#include "driver_uart.h"
#include "modbus.h"
#include "cmsis_os.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static uint8_t modbus_rx_buffer[MODBUS_BUFFER_SIZE];
static uint8_t modbus_tx_buffer[MODBUS_RESPONSE_BUFFER_SIZE];
static osMutexId s_modbus_reg_mutex = NULL;
typedef struct {
    uint16_t value;
    uint16_t default_value;
    bool read_only;
    ModbusRegisterOnChange_t on_change;
} ModbusRegister_t;

static ModbusRegister_t holding_regs[MODBUS_REG_MAX_COUNT];
static const uint16_t s_default_regs[] = {
    0x1234U, 0x5678U, 0xABCDU, 0x1234U, 0x5000U
};

static bool Modbus_ValidateFrame(const uint8_t *frame, uint16_t frame_len, uint8_t *func_code);
static void Modbus_HandleReadHoldingRegs(const uint8_t *frame, uint16_t frame_len);
static void Modbus_BuildReadResponse(uint16_t start_addr, uint16_t reg_count);
static void Modbus_EnsureRegisterMutex(void);
static bool Modbus_LockRegisters(void);
static void Modbus_UnlockRegisters(void);

static void Modbus_OnRegisterChanged(uint16_t old_value, uint16_t new_value)
{
    (void)old_value;
    (void)new_value;
}

static void Modbus_EnsureRegisterMutex(void)
{
    if (s_modbus_reg_mutex != NULL) {
        return;
    }

    osMutexDef(ModbusRegisterMutex);
    s_modbus_reg_mutex = osMutexCreate(osMutex(ModbusRegisterMutex));
}

static bool Modbus_LockRegisters(void)
{
    Modbus_EnsureRegisterMutex();
    
    // 即使mutex创建失败也返回false
    if (s_modbus_reg_mutex == NULL) {
        return false;
    }

    return (osMutexWait(s_modbus_reg_mutex, osWaitForever) == osOK);
}

static void Modbus_UnlockRegisters(void)
{
    if (s_modbus_reg_mutex != NULL) {
        osMutexRelease(s_modbus_reg_mutex);
    }
}

/**
 * @brief Calculate standard Modbus RTU CRC16.
 *
 * The frame stores CRC in low-byte-first order. This helper returns the
 * combined 16-bit value so callers can split it when serializing a response.
 */
static uint16_t CalcCRC16(const uint8_t *data, uint16_t len)
{
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

static void Modbus_SendException(uint8_t func_code, uint8_t exception_code)
{
    uint8_t error_response[MODBUS_EXCEPTION_RESPONSE_SIZE];
    error_response[0] = MODBUS_SLAVE_ADDR;
    error_response[1] = (uint8_t)(func_code | 0x80U);
    error_response[2] = exception_code;

    uint16_t error_crc = CalcCRC16(error_response, 3U);
    error_response[3] = (uint8_t)(error_crc & 0xFFU);
    error_response[4] = (uint8_t)((error_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(error_response, MODBUS_EXCEPTION_RESPONSE_SIZE, BSP_UART_TX_TIMEOUT);
}

static bool Modbus_ValidateFrame(const uint8_t *frame, uint16_t frame_len, uint8_t *func_code)
{
    if (frame == NULL || func_code == NULL) {
        return false;
    }

    if (frame_len < MODBUS_RTU_READ_REQ_LEN) {
        return false;
    }

    if (frame[0] != MODBUS_SLAVE_ADDR) {
        return false;
    }

    /* Modbus RTU CRC in frame order: low byte first, high byte second in the frame.
     * But when recombining to 16-bit value: (high_byte << 8) | low_byte */
    uint16_t received_crc = (uint16_t)((uint16_t)frame[frame_len - 1U] << 8U) |
                            (uint16_t)frame[frame_len - 2U];
    uint16_t calculated_crc = CalcCRC16(frame, (uint16_t)(frame_len - 2U));
    if (received_crc != calculated_crc) {
        return false;
    }

    *func_code = frame[1];
    return true;
}

static void Modbus_BuildReadResponse(uint16_t start_addr, uint16_t reg_count)
{
    if (!Modbus_LockRegisters()) {
        Modbus_SendException(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    modbus_tx_buffer[0] = MODBUS_SLAVE_ADDR;
    modbus_tx_buffer[1] = MODBUS_FUNC_READ_HOLDING_REGS;
    modbus_tx_buffer[2] = (uint8_t)(reg_count * 2U);

    for (uint16_t i = 0U; i < reg_count; i++) {
        uint16_t reg = holding_regs[start_addr + i].value;
        modbus_tx_buffer[3U + (i * 2U)] = (uint8_t)((reg >> 8U) & 0xFFU);
        modbus_tx_buffer[4U + (i * 2U)] = (uint8_t)(reg & 0xFFU);
    }

    Modbus_UnlockRegisters();

    uint16_t resp_data_len = (uint16_t)(3U + (reg_count * 2U));
    uint16_t resp_crc = CalcCRC16(modbus_tx_buffer, resp_data_len);
    modbus_tx_buffer[resp_data_len] = (uint8_t)(resp_crc & 0xFFU);
    modbus_tx_buffer[resp_data_len + 1U] = (uint8_t)((resp_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(modbus_tx_buffer, (uint16_t)(resp_data_len + 2U), BSP_UART_TX_TIMEOUT);
}

static void Modbus_HandleReadHoldingRegs(const uint8_t *frame, uint16_t frame_len)
{
    /* Current implementation only supports fixed-length 0x03 request frame:
     * addr(1) + func(1) + start(2) + count(2) + crc(2) = 8 bytes.
     * This filters out self-echoed/invalid frames early. */
    if (frame == NULL || frame_len != MODBUS_RTU_READ_REQ_LEN) {
        return;
    }

    uint16_t start_addr = (uint16_t)((uint16_t)frame[2] << 8U) | frame[3];
    uint16_t reg_count  = (uint16_t)((uint16_t)frame[4] << 8U) | frame[5];

    if ((reg_count == 0U) || (reg_count > MODBUS_MAX_READ_REGS)) {
        Modbus_SendException(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    // 添加额外的边界检查，防止整数溢出和越界访问
    if (start_addr >= MODBUS_REG_MAX_COUNT || 
        reg_count > MODBUS_REG_MAX_COUNT ||
        (uint32_t)start_addr + (uint32_t)reg_count > (uint32_t)MODBUS_REG_MAX_COUNT) {
        Modbus_SendException(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    Modbus_BuildReadResponse(start_addr, reg_count);
}

// 添加处理写单个寄存器的功能
static void Modbus_HandleWriteSingleReg(const uint8_t *frame, uint16_t frame_len)
{
    if (frame == NULL || frame_len != MODBUS_RTU_WRITE_SINGLE_REQ_LEN) {
        return;
    }

    uint16_t reg_addr = (uint16_t)((uint16_t)frame[2] << 8U) | frame[3];
    uint16_t reg_value = (uint16_t)((uint16_t)frame[4] << 8U) | frame[5];

    // 验证寄存器地址是否有效
    if (reg_addr >= MODBUS_REG_MAX_COUNT) {
        Modbus_SendException(MODBUS_FUNC_WRITE_SINGLE_REG, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    // 尝试写入寄存器
    if (!Modbus_WriteHoldingRegister(reg_addr, reg_value)) {
        Modbus_SendException(MODBUS_FUNC_WRITE_SINGLE_REG, MODBUS_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    // 发送成功响应 - 回显相同的请求内容
    modbus_tx_buffer[0] = MODBUS_SLAVE_ADDR;
    modbus_tx_buffer[1] = MODBUS_FUNC_WRITE_SINGLE_REG;
    modbus_tx_buffer[2] = (uint8_t)((reg_addr >> 8) & 0xFF);
    modbus_tx_buffer[3] = (uint8_t)(reg_addr & 0xFF);
    modbus_tx_buffer[4] = (uint8_t)((reg_value >> 8) & 0xFF);
    modbus_tx_buffer[5] = (uint8_t)(reg_value & 0xFF);

    uint16_t resp_crc = CalcCRC16(modbus_tx_buffer, 6U);
    modbus_tx_buffer[6] = (uint8_t)(resp_crc & 0xFFU);
    modbus_tx_buffer[7] = (uint8_t)((resp_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(modbus_tx_buffer, 8U, BSP_UART_TX_TIMEOUT);
}

void Modbus_Init(void)
{
    /* Create the register mutex during startup so later task access does not
     * race on first-time initialization. If creation fails here, runtime calls
     * will retry through Modbus_LockRegisters(). */
    Modbus_EnsureRegisterMutex();

    for (uint16_t i = 0U; i < MODBUS_REG_MAX_COUNT; i++) {
        holding_regs[i].value = 0U;
        holding_regs[i].default_value = 0U;
        holding_regs[i].read_only = false;
        holding_regs[i].on_change = Modbus_OnRegisterChanged;
    }

    /* Load bounded defaults: avoid hard-coded index writes beyond configured map size. */
    for (uint16_t i = 0U;
         i < (uint16_t)(sizeof(s_default_regs) / sizeof(s_default_regs[0])) &&
         i < MODBUS_REG_MAX_COUNT;
         i++) {
        holding_regs[i].value = s_default_regs[i];
        holding_regs[i].default_value = s_default_regs[i];
    }
}

bool Modbus_ReadHoldingRegister(uint16_t addr, uint16_t *value)
{
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

bool Modbus_WriteHoldingRegister(uint16_t addr, uint16_t value)
{
    uint16_t old_value = 0U;
    ModbusRegisterOnChange_t on_change = NULL;

    if (addr >= MODBUS_REG_MAX_COUNT) {
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

bool Modbus_RegisterOnChange(uint16_t addr, ModbusRegisterOnChange_t on_change)
{
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

void Modbus_Process(void)
{
    uint16_t rx_len = UART_Driver_Receive(modbus_rx_buffer, (uint16_t)sizeof(modbus_rx_buffer), 0U);
    uint8_t func_code = 0U;

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
