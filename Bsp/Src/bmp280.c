// Bsp/Src/bmp280.c
#include "bmp280.h"
#include "soft_i2c.h"
#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
static BMP280_Calib_t s_calib;           // 校准参数缓存，读取自 BMP280 校准寄存器
static volatile bool  s_is_init = false; // 驱动初始化状态标志
static uint8_t        s_ctrl_meas = 0;   // 缓存 CTRL_MEAS 配置，每次读取前触发 forced 测量

// BMP280 空值标志：测量未执行时寄存器固定为 0x800000（或 0x80 0x00 0x0x 形式）
static bool BMP280_IsEmptySample(int32_t adc, const uint8_t* raw3) {
    if (adc == 0x800000) {
        return true;
    }
    if (raw3[0] == 0x80U && raw3[1] == 0x00U && (raw3[2] & 0xF0U) == 0U) {
        return true;
    }
    return false;
}

// 读取寄存器（调用软件 I2C）
static bool BMP280_ReadReg(uint8_t reg_addr, uint8_t* data, uint16_t len) {
    return Sensors_I2C_ReadRegister(BMP280_I2C_ADDR, reg_addr, len, data);
}

// 写寄存器（调用软件 I2C）
static bool BMP280_WriteReg(uint8_t reg_addr, uint8_t data) {
    return Sensors_I2C_WriteRegister(BMP280_I2C_ADDR, reg_addr, 1, &data);
}

// 读取 24 字节校准系数到临时变量，校验通过后更新全局变量
static bool BMP280_ReadCalib(void) {
    uint8_t calib_data[24] = {0};
    BMP280_Calib_t new_calib;  // 临时变量

    if (!BMP280_ReadReg(BMP280_DIG_T1_LSB_REG, calib_data, 24)) {
        return false;
    }

    // 解析所有系数到临时变量
    new_calib.dig_T1 = (uint16_t)calib_data[0] | ((uint16_t)calib_data[1] << 8);
    new_calib.dig_T2 = (int16_t)calib_data[2] | ((int16_t)calib_data[3] << 8);
    new_calib.dig_T3 = (int16_t)calib_data[4] | ((int16_t)calib_data[5] << 8);
    new_calib.dig_P1 = (uint16_t)calib_data[6] | ((uint16_t)calib_data[7] << 8);
    new_calib.dig_P2 = (int16_t)calib_data[8] | ((int16_t)calib_data[9] << 8);
    new_calib.dig_P3 = (int16_t)calib_data[10] | ((int16_t)calib_data[11] << 8);
    new_calib.dig_P4 = (int16_t)calib_data[12] | ((int16_t)calib_data[13] << 8);
    new_calib.dig_P5 = (int16_t)calib_data[14] | ((int16_t)calib_data[15] << 8);
    new_calib.dig_P6 = (int16_t)calib_data[16] | ((int16_t)calib_data[17] << 8);
    new_calib.dig_P7 = (int16_t)calib_data[18] | ((int16_t)calib_data[19] << 8);
    new_calib.dig_P8 = (int16_t)calib_data[20] | ((int16_t)calib_data[21] << 8);
    new_calib.dig_P9 = (int16_t)calib_data[22] | ((int16_t)calib_data[23] << 8);
    
    // 重置 t_fine（临时变量中不需要保留旧值）
    new_calib.t_fine = 0;

    // 校验校准系数有效性（只检查非零，不检查范围）
    if (new_calib.dig_T1 == 0 || new_calib.dig_P1 == 0) {
        return false;  // 无效校准数据
    }

    // 校验通过，更新全局校准数据
    memcpy(&s_calib, &new_calib, sizeof(BMP280_Calib_t));
    
    return true;
}

// 温度补偿：输入 20 位原始温度，返回 0.01°C，同时计算 t_fine 供压力补偿使用
static int32_t BMP280_CompensateT(int32_t adc_T) {
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)s_calib.dig_T1 << 1))) * ((int32_t)s_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)s_calib.dig_T1)) * ((adc_T >> 4) - (int32_t)s_calib.dig_T1)) >> 12) * ((int32_t)s_calib.dig_T3) >> 14;

    s_calib.t_fine = var1 + var2;
    return (s_calib.t_fine * 5 + 128) >> 8;
}

// 压力补偿：输入 20 位原始压力，返回 Q24.8 格式（Pa * 256），溢出时返回 0
static uint32_t BMP280_CompensateP(int32_t adc_P) {
    int64_t var1, var2, p;
    int64_t temp;

    var1 = ((int64_t)s_calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)s_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)s_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)s_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)s_calib.dig_P3) >> 8) + ((var1 * (int64_t)s_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)s_calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0;
    }

    p = 1048576 - adc_P;
    temp = ((int64_t)p << 31) - var2;
    if (temp > INT64_MAX / 3125 || temp < INT64_MIN / 3125) {
        return 0;
    }
    p = (temp * 3125) / var1;
    var1 = (((int64_t)s_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)s_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_calib.dig_P7) << 4);

    return (uint32_t)p;
}

BMP280_Error_t BMP280_Init(void) {
    uint8_t chip_id = 0;
    uint8_t config = 0;
    bool calib_ok = false;

    s_is_init = false;

    // 1. 读取芯片 ID
    if (!BMP280_ReadReg(BMP280_CHIPID_REG, &chip_id, 1)) {
        return BMP280_ERR_I2C_COMM;
    }
    if (chip_id != 0x58) {
        return BMP280_ERR_CHIP_ID;
    }
    // 2. 读取校准系数（重试 3 次）
    for (uint8_t retry = 0; retry < 3; retry++) {
        if (BMP280_ReadCalib()) {
            calib_ok = true;
            break;
        }
        delay_ms(10);
    }
    if (!calib_ok) {
        return BMP280_ERR_CALIB_READ;
    }

    // 3. 软复位
    if (!BMP280_WriteReg(BMP280_RESET_REG, 0xB6)) {
        return BMP280_ERR_RESET_FAIL;
    }
    delay_ms(50);

    // 4. 配置 CONFIG 寄存器（滤波 + 待机）
    config = BMP280_STANDBY_62_5MS | BMP280_FILTER_COEFF_8;
    if (!BMP280_WriteReg(BMP280_CONFIG_REG, config)) {
        return BMP280_ERR_CONFIG_WRITE;
    }
    delay_ms(10);

    // 5. 配置 CTRL_MEAS 寄存器（过采样 + 模式）
    s_ctrl_meas = (BMP280_OVERSAMP_8X << 2) | (BMP280_OVERSAMP_16X << 5) | BMP280_NORMAL_MODE;
    if (!BMP280_WriteReg(BMP280_CTRLMEAS_REG, s_ctrl_meas)) {
        return BMP280_ERR_CONFIG_WRITE;
    }
    delay_ms(10);

    uint8_t check = 0;
    BMP280_ReadReg(BMP280_CTRLMEAS_REG, &check, 1);
    if (check != s_ctrl_meas) {
        return BMP280_ERR_CONFIG_WRITE;  // 配置写入失败
    }

    s_is_init = true;
    return BMP280_OK;
}

BMP280_Error_t BMP280_Read(float* pressure, float* temperature) {
    uint8_t  data[6] = {0};
    int32_t  adc_P = 0;
    int32_t  adc_T = 0;
    int32_t  temp_comp = 0;
    uint32_t press_comp = 0;
    uint8_t  status = 0;

    if (pressure == NULL || temperature == NULL) {
        return BMP280_ERR_DATA_READ;
    }

    if (!s_is_init) {
        return BMP280_ERR_DATA_READ;
    }

    // 每次读取前触发一次 forced 测量（BMP280 forced 模式单次测量后自动 sleep）
    if (!BMP280_WriteReg(BMP280_CTRLMEAS_REG, s_ctrl_meas)) {
        return BMP280_ERR_CONFIG_WRITE;
    }

    // 等待测量完成（检查 STATUS 寄存器的 measuring 位）
    for (uint8_t i = 0; i < 100; i++) {
        status = 0;
        if (!BMP280_ReadReg(BMP280_STATUS_REG, &status, 1)) {
            return BMP280_ERR_STATUS_READ;
        }
        if ((status & 0x08U) == 0) {
            break;
        }
        delay_ms(5);
    }

    // 突发读取 6 字节原始数据
    if (!BMP280_ReadReg(BMP280_PRESSURE_MSB_REG, data, 6)) {
        return BMP280_ERR_DATA_READ;
    }

    // 组装 20 位原始数据
    adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);

    // 原始数据合理性校验（防止 I2C 读取错误数据）
    if (BMP280_IsEmptySample(adc_P, data) || BMP280_IsEmptySample(adc_T, data + 3)) {
        return BMP280_ERR_DATA_INVALID;
    }
    if (adc_T < 400000 || adc_T > 600000) {
        return BMP280_ERR_DATA_INVALID;
    }
    if (adc_P < 300000 || adc_P > 1100000) {
        return BMP280_ERR_DATA_INVALID;
    }
    // 温度补偿
    temp_comp = BMP280_CompensateT(adc_T);
    *temperature = (float)temp_comp / 100.0f;

    // 压力补偿
    press_comp = BMP280_CompensateP(adc_P);
    if (press_comp == 0) {
        return BMP280_ERR_COMP_OVERFLOW;
    }
    *pressure = (float)press_comp / 25600.0f;

    return BMP280_OK;
}

const BMP280_Calib_t* BMP280_GetCalib(void) {
    return &s_calib;
}
