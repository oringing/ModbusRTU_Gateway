// App/Driver/Src/driver_env_sensor.c
#include "driver_env_sensor.h"
#include "aht20.h"
#include "bmp280.h"
#include "driver_uart.h"

static uint8_t s_sensor_status = 0U;
static bool s_is_initialized = false;
static bool s_last_values_valid = false;
static float s_last_temperature = 0.0f;
static float s_last_humidity = 0.0f;
static float s_last_pressure = 0.0f;
static uint8_t s_aht20_fail_count = 0U;
static uint8_t s_bmp280_fail_count = 0U;
static const uint8_t s_failure_threshold = 5U;

static void EnvSensor_Driver_UpdateStatus(uint8_t mask, bool ok) {
    if (ok) {
        s_sensor_status |= mask;
    } else {
        s_sensor_status &= (uint8_t)~mask;
    }
}

#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
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
        default:
            return "BMP280 error: unknown\r\n";
    }
}
#endif

bool EnvSensor_Driver_Init(void) {
    UART2_Driver_DebugPrint("===== EnvSensor Driver Init Start =====\r\n");
    UART2_Driver_DebugPrint("EnvSensor_Driver_Init: calling AHT20_Init()\r\n");
    bool ok = true;
    AHT20_Error_t aht_status = AHT20_Init();
    UART2_Driver_DebugPrint("EnvSensor_Driver_Init: AHT20_Init returned\r\n");

    UART2_Driver_DebugPrint("EnvSensor_Driver_Init: calling BMP280_Init()\r\n");
    BMP280_Error_t bmp_status = BMP280_Init();
    UART2_Driver_DebugPrint("EnvSensor_Driver_Init: BMP280_Init returned\r\n");

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
    if (temp == NULL || humi == NULL || pressure == NULL) {
        return false;
    }

    if (!s_is_initialized) {
        return false;
    }

    bool aht_ok = false;
    bool bmp_ok = false;
    float aht_temp = 0.0f;       // AHT20 温度（用于输出）
    float aht_humi = 0.0f;       // AHT20 湿度
    float bmp_press = 0.0f;      // BMP280 气压
    float bmp_temp = 0.0f;       // BMP280 温度（仅用于传感器内部校准，不输出）

    AHT20_Error_t aht_result = AHT20_Read(&aht_temp, &aht_humi);
    if (aht_result == AHT20_OK) {
        aht_ok = true;
        s_aht20_fail_count = 0U;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_AHT20_OK, true);
        s_last_temperature = aht_temp;
        s_last_humidity = aht_humi;
    } else {
        s_aht20_fail_count++;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_AHT20_OK, false);
        if (s_aht20_fail_count > s_failure_threshold) {
            s_aht20_fail_count = s_failure_threshold;
        }
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
        UART2_Driver_DebugPrint(EnvSensor_Driver_AHT20ErrorToString(aht_result));
#endif
    }

    // BMP280 读取：气压存入 bmp_press，温度存入 bmp_temp（仅供 BMP280 内部校准）
    BMP280_Error_t bmp_result = BMP280_Read(&bmp_press, &bmp_temp);
    if (bmp_result == BMP280_OK) {
        bmp_ok = true;
        s_bmp280_fail_count = 0U;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_BMP280_OK, true);
        s_last_pressure = bmp_press;
    } else {
        s_bmp280_fail_count++;
        EnvSensor_Driver_UpdateStatus(ENV_SENSOR_STATUS_BMP280_OK, false);
        if (s_bmp280_fail_count > s_failure_threshold) {
            s_bmp280_fail_count = s_failure_threshold;
        }
#if ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
        UART2_Driver_DebugPrint(EnvSensor_Driver_BMP280ErrorToString(bmp_result));
#endif
    }

    if (aht_ok || bmp_ok) {
        s_last_values_valid = true;
    }

    // 输出：温度/湿度使用 AHT20 的值
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

    // 输出：气压使用 BMP280 的值
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
