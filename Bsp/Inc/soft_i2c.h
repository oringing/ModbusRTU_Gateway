// Bsp/Inc/soft_i2c.h
#ifndef __SOFT_I2C_H__
#define __SOFT_I2C_H__

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- 硬件引脚映射（与原理图同步）----
#define SOFT_I2C_SDA         GPIO_PIN_7   // PB7，固定硬件连接，不可修改
#define SOFT_I2C_SCL         GPIO_PIN_6   // PB6，固定硬件连接，不可修改
#define SOFT_I2C_PORT        GPIOB

// ---- I2C 电平控制宏（开漏输出，总线空闲时外部上拉至高电平）----
#define SOFT_I2C_SCL_0       HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL, GPIO_PIN_RESET)     // SCL 置低
#define SOFT_I2C_SCL_1       HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SCL, GPIO_PIN_SET)       // SCL 置高
#define SOFT_I2C_SDA_0       HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA, GPIO_PIN_RESET)     // SDA 置低
#define SOFT_I2C_SDA_1       HAL_GPIO_WritePin(SOFT_I2C_PORT, SOFT_I2C_SDA, GPIO_PIN_SET)       // SDA 置高
#define SOFT_I2C_SDA_READ    HAL_GPIO_ReadPin(SOFT_I2C_PORT, SOFT_I2C_SDA)         // 读取 SDA 电平（0 或 1）

/**
 * @brief   初始化软件 I2C 总线
 * @note    将 SDA/SCL 配置为开漏输出，总线初始状态高电平
 * @warning 必须在系统 BSP 初始化阶段调用一次，调用前需确保 GPIO 时钟已使能
 */
void I2C_Bus_Init(void);

/**
 * @brief   设置 I2C 操作失败后的重试间隔
 * @param   ml_sec 重试等待时间（毫秒），传入 0 表示不重试
 */
void Set_I2C_Retry(unsigned short ml_sec);

/**
 * @brief   获取当前 I2C 重试间隔
 * @return  重试时间（毫秒），默认值 55ms
 */
unsigned short Get_I2C_Retry(void);

/**
 * @brief   读取 I2C 寄存器（写寄存器地址，再重启读取连续数据）
 * @param   slave_addr 从机地址（7 位，不含 R/W 位）
 * @param   reg_addr   寄存器起始地址（8 位）
 * @param   len        要读取的字节数（1~32）
 * @param   data_ptr   数据缓冲区指针，长度至少为 len
 * @return  0 = 成功，非 0 = 失败（超时或无应答）
 * @note    用于寄存器式设备（如 BMP280），不支持命令式传感器
 */
int Sensors_I2C_ReadRegister(unsigned char slave_addr, unsigned char reg_addr,
                             unsigned short len, unsigned char *data_ptr);

/**
 * @brief   写入 I2C 寄存器（连续写：设备地址 + 寄存器地址 + 数据）
 * @param   slave_addr 从机地址（7 位，不含 R/W 位）
 * @param   reg_addr   寄存器起始地址（8 位）
 * @param   len        写入字节数（1~32）
 * @param   data_ptr   待写入数据指针
 * @return  0 = 成功，非 0 = 失败
 */
int Sensors_I2C_WriteRegister(unsigned char slave_addr, unsigned char reg_addr,
                              unsigned short len, const unsigned char *data_ptr);

/**
 * @brief   向命令式传感器发送原始命令（无寄存器地址）
 * @param   slave_addr 从机地址（7 位，不含 R/W 位）
 * @param   data       命令字节数组（首字节通常为命令码）
 * @param   len        命令长度（字节数）
 * @return  0 = 成功，非 0 = 失败
 * @note    用于 AHT20 等先写命令再读数据的器件
 */
int Sensors_I2C_WriteCommand(unsigned char slave_addr, const unsigned char *data,
                             unsigned short len);

/**
 * @brief   先写命令再读取数据（AHT20 专用）
 * @param   slave_addr 从机地址（7 位，不含 R/W 位）
 * @param   cmd        单字节命令（如 0x71 读取状态、0xAC 触发测量）
 * @param   data       数据缓冲区指针，长度至少为 len
 * @param   len        要读取的字节数（AHT20 读取测量结果时为 6）
 * @return  0 = 成功，非 0 = 失败
 * @note    底层实现：START + Addr(W) + cmd + RESTART + Addr(R) + read(len)
 */
int Sensors_I2C_ReadCommandData(unsigned char slave_addr, unsigned char cmd,
                                unsigned char *data, unsigned short len);

#ifdef __cplusplus
}
#endif

#endif /* __SOFT_I2C_H__ */
