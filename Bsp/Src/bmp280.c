//Bsp/Src/bmp280.c
#include "bmp280.h"
#include "i2c.h"
#include "stm32f1xx_hal.h"
#include <math.h>

static BMP280_Calib_t s_calib;          // 校准系数，BMP280_Init中读取
static bool s_is_init = false;          // 初始化成功标志

// 等待测量完成，检查STATUS寄存器的measuring位
static bool BMP280_WaitForMeasurement(void)
{
    uint8_t status = 0;

    for (uint32_t i = 0; i < 100; i++) {
        if (!BSP_I2C_Mem_Read(BMP280_I2C_ADDR, BMP280_REG_STATUS,
                              I2C_MEMADD_SIZE_8BIT, &status, 1)) {
            return false;
        }
        // measuring位(Bit3)=0表示测量完成
        if ((status & 0x08U) == 0) {
            return true;
        }
        HAL_Delay(1);
    }
    return false;
}

// 读取24字节校准系数
static bool BMP280_ReadCalib(void)
{
    uint8_t calib_data[24] = {0};

    if (!BSP_I2C_Mem_Read(BMP280_I2C_ADDR, BMP280_REG_CALIB_START,
                          I2C_MEMADD_SIZE_8BIT, calib_data, 24)) {
        return false;
    }

    // 小端格式解析
    s_calib.dig_T1 = (uint16_t)calib_data[0] | ((uint16_t)calib_data[1] << 8);
    s_calib.dig_T2 = (int16_t)calib_data[2] | ((int16_t)calib_data[3] << 8);
    s_calib.dig_T3 = (int16_t)calib_data[4] | ((int16_t)calib_data[5] << 8);
    s_calib.dig_P1 = (uint16_t)calib_data[6] | ((uint16_t)calib_data[7] << 8);
    s_calib.dig_P2 = (int16_t)calib_data[8] | ((int16_t)calib_data[9] << 8);
    s_calib.dig_P3 = (int16_t)calib_data[10] | ((int16_t)calib_data[11] << 8);
    s_calib.dig_P4 = (int16_t)calib_data[12] | ((int16_t)calib_data[13] << 8);
    s_calib.dig_P5 = (int16_t)calib_data[14] | ((int16_t)calib_data[15] << 8);
    s_calib.dig_P6 = (int16_t)calib_data[16] | ((int16_t)calib_data[17] << 8);
    s_calib.dig_P7 = (int16_t)calib_data[18] | ((int16_t)calib_data[19] << 8);
    s_calib.dig_P8 = (int16_t)calib_data[20] | ((int16_t)calib_data[21] << 8);
    s_calib.dig_P9 = (int16_t)calib_data[22] | ((int16_t)calib_data[23] << 8);

    return true;
}

// 温度补偿：输入20位原始值，输出0.01°C，t_fine传递给压力补偿
static int32_t BMP280_CompensateT(int32_t adc_T)
{
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)s_calib.dig_T1 << 1))) *
            ((int32_t)s_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)s_calib.dig_T1)) *
             ((adc_T >> 4) - (int32_t)s_calib.dig_T1)) >> 12) *
            ((int32_t)s_calib.dig_T3) >> 14;

    s_calib.t_fine = var1 + var2;
    return (s_calib.t_fine * 5 + 128) >> 8;
}

// 压力补偿：输入20位原始值，输出压力(Pa)
static uint32_t BMP280_CompensateP(int32_t adc_P)
{
    int32_t var1, var2;
    uint32_t p;

    var1 = (((int32_t)s_calib.t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)s_calib.dig_P6);
    var2 = var2 + ((var1 * (int32_t)s_calib.dig_P5) << 1);
    var2 = (var2 >> 2) + ((int32_t)s_calib.dig_P4 << 16);
    var1 = (((s_calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
            ((((int32_t)s_calib.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)s_calib.dig_P1)) >> 15);

    if (var1 == 0) {
        return 0;
    }

    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000) {
        p = (p << 1) / (uint32_t)var1;
    } else {
        p = (p / (uint32_t)var1) * 2;
    }

    var1 = (((int32_t)s_calib.dig_P9) *
            ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = ((int32_t)(p >> 2)) * ((int32_t)s_calib.dig_P8) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + s_calib.dig_P7) >> 4));

    return p;
}

// 组装20位原始数据：压力3字节+温度3字节，高位对齐
static void BMP280_AssembleRawData(const uint8_t *data, int32_t *adc_P, int32_t *adc_T)
{
    *adc_P = ((int32_t)data[0] << 12) |
             ((int32_t)data[1] << 4) |
             ((int32_t)data[2] >> 4);

    *adc_T = ((int32_t)data[3] << 12) |
             ((int32_t)data[4] << 4) |
             ((int32_t)data[5] >> 4);
}

bool BMP280_Init(void)
{
    uint8_t chip_id = 0;
    uint8_t ctrl_meas = 0;
    uint8_t config = 0;
    uint8_t reset_cmd = BMP280_RESET_CMD;

    // 1. 读取芯片ID验证通信
    if (!BSP_I2C_Mem_Read(BMP280_I2C_ADDR, BMP280_REG_ID,
                          I2C_MEMADD_SIZE_8BIT, &chip_id, 1)) {
        return false;
    }
    if (chip_id != BMP280_CHIP_ID) {
        return false;
    }

    // 2. 读取24字节校准系数
    if (!BMP280_ReadCalib()) {
        return false;
    }

    // 3. 软复位
    BSP_I2C_Mem_Write(BMP280_I2C_ADDR, BMP280_REG_RESET,
                      I2C_MEMADD_SIZE_8BIT, &reset_cmd, 1);
    HAL_Delay(50);

    // 4. 配置CONFIG寄存器：IIR滤波系数=8，待机时间=62.5ms
    config = BMP280_STANDBY_62_5MS | BMP280_FILTER_COEFF_8;
    BSP_I2C_Mem_Write(BMP280_I2C_ADDR, BMP280_REG_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &config, 1);
    HAL_Delay(10);

    // 5. 配置CTRL_MEAS：温度×2，压力×4，模式由宏选择
    ctrl_meas = BMP280_OSRS_T_X2 | BMP280_OSRS_P_X4 | BMP280_MEASUREMENT_MODE;
    if (!BSP_I2C_Mem_Write(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS,
                           I2C_MEMADD_SIZE_8BIT, &ctrl_meas, 1)) {
        return false;
    }
    HAL_Delay(10);

    HAL_Delay(50);
    s_is_init = true;
    return true;
}


bool BMP280_Read(float *pressure, float *temperature)
{
    uint8_t data[6] = {0};
    int32_t adc_P = 0;
    int32_t adc_T = 0;
    int32_t temp_comp = 0;
    uint32_t press_comp = 0;
    float abs_pressure = 0.0f;

    if (pressure == NULL || temperature == NULL) {
        return false;
    }

    if (!s_is_init) {
        return false;
    }

#if (BMP280_MEASUREMENT_MODE == BMP280_MODE_FORCED)
    // 强制模式下，每次读取前触发一次新测量
    {
        uint8_t ctrl = BMP280_OSRS_T_X2 | BMP280_OSRS_P_X4 | BMP280_MODE_FORCED;
        if (!BSP_I2C_Mem_Write(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS,
                               I2C_MEMADD_SIZE_8BIT, &ctrl, 1)) {
            return false;
        }
        HAL_Delay(BMP280_MEASURE_DELAY_MS);
    }
#endif

    // 等待测量完成
    if (!BMP280_WaitForMeasurement()) {
        return false;
    }

    // 突发读取6字节原始数据(0xF7-0xFC)
    if (!BSP_I2C_Mem_Read(BMP280_I2C_ADDR, BMP280_REG_PRESS_MSB,
                          I2C_MEMADD_SIZE_8BIT, data, 6)) {
        return false;
    }

    BMP280_AssembleRawData(data, &adc_P, &adc_T);

    temp_comp = BMP280_CompensateT(adc_T);
    *temperature = (float)temp_comp / 100.0f;

    press_comp = BMP280_CompensateP(adc_P);
    abs_pressure = (float)press_comp / 100.0f;

    // 软件调平：固定偏移补偿
    abs_pressure -= BMP280_PRESSURE_SOFT_OFFSET_HPA;
    if (abs_pressure < 0.0f) {
        abs_pressure = 0.0f;
    }

    *pressure = abs_pressure;

    return true;
}

const BMP280_Calib_t *BMP280_GetCalib(void)
{
    return &s_calib;
}
