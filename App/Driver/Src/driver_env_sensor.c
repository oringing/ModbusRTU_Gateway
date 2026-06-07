// App/Driver/Src/driver_env_sensor.c
#include "driver_env_sensor.h"
#include "aht20.h"
#include "bmp280.h"
#include "driver_uart.h"
#include "soft_i2c.h"
#include "stm32f1xx_hal.h"

// ---- 可调参数（性能调优）----
#define SENSOR_RETRY_MAX_COUNT 2U     // 最大重试次数
#define SENSOR_RETRY_DELAY_MS 10U     // 重试间隔(ms)
#define SENSOR_LOG_THROTTLE_MS 10000U // 限流告警间隔(10秒)
#define SENSOR_FAILURE_THRESHOLD 5U   // 连续失败阈值（达到后标记故障）

// ---- 静态变量 ----
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE  // UART2 调试日志开关
static uint32_t s_last_aht20_log_tick = 0;   // AHT20上次错误日志时间戳，用于限流
static uint32_t s_last_bmp280_log_tick = 0;  // BMP280上次错误日志时间戳，用于限流
#endif
static uint8_t  s_sensor_status = 0U;        // 传感器状态位（Bit4=AHT20, Bit5=BMP280）
static bool     s_is_initialized = false;    // 驱动层初始化标志
static bool     s_last_values_valid = false; // 上次有效值是否可用
static float    s_last_temperature = 0.0f;   // 上次有效温度（℃）
static float    s_last_humidity = 0.0f;      // 上次有效湿度（%）
static float    s_last_pressure = 0.0f;      // 上次有效气压（hPa）
static uint8_t  s_aht20_fail_count = 0U;     // AHT20连续失败计数
static uint8_t  s_bmp280_fail_count = 0U;    // BMP280连续失败计数

// 更新传感器状态位
static void EnvSensor_Driver_UpdateStatus(uint8_t mask, bool ok) {
    if (ok) {
        s_sensor_status |= mask;
    } else {
        s_sensor_status &= (uint8_t)~mask;
    }
}

#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
// 将AHT20错误码转为可读字符串（仅调试用）
static const char* EnvSensor_Driver_AHT20ErrorToString(AHT20_Error_t err) {
    switch (err) {
    case AHT20_OK:
        return "AHT20 OK\r\n";
    case AHT20_ERR_I2C_COMM:
        return "AHT20 error: I2C communication\r\n";
    case AHT20_ERR_STATUS_READ:
        return "AHT20 error: status read\r\n";
    case AHT20_ERR_BUSY_TIMEOUT:
        return "AHT20 error: busy timeout\r\n";
    case AHT20_ERR_CALIB_MISSING:
        return "AHT20 error: calibration missing\r\n";
    case AHT20_ERR_SEND_CMD:
        return "AHT20 error: send command\r\n";
    case AHT20_ERR_DATA_INVALID:
        return "AHT20 error: data invalid\r\n";
    default:
        return "AHT20 error: unknown\r\n";
    }
}

// 将BMP280错误码转为可读字符串（仅调试用）
static const char* EnvSensor_Driver_BMP280ErrorToString(BMP280_Error_t err) {
    switch (err) {
    case BMP280_OK:
        return "BMP280 OK\r\n";
    case BMP280_ERR_I2C_COMM:
        return "BMP280 error: I2C communication\r\n";
    case BMP280_ERR_CHIP_ID:
        return "BMP280 error: invalid chip id\r\n";
    case BMP280_ERR_CALIB_READ:
        return "BMP280 error: calibration read\r\n";
    case BMP280_ERR_RESET_FAIL:
        return "BMP280 error: reset fail\r\n";
    case BMP280_ERR_CONFIG_WRITE:
        return "BMP280 error: config write\r\n";
    case BMP280_ERR_STATUS_READ:
        return "BMP280 error: status read\r\n";
    case BMP280_ERR_DATA_READ:
        return "BMP280 error: data read\r\n";
    case BMP280_ERR_COMP_OVERFLOW:
        return "BMP280 error: compensation overflow\r\n";
    case BMP280_ERR_DATA_INVALID:
        return "BMP280 error: data invalid\r\n";
    default:
        return "BMP280 error: unknown\r\n";
    }
}
#endif

bool EnvSensor_Driver_Init(void) {
    bool          ok = true;
    AHT20_Error_t aht_status = AHT20_Init();
    BMP280_Error_t bmp_status = BMP280_Init();

    if (aht_status == AHT20_OK) {
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_AHT20_OK, true);
        s_aht20_fail_count = 0U;
    } else {
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_AHT20_OK, false);
        ok = false;
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
        UART2_Driver_DebugPrint(EnvSensor_Driver_AHT20ErrorToString(aht_status));
#endif
    }

    if (bmp_status == BMP280_OK) {
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_BMP280_OK, true);
        s_bmp280_fail_count = 0U;
    } else {
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_BMP280_OK, false);
        ok = false;
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
        UART2_Driver_DebugPrint(EnvSensor_Driver_BMP280ErrorToString(bmp_status));
#endif
    }

    s_is_initialized = true;
    return ok;
}

bool EnvSensor_Driver_ReadData(float* temp, float* humi, float* pressure) {
    // 1. 参数校验
    if (temp == NULL || humi == NULL || pressure == NULL) {
        return false;
    }

    if (!s_is_initialized) {
        return false;
    }

    bool     aht_ok = false;
    bool     bmp_ok = false;
    float    aht_temp = 0.0f;
    float    aht_humi = 0.0f;
    float    bmp_press = 0.0f;
    float    bmp_temp = 0.0f;
    uint32_t now = HAL_GetTick();

    // 2. 读取 AHT20（带重试）
    if (s_aht20_fail_count >= SENSOR_FAILURE_THRESHOLD) {
        I2C_Bus_Recover();      // 先恢复总线
        AHT20_Error_t aht_status = AHT20_Init();
        if (aht_status == AHT20_OK) {            s_aht20_fail_count = 0U;
        }
    }
    AHT20_Error_t aht_result = AHT20_ERR_I2C_COMM;
    for (uint8_t retry = 0; retry < SENSOR_RETRY_MAX_COUNT; retry++) {
        aht_result = AHT20_Read(&aht_temp, &aht_humi);
        if (aht_result == AHT20_OK) {
            break;
        }
        if (retry < SENSOR_RETRY_MAX_COUNT - 1) {
            delay_ms(SENSOR_RETRY_DELAY_MS);
        }
    }

    if (aht_result == AHT20_OK) {
        aht_ok = true;
        s_aht20_fail_count = 0U;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_AHT20_OK, true);
        s_last_temperature = aht_temp;
        s_last_humidity = aht_humi;
    } else {
        s_aht20_fail_count++;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_AHT20_OK, false);
        if (s_aht20_fail_count > SENSOR_FAILURE_THRESHOLD) {
            s_aht20_fail_count = SENSOR_FAILURE_THRESHOLD;
        }
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
        // 限流打印：10秒内最多打印一次
        if ((now - s_last_aht20_log_tick) >= SENSOR_LOG_THROTTLE_MS) {
            s_last_aht20_log_tick = now;
            UART2_Driver_DebugPrint(EnvSensor_Driver_AHT20ErrorToString(aht_result));
        }
#endif
    }

    // 3. 读取 BMP280（带重试）
    if (s_bmp280_fail_count >= SENSOR_FAILURE_THRESHOLD) {
        I2C_Bus_Recover();      // 先恢复总线
        BMP280_Error_t bmp_status = BMP280_Init();  // 尝试重新初始化
        if (bmp_status == BMP280_OK) {            
            s_bmp280_fail_count = 0U;
        }
    }
    BMP280_Error_t bmp_result = BMP280_ERR_I2C_COMM;
    for (uint8_t retry = 0; retry < SENSOR_RETRY_MAX_COUNT; retry++) {
        bmp_result = BMP280_Read(&bmp_press, &bmp_temp);
        if (bmp_result == BMP280_OK) {
            break;
        }
        if (retry < SENSOR_RETRY_MAX_COUNT - 1) {
            delay_ms(SENSOR_RETRY_DELAY_MS);
        }
    }

    if (bmp_result == BMP280_OK) {
        bmp_ok = true;
        s_bmp280_fail_count = 0U;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_BMP280_OK, true);
        s_last_pressure = bmp_press;
    } else {
        s_bmp280_fail_count++;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_BMP280_OK, false);
        if (s_bmp280_fail_count > SENSOR_FAILURE_THRESHOLD) {
            s_bmp280_fail_count = SENSOR_FAILURE_THRESHOLD;
        }
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
        // 限流打印：10秒内最多打印一次
        if ((now - s_last_bmp280_log_tick) >= SENSOR_LOG_THROTTLE_MS) {
            s_last_bmp280_log_tick = now;
            UART2_Driver_DebugPrint(EnvSensor_Driver_BMP280ErrorToString(bmp_result));
        }
#endif
    }

    if (aht_ok && bmp_ok) {
        float temp_diff = aht_temp - bmp_temp;
        if (temp_diff > 5.0f || temp_diff < -5.0f) {
            // 两个传感器温差过大，BMP280 数据可能有问题
            bmp_ok = false;  // 丢弃本次 BMP280 数据
            s_bmp280_fail_count++;
            if (s_bmp280_fail_count < SENSOR_FAILURE_THRESHOLD) {
                s_bmp280_fail_count = SENSOR_FAILURE_THRESHOLD;
            }
        }
    }

    // 4. 标记上次有效值是否可用
    if (aht_ok || bmp_ok) {
        s_last_values_valid = true;
    }

    // 5. 输出温度/湿度（优先用AHT20新值，失败则用上次有效值）
    if (aht_ok) {
        *temp = aht_temp;
        *humi = aht_humi;
    } else if (s_last_values_valid) {
        *temp = s_last_temperature;
        *humi = s_last_humidity;
    } else {
        *temp = 0.0f;
        *humi = 0.0f;
    }

    // 6. 输出气压（优先用BMP280新值，失败则用上次有效值）
    if (bmp_ok) {
        *pressure = bmp_press;
    } else if (s_last_values_valid) {
        *pressure = s_last_pressure;
    } else {
        *pressure = 0.0f;
    }

    return (aht_ok && bmp_ok);
}

uint8_t EnvSensor_Driver_GetStatus(void) {
    return s_sensor_status;
}
