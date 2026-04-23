// App/Inc/modbus.h
#ifndef __MODBUS_H
#define __MODBUS_H

#include <stdint.h>

// MODBUS 协议配置
#define MODBUS_SLAVE_ADDR       0x01
#define MODBUS_RX_TIMEOUT_MS    5       // 帧间隔超时 (用于判断帧结束)
#define MODBUS_REG_MAX_COUNT    10      // 最大寄存器数量

// 硬件配置 (如果以后换引脚，只改这里)
#define MODBUS_UART_INSTANCE    USART1
#define MODBUS_BAUDRATE         115200

void Modbus_Init(void);
void Modbus_Process(void);

#endif /* __MODBUS_H */

