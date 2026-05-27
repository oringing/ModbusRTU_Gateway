// Bsp/Inc/bmp280.h
#ifndef __BMP280_H
#define __BMP280_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- I2C 地址 ----
#define BMP280_I2C_ADDR              0x77U

// ---- 寄存器地址 ----
#define BMP280_DIG_T1_LSB_REG        0x88U
#define BMP280_DIG_T1_MSB_REG        0x89U
#define BMP280_DIG_T2_LSB_REG        0x8AU
#define BMP280_DIG_T2_MSB_REG        0x8BU
#define BMP280_DIG_T3_LSB_REG        0x8CU
#define BMP280_DIG_T3_MSB_REG        0x8DU
#define BMP280_DIG_P1_LSB_REG        0x8EU
#define BMP280_DIG_P1_MSB_REG        0x8FU
#define BMP280_DIG_P2_LSB_REG        0x90U
#define BMP280_DIG_P2_MSB_REG        0x91U
#define BMP280_DIG_P3_LSB_REG        0x92U
#define BMP280_DIG_P3_MSB_REG        0x93U
#define BMP280_DIG_P4_LSB_REG        0x94U
#define BMP280_DIG_P4_MSB_REG        0x95U
#define BMP280_DIG_P5_LSB_REG        0x96U
#define BMP280_DIG_P5_MSB_REG        0x97U
#define BMP280_DIG_P6_LSB_REG        0x98U
#define BMP280_DIG_P6_MSB_REG        0x99U
#define BMP280_DIG_P7_LSB_REG        0x9AU
#define BMP280_DIG_P7_MSB_REG        0x9BU
#define BMP280_DIG_P8_LSB_REG        0x9CU
#define BMP280_DIG_P8_MSB_REG        0x9DU
#define BMP280_DIG_P9_LSB_REG        0x9EU
#define BMP280_DIG_P9_MSB_REG        0x9FU

#define BMP280_CHIPID_REG            0xD0U
#define BMP280_RESET_REG             0xE0U
#define BMP280_STATUS_REG            0xF3U
#define BMP280_CTRLMEAS_REG          0xF4U
#define BMP280_CONFIG_REG            0xF5U
#define BMP280_PRESSURE_MSB_REG      0xF7U
#define BMP280_PRESSURE_LSB_REG      0xF8U
#define BMP280_PRESSURE_XLSB_REG     0xF9U
#define BMP280_TEMPERATURE_MSB_REG   0xFAU
#define BMP280_TEMPERATURE_LSB_REG   0xFBU
#define BMP280_TEMPERATURE_XLSB_REG  0xFCU

// ---- 运行模式 ----
#define BMP280_SLEEP_MODE            (0x00U)
#define BMP280_FORCED_MODE           (0x01U)
#define BMP280_NORMAL_MODE           (0x03U)

// ---- 过采样 ----
#define BMP280_OVERSAMP_SKIPPED      (0x00U)
#define BMP280_OVERSAMP_1X           (0x01U)
#define BMP280_OVERSAMP_2X           (0x02U)
#define BMP280_OVERSAMP_4X           (0x03U)
#define BMP280_OVERSAMP_8X           (0x04U)
#define BMP280_OVERSAMP_16X          (0x05U)

// ---- 校准数据结构 ----
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    int32_t  t_fine;
} BMP280_Calib_t;

/**
 * @brief 初始化 BMP280
 * @return true=成功, false=失败
 */
bool BMP280_Init(void);

/**
 * @brief 读取气压和温度
 * @param pressure 输出气压 (hPa)
 * @param temperature 输出温度 (℃)
 * @return true=成功, false=失败
 */
bool BMP280_Read(float *pressure, float *temperature);

/**
 * @brief 获取校准参数（调试用）
 */
const BMP280_Calib_t *BMP280_GetCalib(void);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */
