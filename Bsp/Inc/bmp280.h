// Bsp/Inc/bmp280.h
#ifndef __BMP280_H
#define __BMP280_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- 协议常量（不可修改）----
#define BMP280_I2C_ADDR 0x77U // I2C从机地址（7位，不含 R/W 位）

// ---- 寄存器地址映射表（与硬件图同步）----
#define BMP280_DIG_T1_LSB_REG       0x88U // 校准系数 T1 低字节
#define BMP280_DIG_T1_MSB_REG       0x89U // 校准系数 T1 高字节
#define BMP280_DIG_T2_LSB_REG       0x8AU // 校准系数 T2 低字节
#define BMP280_DIG_T2_MSB_REG       0x8BU // 校准系数 T2 高字节
#define BMP280_DIG_T3_LSB_REG       0x8CU // 校准系数 T3 低字节
#define BMP280_DIG_T3_MSB_REG       0x8DU // 校准系数 T3 高字节
#define BMP280_DIG_P1_LSB_REG       0x8EU // 校准系数 P1 低字节
#define BMP280_DIG_P1_MSB_REG       0x8FU // 校准系数 P1 高字节
#define BMP280_DIG_P2_LSB_REG       0x90U // 校准系数 P2 低字节
#define BMP280_DIG_P2_MSB_REG       0x91U // 校准系数 P2 高字节
#define BMP280_DIG_P3_LSB_REG       0x92U // 校准系数 P3 低字节
#define BMP280_DIG_P3_MSB_REG       0x93U // 校准系数 P3 高字节
#define BMP280_DIG_P4_LSB_REG       0x94U // 校准系数 P4 低字节
#define BMP280_DIG_P4_MSB_REG       0x95U // 校准系数 P4 高字节
#define BMP280_DIG_P5_LSB_REG       0x96U // 校准系数 P5 低字节
#define BMP280_DIG_P5_MSB_REG       0x97U // 校准系数 P5 高字节
#define BMP280_DIG_P6_LSB_REG       0x98U // 校准系数 P6 低字节
#define BMP280_DIG_P6_MSB_REG       0x99U // 校准系数 P6 高字节
#define BMP280_DIG_P7_LSB_REG       0x9AU // 校准系数 P7 低字节
#define BMP280_DIG_P7_MSB_REG       0x9BU // 校准系数 P7 高字节
#define BMP280_DIG_P8_LSB_REG       0x9CU // 校准系数 P8 低字节
#define BMP280_DIG_P8_MSB_REG       0x9DU // 校准系数 P8 高字节
#define BMP280_DIG_P9_LSB_REG       0x9EU // 校准系数 P9 低字节
#define BMP280_DIG_P9_MSB_REG       0x9FU // 校准系数 P9 高字节

#define BMP280_CHIPID_REG           0xD0U       // 芯片 ID 寄存器（只读，固定返回 0x58）
#define BMP280_RESET_REG            0xE0U       // 软复位寄存器（写 0xB6 触发复位）
#define BMP280_STATUS_REG           0xF3U       // 状态寄存器（Bit3=measuring，Bit0=im_update）
#define BMP280_CTRLMEAS_REG         0xF4U       // 测量控制寄存器（过采样 + 模式）
#define BMP280_CONFIG_REG           0xF5U       // 配置寄存器（滤波 + 待机）
#define BMP280_PRESSURE_MSB_REG     0xF7U       // 压力数据 MSB
#define BMP280_PRESSURE_LSB_REG     0xF8U       // 压力数据 LSB
#define BMP280_PRESSURE_XLSB_REG    0xF9U       // 压力数据 XLSB（仅高4位有效）
#define BMP280_TEMPERATURE_MSB_REG  0xFAU       // 温度数据 MSB
#define BMP280_TEMPERATURE_LSB_REG  0xFBU       // 温度数据 LSB
#define BMP280_TEMPERATURE_XLSB_REG 0xFCU       // 温度数据 XLSB（仅高4位有效）

// ---- 运行模式配置宏（可修改）----
#define BMP280_SLEEP_MODE       (0x00U)  // 睡眠模式，不测量
#define BMP280_FORCED_MODE      (0x01U)  // 强制模式，单次测量后自动睡眠
#define BMP280_NORMAL_MODE      (0x03U)  // 正常模式，连续测量

// ---- CONFIG 寄存器配置 ----
#define BMP280_STANDBY_62_5MS       (0x01U << 5) // 待机时间 62.5ms
#define BMP280_FILTER_COEFF_8       (0x03U << 2) // IIR 滤波系数 8

// ---- 可调参数（性能调优）----
#define BMP280_OVERSAMP_SKIPPED     (0x00U) // 跳过测量
#define BMP280_OVERSAMP_1X          (0x01U) // 过采样 ×1
#define BMP280_OVERSAMP_2X          (0x02U) // 过采样 ×2
#define BMP280_OVERSAMP_4X          (0x03U) // 过采样 ×4
#define BMP280_OVERSAMP_8X          (0x04U) // 过采样 ×8
#define BMP280_OVERSAMP_16X         (0x05U) // 过采样 ×16

// ---- 校准数据结构 ----
typedef struct {
    uint16_t dig_T1;    // 温度补偿系数 1（无符号）
    int16_t  dig_T2;    // 温度补偿系数 2（有符号）
    int16_t  dig_T3;    // 温度补偿系数 3（有符号）
    uint16_t dig_P1;    // 压力补偿系数 1（无符号）
    int16_t  dig_P2;    // 压力补偿系数 2（有符号）
    int16_t  dig_P3;    // 压力补偿系数 3（有符号）
    int16_t  dig_P4;    // 压力补偿系数 4（有符号）
    int16_t  dig_P5;    // 压力补偿系数 5（有符号）
    int16_t  dig_P6;    // 压力补偿系数 6（有符号）
    int16_t  dig_P7;    // 压力补偿系数 7（有符号）
    int16_t  dig_P8;    // 压力补偿系数 8（有符号）
    int16_t  dig_P9;    // 压力补偿系数 9（有符号）
    int32_t  t_fine;    // 中间变量，温度补偿传递给压力补偿
} BMP280_Calib_t;

// ---- 错误码枚举 ----
typedef enum {
    BMP280_OK = 0,
    BMP280_ERR_I2C_COMM,       // I2C 通信失败
    BMP280_ERR_CHIP_ID,        // 芯片 ID 错误（期望 0x58）
    BMP280_ERR_CALIB_READ,     // 校准系数读取失败
    BMP280_ERR_RESET_FAIL,     // 软复位失败
    BMP280_ERR_CONFIG_WRITE,   // 配置寄存器写入失败
    BMP280_ERR_STATUS_READ,    // 状态寄存器读取失败
    BMP280_ERR_DATA_READ,      // 原始数据读取失败
    BMP280_ERR_COMP_OVERFLOW   // 补偿计算溢出
} BMP280_Error_t;

/**
 * @brief   初始化 BMP280 传感器并读取校准参数
 * @return  BMP280_Error_t 错误码，BMP280_OK 表示成功
 */
BMP280_Error_t BMP280_Init(void);

/**
 * @brief   读取当前气压和温度
 * @param   pressure 输出气压（单位：hPa）
 * @param   temperature 输出温度（单位：℃）
 * @return  BMP280_Error_t 错误码，BMP280_OK 表示成功
 */
BMP280_Error_t BMP280_Read(float* pressure, float* temperature);

/**
 * @brief 获取校准参数（调试用）
 */
const BMP280_Calib_t* BMP280_GetCalib(void);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */
