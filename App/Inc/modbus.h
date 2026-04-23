// App/Inc/modbus.h
#ifndef __MODBUS_H__
#define __MODBUS_H__

#include <stdint.h>
#include <stdbool.h>

// 定义 Modbus 从机地址
#define MODBUS_SLAVE_ADDR     1
#define MODBUS_REG_MAX_COUNT  10

// 定义功能码
#define MODBUS_FUNC_READ_HOLDING_REGS  0x03
#define MODBUS_FUNC_WRITE_SINGLE_REG   0x06

void Modbus_Init(void);
void Modbus_Process(void);

#endif /* __MODBUS_H__ */

