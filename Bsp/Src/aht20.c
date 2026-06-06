// Bsp/Src/aht20.c
#include "aht20.h"
#include "soft_i2c.h"
#include "stm32f1xx_hal.h"
#include "uart.h"

// 发送命令（无寄存器地址，直接写命令字节）， 用于 AHT20 的命令型交互（如触发测量、软复位）
static bool AHT20_SendCmd(uint8_t cmd, const uint8_t* param, uint8_t param_len) {
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

// 读取测量结果（先写 0x71 命令，再读 6 字节），调用时需保证 data 有至少 6 字节缓冲
static bool AHT20_ReadMeasurement(uint8_t* data) {
    if (data == NULL) {
        return false;
    }
    return Sensors_I2C_ReadCommandData(AHT20_I2C_ADDR, AHT20_CMD_STATUS, data, 6);
}

// 读取单字节状态，通过 Sensors_I2C_ReadCommandData 读取状态寄存器
static bool AHT20_ReadStatus(uint8_t* status) {
    if (status == NULL) {
        return false;
    }
    return Sensors_I2C_ReadCommandData(AHT20_I2C_ADDR, AHT20_CMD_STATUS, status, 1);
}

// 解析原始测量数据并做合理性校验，按数据手册拆分 20 位湿度和 20 位温度原始值，转换为工程量并校验范围
static bool AHT20_ParseData(const uint8_t* raw, float* temp, float* humi) {
    uint32_t hum_raw, temp_raw;

    hum_raw = ((uint32_t)raw[1] << 12) | ((uint32_t)raw[2] << 4) | ((uint32_t)raw[3] >> 4);

    temp_raw = ((uint32_t)(raw[3] & 0x0FU) << 16) | ((uint32_t)raw[4] << 8) | (uint32_t)raw[5];

    *humi = (float)hum_raw * AHT20_HUMIDITY_FACTOR / AHT20_DATA_SCALE;
    *temp = (float)temp_raw * AHT20_TEMP_FACTOR / AHT20_DATA_SCALE - AHT20_TEMP_OFFSET;

    if (*temp < AHT20_TEMP_MIN || *temp > AHT20_TEMP_MAX)
        return false;
    if (*humi < AHT20_HUMI_MIN || *humi > AHT20_HUMI_MAX)
        return false;

    return true;
}

// 判断校准位（Bit3）是否已完成
static bool AHT20_IsCalibrated(uint8_t status) {
    return ((status & 0x08U) != 0U);
}

// 判断传感器是否空闲（measuring 位为 0）
static bool AHT20_IsIdle(uint8_t status) {
    return ((status & 0x80U) == 0U);
}

AHT20_Error_t AHT20_Init(void) {
    uint8_t status = 0;

    HAL_Delay(AHT20_POWER_UP_DELAY_MS);

    if (!AHT20_ReadStatus(&status)) {
        return AHT20_ERR_STATUS_READ;
    }

    for (uint8_t i = 0; i < 10; i++) {
        if (AHT20_IsIdle(status))
            break;
        HAL_Delay(10);
        if (!AHT20_ReadStatus(&status)) {
            return AHT20_ERR_STATUS_READ;
        }
    }

    if (!AHT20_IsCalibrated(status)) {
        if (!AHT20_SendCmd(AHT20_CMD_SOFT_RESET, NULL, 0)) {
            return AHT20_ERR_SEND_CMD;
        }
        HAL_Delay(AHT20_SOFT_RESET_DELAY_MS);
        if (!AHT20_ReadStatus(&status)) {
            return AHT20_ERR_STATUS_READ;
        }
        if (!AHT20_IsCalibrated(status)) {
            return AHT20_ERR_CALIB_MISSING;
        }
    }

    return AHT20_OK;
}

AHT20_Error_t AHT20_Read(float* temp, float* humi) {
    uint8_t raw_data[6] = {0};
    uint8_t measure_param[2] = {AHT20_MEASURE_DATA0, AHT20_MEASURE_DATA1};
    uint8_t status = 0;

    if (temp == NULL || humi == NULL)
        return AHT20_ERR_DATA_INVALID;

    if (!AHT20_SendCmd(AHT20_CMD_MEASURE, measure_param, 2)) {
        return AHT20_ERR_SEND_CMD;
    }

    HAL_Delay(AHT20_MEASURE_DELAY_MS);

    for (uint8_t i = 0; i < 10; i++) {
        if (!AHT20_ReadStatus(&status)) {
            return AHT20_ERR_STATUS_READ;
        }
        if (AHT20_IsIdle(status))
            break;
        HAL_Delay(10);
    }

    if (!AHT20_IsIdle(status)) {
        return AHT20_ERR_BUSY_TIMEOUT;
    }

    if (!AHT20_ReadMeasurement(raw_data)) {
        return AHT20_ERR_I2C_COMM;
    }

    if (!AHT20_ParseData(raw_data, temp, humi)) {
        return AHT20_ERR_DATA_INVALID;
    }

    return AHT20_OK;
}
