// App/Driver/Inc/driver_env_sensor.h
#ifndef __DRIVER_ENV_SENSOR_H__
#define __DRIVER_ENV_SENSOR_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ENV_SENSOR_DRIVER_UART2_LOG_ENABLE
#define ENV_SENSOR_DRIVER_UART2_LOG_ENABLE 1  // UART2 调试日志开关
#endif
//系统状态寄存器故障标志位
#define ENV_SENSOR_STATUS_AHT20_OK  (1U << 4)  // Bit4: AHT20 正常
#define ENV_SENSOR_STATUS_BMP280_OK (1U << 5)  // Bit5: BMP280 正常

/**
 * @brief  初始化环境传感器驱动层
 * @return true=全部初始化成功，false=至少一个传感器初始化失败
 */
bool EnvSensor_Driver_Init(void);

/**
 * @brief  读取环境传感器数据
 * @param  temp 输出温度（单位：°C）
 * @param  humi 输出湿度（单位：%RH）
 * @param  pressure 输出气压（单位：hPa）
 * @return true=两个传感器都读取成功，false=任一传感器读取失败
 * @note   当某个传感器读取失败时，函数仍会保留并返回上次有效值
 */
bool EnvSensor_Driver_ReadData(float* temp, float* humi, float* pressure);

/**
 * @brief  获取传感器简化状态位
 * @return Bit4=1: AHT20正常; Bit5=1: BMP280正常
 */
uint8_t EnvSensor_Driver_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_ENV_SENSOR_H__ */
