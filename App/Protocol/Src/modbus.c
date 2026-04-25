// App/Protocol/Src/modbus.c
#include "uart.h"
#include "driver_uart.h"
#include "modbus.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t modbus_rx_buffer[MODBUS_BUFFER_SIZE];
static uint8_t modbus_tx_buffer[MODBUS_RESPONSE_BUFFER_SIZE];
static uint16_t holding_regs[MODBUS_REG_MAX_COUNT];

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

void Modbus_Init(void)
{
    for (uint16_t i = 0U; i < MODBUS_REG_MAX_COUNT; i++) {
        holding_regs[i] = 0U;
    }

    /* Default register values for debugging */
    holding_regs[0] = 0x1234U;
    holding_regs[1] = 0x5678U;
    holding_regs[2] = 0xABCDU;
    holding_regs[3] = 0x1234U;
    holding_regs[4] = 0x5000U;
}

void Modbus_Process(void)
{
    if (!BSP_UART_IsFrameReady()) {
        return;
    }

    uint16_t rx_len = BSP_UART_ReadFrame(modbus_rx_buffer, (uint16_t)sizeof(modbus_rx_buffer));
    if (rx_len < 8U) {
        return;
    }

    if (modbus_rx_buffer[0] != MODBUS_SLAVE_ADDR) {
        return;
    }

    uint16_t received_crc = (uint16_t)((uint16_t)modbus_rx_buffer[rx_len - 1U] << 8U) |
                            (uint16_t)modbus_rx_buffer[rx_len - 2U];
    uint16_t calculated_crc = CalcCRC16(modbus_rx_buffer, (uint16_t)(rx_len - 2U));
    if (received_crc != calculated_crc) {
        return;
    }

    uint8_t func_code = modbus_rx_buffer[1];
    if (func_code != MODBUS_FUNC_READ_HOLDING_REGS) {
        return;
    }

    uint16_t start_addr = (uint16_t)((uint16_t)modbus_rx_buffer[2] << 8U) | modbus_rx_buffer[3];
    uint16_t reg_count  = (uint16_t)((uint16_t)modbus_rx_buffer[4] << 8U) | modbus_rx_buffer[5];

    if ((reg_count == 0U) ||
        (reg_count > MODBUS_MAX_READ_REGS) ||
        ((uint32_t)start_addr + (uint32_t)reg_count > (uint32_t)MODBUS_REG_MAX_COUNT)) {
        uint8_t error_response[MODBUS_EXCEPTION_RESPONSE_SIZE];
        error_response[0] = MODBUS_SLAVE_ADDR;
        error_response[1] = (uint8_t)(func_code | 0x80U);
        error_response[2] = 0x02U; /* Illegal data address */

        uint16_t error_crc = CalcCRC16(error_response, 3U);
        error_response[3] = (uint8_t)(error_crc & 0xFFU);
        error_response[4] = (uint8_t)((error_crc >> 8U) & 0xFFU);

        (void)UART_Driver_Send(error_response, MODBUS_EXCEPTION_RESPONSE_SIZE, BSP_UART_TX_TIMEOUT);
        return;
    }

    modbus_tx_buffer[0] = MODBUS_SLAVE_ADDR;
    modbus_tx_buffer[1] = MODBUS_FUNC_READ_HOLDING_REGS;
    modbus_tx_buffer[2] = (uint8_t)(reg_count * 2U);

    for (uint16_t i = 0U; i < reg_count; i++) {
        uint16_t reg = holding_regs[start_addr + i];
        modbus_tx_buffer[3U + (i * 2U)] = (uint8_t)((reg >> 8U) & 0xFFU);
        modbus_tx_buffer[4U + (i * 2U)] = (uint8_t)(reg & 0xFFU);
    }

    uint16_t resp_data_len = (uint16_t)(3U + (reg_count * 2U));
    uint16_t resp_crc = CalcCRC16(modbus_tx_buffer, resp_data_len);
    modbus_tx_buffer[resp_data_len] = (uint8_t)(resp_crc & 0xFFU);
    modbus_tx_buffer[resp_data_len + 1U] = (uint8_t)((resp_crc >> 8U) & 0xFFU);

    (void)UART_Driver_Send(modbus_tx_buffer, (uint16_t)(resp_data_len + 2U), BSP_UART_TX_TIMEOUT);
}

