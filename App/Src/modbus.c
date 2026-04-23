// App/Src/modbus.c

#include "uart.h"
#include <string.h>
#include <stdio.h>

// 定义 Modbus 从机地址
#define MODBUS_SLAVE_ADDR     1
#define MODBUS_REG_MAX_COUNT  10

// 定义功能码
#define MODBUS_FUNC_READ_HOLDING_REGS  0x03
#define MODBUS_FUNC_WRITE_SINGLE_REG   0x06

// 定义保持寄存器数组
static uint16_t holding_regs[MODBUS_REG_MAX_COUNT] = {0};

// CRC16 计算函数
static uint16_t CalcCRC16(uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) { 
                crc >>= 1; 
                crc ^= 0xA001; 
            } 
            else { 
                crc >>= 1; 
            }
        }
    }
    return crc;
}

void Modbus_Init(void)
{
    // 初始化保持寄存器的默认值
    holding_regs[0] = 0x1234; 
    holding_regs[1] = 0x5678;
    holding_regs[2] = 0xABCD;
    holding_regs[3] = 0x1234;
    holding_regs[4] = 5000;
    
    // 其他寄存器保持默认的0值
}

void Modbus_Process(void)
{
    if (BSP_UART_IsFrameReady())
    {
        uint8_t rx_buffer[128];
        uint16_t rx_len = BSP_UART_ReadFrame(rx_buffer, sizeof(rx_buffer));
        
        // 检查帧长度是否足够（至少包含地址+功能码+数据+CRC）
        if (rx_len < 4) {
            return;
        }
        
        // 验证从机地址
        if (rx_buffer[0] != MODBUS_SLAVE_ADDR) {
            return; // 不是发给本设备的请求
        }
        
        // 获取功能码
        uint8_t func_code = rx_buffer[1];
        
        // 计算并验证CRC
        uint16_t received_crc = (rx_buffer[rx_len-1] << 8) | rx_buffer[rx_len-2];
        uint16_t calculated_crc = CalcCRC16(rx_buffer, rx_len - 2);
        
        if (received_crc != calculated_crc) {
            return; // CRC校验失败
        }
        
        // 处理功能码 0x03 (读保持寄存器)
        if (func_code == MODBUS_FUNC_READ_HOLDING_REGS) {
            // 解析起始地址和寄存器数量
            uint16_t start_addr = (rx_buffer[2] << 8) | rx_buffer[3];
            uint16_t reg_count = (rx_buffer[4] << 8) | rx_buffer[5];
            
            // 验证寄存器数量范围（Modbus最大一次读取125个寄存器）
            if (reg_count == 0 || reg_count > 125 || (start_addr + reg_count) > MODBUS_REG_MAX_COUNT) {
                // 发送异常响应：地址超出范围
                uint8_t error_response[5];
                error_response[0] = MODBUS_SLAVE_ADDR;
                error_response[1] = func_code | 0x80;  // 异常功能码
                error_response[2] = 0x02;              // 异常码：非法数据值
                uint16_t error_crc = CalcCRC16(error_response, 3);
                error_response[3] = error_crc & 0xFF;
                error_response[4] = (error_crc >> 8) & 0xFF;
                
                BSP_UART_Send(error_response, 5, 100);
                return;
            }
            
            // 构建响应帧
            uint8_t response_frame[256]; // 足够大的缓冲区
            response_frame[0] = MODBUS_SLAVE_ADDR;
            response_frame[1] = MODBUS_FUNC_READ_HOLDING_REGS;
            response_frame[2] = reg_count * 2; // 字节数 = 寄存器数 * 2
            
            // 将寄存器值复制到响应帧
            for (int i = 0; i < reg_count; i++) {
                response_frame[3 + (i * 2)] = (holding_regs[start_addr + i] >> 8) & 0xFF; // 高字节
                response_frame[4 + (i * 2)] = holding_regs[start_addr + i] & 0xFF;       // 低字节
            }
            
            // 计算响应帧的CRC
            uint16_t resp_data_len = 3 + (reg_count * 2); // 地址 + 功能码 + 字节计数 + 寄存器数据
            uint16_t resp_crc = CalcCRC16(response_frame, resp_data_len);
            response_frame[resp_data_len] = resp_crc & 0xFF;     // CRC低字节
            response_frame[resp_data_len + 1] = (resp_crc >> 8) & 0xFF; // CRC高字节
            
            // 发送响应
            BSP_UART_Send(response_frame, resp_data_len + 2, 100);
        }
        // 如果是其他功能码，暂时不做处理
    }
}

