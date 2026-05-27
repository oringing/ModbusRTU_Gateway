//Bsp/Src/aht20.c
#include "aht20.h"
#include "i2c.h"
#include "stm32f1xx_hal.h"

// 将命令和参数合并为单次 I2C 传输（命令式设备专用）
static bool AHT20_SendCmd(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t buf[3];
    uint8_t total = 1 + len;

    if (len > 2) {
        return false;   // AHT20 命令最大 3 字节（1 命令 + 2 参数）
    }

    buf[0] = cmd;
    for (uint8_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }

    return BSP_I2C_Master_Transmit(AHT20_I2C_ADDR, buf, total);
}

// 读取 6 字节传感器数据（状态 + 湿度 3 字节 + 温度 3 字节）
static bool AHT20_ReadRaw(uint8_t *data)
{
    if (data == NULL) {
        return false;
    }
    return BSP_I2C_Master_Receive(AHT20_I2C_ADDR, data, 6);
}

// 解析原始数据并校验合理性
static bool AHT20_ParseData(const uint8_t *raw, float *temp, float *humi)
{
    uint32_t hum_raw, temp_raw;

    // 湿度原始值：20 位（数据手册第 12 页）
    hum_raw = ((uint32_t)raw[1] << 12) |
              ((uint32_t)raw[2] << 4) |
              ((uint32_t)raw[3] >> 4);

    // 温度原始值：20 位（数据手册第 12 页）
    temp_raw = ((uint32_t)(raw[3] & 0x0FU) << 16) |
               ((uint32_t)raw[4] << 8) |
               (uint32_t)raw[5];

    // 工程值转换（数据手册第 12 页公式）
    *humi = (float)hum_raw * AHT20_HUMIDITY_FACTOR / AHT20_DATA_SCALE;
    *temp = (float)temp_raw * AHT20_TEMP_FACTOR / AHT20_DATA_SCALE - AHT20_TEMP_OFFSET;

    // 合理性校验（防止 I2C 错误导致异常值）
    if (*temp < AHT20_TEMP_MIN || *temp > AHT20_TEMP_MAX) {
        return false;
    }
    if (*humi < AHT20_HUMI_MIN || *humi > AHT20_HUMI_MAX) {
        return false;
    }

    return true;
}

// 检查校准使能位（Bit 3 = 1 表示 OTP 数据有效）
static bool AHT20_IsCalibrated(uint8_t status)
{
    return ((status & 0x08U) != 0U);
}

// 检查忙标志（Bit 7 = 0 表示空闲）
static bool AHT20_IsIdle(uint8_t status)
{
    return ((status & 0x80U) == 0U);
}

/**
 * @brief   初始化 AHT20（按数据手册第 11 页流程）
 * @note    正确流程：上电 → 等待 5ms → 读状态检查校准位
 *          ❌ 错误流程：发送 0xE1 初始化命令（数据手册无此命令）
 */
bool AHT20_Init(void)
{
    uint8_t raw_data[6] = {0};

    // 数据手册第 11 页：上电后至少等待 5ms
    HAL_Delay(AHT20_POWER_UP_DELAY_MS);

    // 读取状态数据验证通信
    if (!AHT20_ReadRaw(raw_data)) {
        return false;
    }

    // 等待传感器退出忙状态（最多 100ms）
    for (uint8_t i = 0; i < 10; i++) {
        if (AHT20_IsIdle(raw_data[0])) {
            break;
        }
        HAL_Delay(10);
        if (!AHT20_ReadRaw(raw_data)) {
            return false;
        }
    }

    // 检查校准使能位，若未使能则执行软复位
    if (!AHT20_IsCalibrated(raw_data[0])) {
        if (!AHT20_SendCmd(AHT20_CMD_SOFT_RESET, NULL, 0)) {
            return false;
        }
        HAL_Delay(AHT20_SOFT_RESET_DELAY_MS);
    }

    return true;
}

/**
 * @brief   读取温湿度数据（数据手册第 11-12 页流程）
 * @note    流程：发送 0xAC 0x33 0x00 → 等待 80ms → 读取 6 字节
 */
bool AHT20_Read(float *temp, float *humi)
{
    uint8_t raw_data[6] = {0};
    uint8_t measure_param[2] = {AHT20_MEASURE_DATA0, AHT20_MEASURE_DATA1};

    if (temp == NULL || humi == NULL) {
        return false;
    }

    // 触发测量（命令 0xAC + 参数 0x33 0x00）
    if (!AHT20_SendCmd(AHT20_CMD_MEASURE, measure_param, 2)) {
        return false;
    }

    // 等待测量完成（数据手册要求 80ms）
    HAL_Delay(AHT20_MEASURE_DELAY_MS);

    // 读取 6 字节数据
    if (!AHT20_ReadRaw(raw_data)) {
        return false;
    }

    // 检查忙标志（应为 0）
    if (!AHT20_IsIdle(raw_data[0])) {
        return false;
    }

    // 解析并校验数据
    if (!AHT20_ParseData(raw_data, temp, humi)) {
        // 数据异常时返回默认值（避免上层读到无效数据）
        *temp = 25.0f;
        *humi = 50.0f;
        return false;
    }

    return true;
}

