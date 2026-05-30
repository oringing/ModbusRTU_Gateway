// Bsp/Src/aht20.c
#include "aht20.h"
#include "soft_i2c.h"
#include "stm32f1xx_hal.h"
#include "uart.h"

#include <stdio.h>

// 打包命令并发送，AHT20 采用命令式交互，无寄存器地址
static bool AHT20_SendCmd(uint8_t cmd, const uint8_t* param, uint8_t param_len)
{
    uint8_t buf[3];
    uint8_t total = 1 + param_len;

    if (param_len > 2) {
        return false;
    }

    buf[0] = cmd;
    for (uint8_t i = 0; i < param_len; i++) {
        buf[1 + i] = param[i];
    }

    return Sensors_I2C_WriteCommand(AHT20_I2C_ADDR, buf, total);
}

// 发送 0x71 命令后读取 6 字节测量结果
static bool AHT20_ReadMeasurement(uint8_t* data)
{
    if (data == NULL) {
        return false;
    }
    return Sensors_I2C_ReadCommandData(AHT20_I2C_ADDR, AHT20_CMD_STATUS, data, 6);
}

// 读取单字节状态寄存器
static bool AHT20_ReadStatus(uint8_t* status)
{
    if (status == NULL) {
        return false;
    }
    return Sensors_I2C_ReadCommandData(AHT20_I2C_ADDR, AHT20_CMD_STATUS, status, 1);
}

// 将 20 位原始数据转换为温湿度物理量，并校验是否在合理范围内
static bool AHT20_ParseData(const uint8_t* raw, float* temp, float* humi)
{
    uint32_t hum_raw, temp_raw;

    // 湿度：raw[1]高8位，raw[2]中8位，raw[3]高4位
    hum_raw = ((uint32_t)raw[1] << 12) | ((uint32_t)raw[2] << 4) | ((uint32_t)raw[3] >> 4);

    // 温度：raw[3]低4位，raw[4]高8位，raw[5]低8位
    temp_raw = ((uint32_t)(raw[3] & 0x0FU) << 16) | ((uint32_t)raw[4] << 8) | (uint32_t)raw[5];

    *humi = (float)hum_raw * AHT20_HUMIDITY_FACTOR / AHT20_DATA_SCALE;
    *temp = (float)temp_raw * AHT20_TEMP_FACTOR / AHT20_DATA_SCALE - AHT20_TEMP_OFFSET;

    if (*temp < AHT20_TEMP_MIN || *temp > AHT20_TEMP_MAX) {
        return false;
    }
    if (*humi < AHT20_HUMI_MIN || *humi > AHT20_HUMI_MAX) {
        return false;
    }

    return true;
}

// 检查状态寄存器的 Bit3（校准使能位）
static bool AHT20_IsCalibrated(uint8_t status)
{
    return ((status & 0x08U) != 0U);
}

// 检查状态寄存器的 Bit7（忙标志位），0 表示空闲
static bool AHT20_IsIdle(uint8_t status)
{
    return ((status & 0x80U) == 0U);
}

AHT20_Error_t AHT20_Init(void)
{
    uint8_t status = 0;

    // 1. 上电后等待传感器稳定
    delay_ms(AHT20_POWER_UP_DELAY_MS);

    // 2. 读取状态寄存器
    if (!AHT20_ReadStatus(&status)) {
        return AHT20_ERR_STATUS_READ;
    }

    // 3. 轮询等待传感器退出忙状态
    for (uint8_t i = 0; i < 10; i++) {
        if (AHT20_IsIdle(status)) {
            break;
        }
        delay_ms(10);
        if (!AHT20_ReadStatus(&status)) {
            return AHT20_ERR_STATUS_READ;
        }
    }

    // 4. 检查校准位，未校准则执行软复位
    if (!AHT20_IsCalibrated(status)) {
        if (!AHT20_SendCmd(AHT20_CMD_SOFT_RESET, NULL, 0)) {
            return AHT20_ERR_SEND_CMD;
        }
        delay_ms(AHT20_SOFT_RESET_DELAY_MS);

        if (!AHT20_ReadStatus(&status)) {
            return AHT20_ERR_STATUS_READ;
        }
        if (!AHT20_IsCalibrated(status)) {
            return AHT20_ERR_CALIB_MISSING;
        }
    }

    return AHT20_OK;
}

AHT20_Error_t AHT20_Read(float* temp, float* humi)
{
    uint8_t raw_data[6] = {0};
    uint8_t measure_param[2] = {AHT20_MEASURE_DATA0, AHT20_MEASURE_DATA1};
    uint8_t status = 0;

    if (temp == NULL || humi == NULL) {
        return AHT20_ERR_DATA_INVALID;
    }

    // 1. 触发测量
    if (!AHT20_SendCmd(AHT20_CMD_MEASURE, measure_param, 2)) {
        return AHT20_ERR_SEND_CMD;
    }

    // 2. 等待测量完成
    delay_ms(AHT20_MEASURE_DELAY_MS);

    // 3. 轮询等待传感器空闲
    for (uint8_t i = 0; i < 10; i++) {
        if (!AHT20_ReadStatus(&status)) {
            return AHT20_ERR_STATUS_READ;
        }
        if (AHT20_IsIdle(status)) {
            break;
        }
        delay_ms(10);
    }

    if (!AHT20_IsIdle(status)) {
        return AHT20_ERR_BUSY_TIMEOUT;
    }

    // 4. 读取 6 字节原始数据
    if (!AHT20_ReadMeasurement(raw_data)) {
        return AHT20_ERR_I2C_COMM;
    }

    // 5. 解析并校验数据
    if (!AHT20_ParseData(raw_data, temp, humi)) {
        return AHT20_ERR_DATA_INVALID;
    }

    return AHT20_OK;
}
