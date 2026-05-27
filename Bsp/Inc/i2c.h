// Bsp/Inc/i2c.h
#ifndef __I2C_H__
#define __I2C_H__

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- I2C1 硬件配置（不可修改）----
#define BSP_I2C_INSTANCE I2C1         // I2C实例（I2C1）
#define BSP_I2C_GPIO_PORT GPIOB       // I2C GPIO端口（PB6/PB7）
#define BSP_I2C_SCL_PIN GPIO_PIN_6    // SCL引脚（PB6）
#define BSP_I2C_SDA_PIN GPIO_PIN_7    // SDA引脚（PB7）
#define BSP_I2C_SPEED 100000U         // I2C时钟频率（100kHz，标准模式）
#define BSP_I2C_TIMEOUT_MS 100U       // I2C操作超时时间（ms）

/**
 * @brief   初始化I2C1硬件及中断
 * @warning 需在FreeRTOS启动前调用，内部会开启NVIC中断
 */
void BSP_I2C_Init(void);

/**
 * @brief   I2C主机发送数据
 * @param   dev_addr 从机地址（7位，不含R/W位）
 * @param   data 待发送数据指针
 * @param   len 数据长度(字节)
 * @return  true=发送成功, false=超时或硬件错误
 * @warning 同步阻塞发送，不建议在实时任务中频繁调用
 */
bool BSP_I2C_Master_Transmit(uint8_t dev_addr, const uint8_t* data, uint16_t len);

/**
 * @brief   I2C主机接收数据
 * @param   dev_addr 从机地址（7位，不含R/W位）
 * @param   data 输出缓冲区指针
 * @param   len 待接收数据长度(字节)
 * @return  true=接收成功, false=超时或硬件错误
 * @warning 同步阻塞接收，需确保从机已准备好数据
 */
bool BSP_I2C_Master_Receive(uint8_t dev_addr, uint8_t* data, uint16_t len);

/**
 * @brief   I2C内存读操作（先写寄存器地址，再读数据）
 * @param   dev_addr 从机地址（7位）
 * @param   mem_addr 内存地址指针
 * @param   mem_addr_size 内存地址长度（1或2字节）
 * @param   data 输出缓冲区指针
 * @param   len 待读取数据长度
 * @return  true=读取成功, false=失败
 * @warning 适用于AHT20/BMP280等带寄存器映射的传感器
 */
bool BSP_I2C_Mem_Read(uint8_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size,
                      uint8_t* data, uint16_t len);

/**
 * @brief   I2C内存写操作（先写寄存器地址，再写数据）
 * @param   dev_addr 从机地址（7位）
 * @param   mem_addr 内存地址
 * @param   mem_addr_size 内存地址长度（1或2字节）
 * @param   data 待写入数据指针
 * @param   len 数据长度
 * @return  true=写入成功, false=失败
 */
bool BSP_I2C_Mem_Write(uint8_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size,
                       const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */
