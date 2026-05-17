// App/Driver/Inc/driver_uart.h
#ifndef __DRIVER_UART_H__
#define __DRIVER_UART_H__

#include "cmsis_os.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   初始化UART驱动层（创建互斥锁）
 * @return  true=成功, false=失败
 * @warning 需在FreeRTOS调度器启动后调用
 */
bool UART_Driver_Init(void);

/**
 * @brief   线程安全发送数据（Modbus响应使用）
 * @param   data 待发送数据指针
 * @param   len 数据长度(字节)
 * @param   timeout 超时时间(ms)
 * @return  true=发送成功, false=超时或互斥锁获取失败
 * @warning 调度器未启动时直接调用BSP层，启动后加锁保护
 */
bool UART_Driver_Send(const uint8_t* data, uint16_t len, uint32_t timeout);

/**
 * @brief   超时轮询接收完整帧
 * @param   buffer 输出缓冲区指针
 * @param   max_len 缓冲区最大容量(字节)
 * @param   timeout 超时时间(ms)，0=非阻塞查询
 * @return  实际读取字节数，0=无数据或超时
 * @warning timeout=0时立即返回，不等待；timeout>0时每1ms轮询一次
 */
uint16_t UART_Driver_Receive(uint8_t* buffer, uint16_t max_len, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_UART_H__ */
