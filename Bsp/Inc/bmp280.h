//Bsp/Inc/bmp280.h
#ifndef __BMP280_H__
#define __BMP280_H__

#include <stdbool.h>
#include <stdint.h>

#include "i2c.h"
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- 设备地址（7 位，不含 R/W 位）----
#define BMP280_I2C_ADDR         0x77U   // BMP280模块IIC地址 （SDO 接 VDDIO）

// ---- 寄存器地址（数据手册第 24-27 页）----
#define BMP280_REG_ID           0xD0U   // 芯片 ID（只读，预期值为 0x58）
#define BMP280_REG_RESET        0xE0U   // 软复位（写 0xB6）
#define BMP280_REG_STATUS       0xF3U   // 状态寄存器
#define BMP280_REG_CTRL_MEAS    0xF4U   // 控制测量（过采样 + 模式）
#define BMP280_REG_CONFIG       0xF5U   // 配置（滤波 + 待机）
#define BMP280_REG_PRESS_MSB    0xF7U   // 压力数据 MSB（起始地址）
#define BMP280_REG_TEMP_MSB     0xFAU   // 温度数据 MSB（起始地址）
#define BMP280_REG_CALIB_START  0x88U   // 校准系数起始地址（24 字节）

// ---- 软复位命令 ----
#define BMP280_RESET_CMD        0xB6U   // 写入 0xE0 触发软复位

// ---- 芯片 ID 预期值 ----
#define BMP280_CHIP_ID          0x58U   // BMP280芯片ID（0x58），用于验证通信

// ---- 运行模式选择 ----
#define BMP280_MEASUREMENT_MODE BMP280_MODE_FORCED   // 默认 forced measurement；可改为 BMP280_MODE_NORMAL

// ---- 测量等待时间 ----
#define BMP280_MEASURE_DELAY_MS 30U             // ×2×4 模式约 20ms，取 30ms 安全

#define BMP280_PRESSURE_SOFT_OFFSET_HPA 65.42f       // 软件调平偏移值，修正硬件I2C读取误差，单位hPa

// ---- 温度过采样配置 ----
#define BMP280_OSRS_T_OFF       (0x00U << 5)    // 跳过温度测量
#define BMP280_OSRS_T_X1        (0x01U << 5)    // ×1（最低功耗）
#define BMP280_OSRS_T_X2        (0x02U << 5)    // ×2
#define BMP280_OSRS_T_X4        (0x03U << 5)    // ×4
#define BMP280_OSRS_T_X8        (0x04U << 5)    // ×8
#define BMP280_OSRS_T_X16       (0x05U << 5)    // ×16（最高精度）

// ---- 压力过采样配置 ----
#define BMP280_OSRS_P_OFF       (0x00U << 2)    // 跳过压力测量
#define BMP280_OSRS_P_X1        (0x01U << 2)    // ×1（最低功耗）
#define BMP280_OSRS_P_X2        (0x02U << 2)    // ×2
#define BMP280_OSRS_P_X4        (0x03U << 2)    // ×4
#define BMP280_OSRS_P_X8        (0x04U << 2)    // ×8
#define BMP280_OSRS_P_X16       (0x05U << 2)    // ×16（最高精度）

// ---- 功率模式配置 ----
#define BMP280_MODE_SLEEP       (0x00U)         // 睡眠模式（不测量）
#define BMP280_MODE_FORCED      (0x01U)         // 强制模式（单次测量后睡眠）
#define BMP280_MODE_NORMAL      (0x03U)         // 正常模式（连续测量）

// ---- IIR 滤波系数 ----
#define BMP280_FILTER_OFF       (0x00U << 2)    // 无滤波
#define BMP280_FILTER_COEFF_2   (0x01U << 2)    // 系数 2
#define BMP280_FILTER_COEFF_4   (0x02U << 2)    // 系数 4
#define BMP280_FILTER_COEFF_8   (0x03U << 2)    // 系数 8
#define BMP280_FILTER_COEFF_16  (0x04U << 2)    // 系数 16

// ---- 待机时间配置 ----
#define BMP280_STANDBY_0_5MS    (0x00U << 5)    // 0.5 ms
#define BMP280_STANDBY_62_5MS   (0x01U << 5)    // 62.5 ms
#define BMP280_STANDBY_125MS    (0x02U << 5)    // 125 ms
#define BMP280_STANDBY_250MS    (0x03U << 5)    // 250 ms
#define BMP280_STANDBY_500MS    (0x04U << 5)    // 500 ms
#define BMP280_STANDBY_1000MS   (0x05U << 5)    // 1000 ms
#define BMP280_STANDBY_2000MS   (0x06U << 5)    // 2000 ms
#define BMP280_STANDBY_4000MS   (0x07U << 5)    // 4000 ms


/**
 * @brief   校准系数结构体（数据手册第 21 页 Table 17）
 * @note    从 0x88-0xA1 读取 24 字节解析得到
 */
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

/**
 * @brief   初始化 BMP280 传感器
 * @return  true  = 初始化成功
 * @return  false = 初始化失败（ID 校验失败或 I2C 通信异常）
 * @note    流程：读 ID → 读校准系数 → 软复位 → 配置滤波 → 配置测量参数
 */
bool BMP280_Init(void);

/**
 * @brief   读取气压和温度数据
 * @param   pressure    输出气压指针（单位：hPa，已补偿到海平面）
 * @param   temperature 输出温度指针（单位：°C）
 * @return  true  = 读取成功
 * @return  false = 读取失败（I2C 通信异常或数据无效）
 * @note    使用 forced mode，每次读取前触发一次测量
 * @warning 温度值来自 BMP280 内部传感器，环境温度建议以 AHT20 为准
 */
bool BMP280_Read(float *pressure, float *temperature);

/**
 * @brief   获取校准系数指针（供调试用）
 * @return  校准系数结构体指针
 * @warning 仅在 BMP280_Init 成功后有效
 */
const BMP280_Calib_t *BMP280_GetCalib(void);

/**
 * @brief   设置海拔高度（用于气压海平面补偿）
 * @param   altitude 海拔高度（单位：米）
 * @note    默认值 BMP280_DEFAULT_ALTITUDE（南宁 76.5 米）
 */
void BMP280_SetAltitude(float altitude);

/**
 * @brief   将绝对气压转换为海平面气压（国际标准大气模型）
 * @param   abs_pressure 绝对气压（hPa）
 * @param   altitude 海拔高度（米）
 * @return  海平面气压（hPa）
 */
float BMP280_ToLocalPressure(float sea_level_pressure, float altitude);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H__ */
