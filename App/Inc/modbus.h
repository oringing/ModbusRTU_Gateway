// App/Inc/modbus.h
#ifndef __MODBUS_H__
#define __MODBUS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

// Modbus协议相关
#define MODBUS_SLAVE_ADDR                 1      // 从机地址
#define MODBUS_REG_MAX_COUNT             10      // 寄存器最大数量
#define MODBUS_FUNC_READ_HOLDING_REGS    0x03    // 读保持寄存器功能码
#define MODBUS_FUNC_WRITE_SINGLE_REG     0x06    // 写单个寄存器功能码
#define MODBUS_MAX_READ_REGS             125     // Modbus最大一次读取寄存器数量
#define MODBUS_BUFFER_SIZE               128     // 接收缓冲区大小
#define MODBUS_RESPONSE_BUFFER_SIZE      256     // 响应缓冲区大小
#define MODBUS_EXCEPTION_RESPONSE_SIZE   5       // 异常响应帧大小
#define MODBUS_POLL_INTERVAL             10      // Modbus轮询间隔(ms)

void Modbus_Init(void);
void Modbus_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */

