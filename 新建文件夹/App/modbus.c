// App/Src/modbus.c
#include "modbus.h"
#include "uart.h"
#include <string.h>

static uint16_t holding_regs[MODBUS_REG_MAX_COUNT] = {0};


// 简单的 CRC16 计算函数（网上有很多现成的，直接复制即可）

static uint16_t CalcCRC16(uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; } 
            else { crc >>= 1; }
        }
    }
    return crc;
}

void Modbus_Init(void) {

}

void Modbus_Process(void) {
    uint8_t rx_buf[128];
    
    // 1. 检查是否有新帧
    if (!BSP_UART_IsFrameReady()) {
        return;
    }

    // 2. 读取数据
    uint16_t len = BSP_UART_ReadFrame(rx_buf, sizeof(rx_buf));
    if (len < 5) return;

    // 3. CRC 校验
    uint16_t recv_crc = (rx_buf[len-1] << 8) | rx_buf[len-2];
    uint16_t calc_crc = CalcCRC16(rx_buf, len - 2);
    
    if (recv_crc != calc_crc) {
        return;
    }

    // 4. 解析地址和功能码
    uint8_t slave_addr = rx_buf[0];
    uint8_t func_code = rx_buf[1];

    if (slave_addr != MODBUS_SLAVE_ADDR) {
        return;
    }

    // 5. 功能码处理
    if (func_code == 0x03) { // 读保持寄存器
        uint8_t tx_buf[9];
        tx_buf[0] = slave_addr;
        tx_buf[1] = func_code;
        tx_buf[2] = 4; // 字节数
        tx_buf[3] = (holding_regs[0] >> 8) & 0xFF;
        tx_buf[4] = holding_regs[0] & 0xFF;
        tx_buf[5] = (holding_regs[1] >> 8) & 0xFF;
        tx_buf[6] = holding_regs[1] & 0xFF;
        
        uint16_t crc = CalcCRC16(tx_buf, 7);
        tx_buf[7] = crc & 0xFF;
        tx_buf[8] = (crc >> 8) & 0xFF;
        
        // 发送响应
        if (BSP_UART_Send(tx_buf, 9, BSP_UART_TX_TIMEOUT)) {
            // 发送成功
        } else {
            // 发送失败，可以记录日志或重试
        }
    }
}

