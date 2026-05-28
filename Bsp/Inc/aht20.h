//Bsp/Inc/aht20.h
#ifndef __AHT20_H__
#define __AHT20_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- 协议常量（不可修改）----
#define AHT20_I2C_ADDR              0x38U   // 固定地址，不可修改

// ---- 命令定义（数据手册第 11-12 页）----
#define AHT20_CMD_MEASURE           0xACU   // 触发测量命令
#define AHT20_CMD_SOFT_RESET        0xBAU   // 软复位命令（仅异常时使用）
#define AHT20_CMD_STATUS            0x71U   // 状态查询命令（备用）

// ---- 配置宏（可修改）----
#define AHT20_MEASURE_DATA0         0x33U   // 第 1 参数（使能传感器）
#define AHT20_MEASURE_DATA1         0x00U   // 第 2 参数（保留）

// ---- 可调参数（性能调优）----
#define AHT20_POWER_UP_DELAY_MS     5U      // 上电后等待传感器稳定（最小 5ms）
#define AHT20_MEASURE_DELAY_MS      80U     // 触发测量后等待（典型 75ms，最大 80ms）
#define AHT20_SOFT_RESET_DELAY_MS   20U     // 软复位后等待

// ---- 数据转换常数 ----
#define AHT20_DATA_SCALE            1048576.0f  // 2^20，20 位数据满量程
#define AHT20_HUMIDITY_FACTOR       100.0f      // 湿度转换因子（0-100%RH）
#define AHT20_TEMP_FACTOR           200.0f      // 温度转换因子（-40-85°C 范围 200 度）
#define AHT20_TEMP_OFFSET           50.0f       // 温度偏移量（从 0°C 移到的 -50°C）

// ---- 合理性校验范围 ----
#define AHT20_TEMP_MIN              -40.0f      // 工作温度下限
#define AHT20_TEMP_MAX              85.0f       // 工作温度上限
#define AHT20_HUMI_MIN              0.0f        // 湿度下限
#define AHT20_HUMI_MAX              100.0f      // 湿度上限

/**
 * @brief   初始化 AHT20 传感器
 * @return  true  = 初始化成功（通信正常且校准使能）
 * @return  false = 初始化失败（通信异常或校准无效）
 * @note    按数据手册流程：上电延时 5ms → 检查状态位 Bit 3（校准使能）
 *          → 若校准未使能则执行软复位
 * @warning BSP 层调用（RTOS 启动前），使用 HAL_Delay() 阻塞延时
 */
bool AHT20_Init(void);

/**
 * @brief   读取温湿度数据
 * @param   temp 温度输出指针（单位：°C，范围 -40~85）
 * @param   humi 湿度输出指针（单位：%RH，范围 0~100）
 * @return  true  = 读取成功（数据已校验）
 * @return  false = 读取失败（通信超时或数据异常）
 * @note    流程：发送测量命令（0xAC 0x33 0x00）→ 延时 80ms → 读取 6 字节
 *          → 解析 → 合理性校验
 */
bool AHT20_Read(float *temp, float *humi);

#ifdef __cplusplus
}
#endif

#endif /* __AHT20_H__ */
