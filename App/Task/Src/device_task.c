// App/Task/Src/device_task.c
#include "device_task.h"
#include "cmsis_os.h"
#include "driver_env_sensor.h"
#include "driver_led.h"
#include "driver_uart.h"
#include "modbus.h"
#include <stdio.h>

static volatile uint8_t s_device_task_stop = 0U; // 停止标志，任务内循环检查
static uint32_t         s_last_led_tick = 0;     // 上次LED翻转的系统滴答
static uint32_t         s_last_sensor_tick = 0;  // 上次传感器读取的系统滴答

// 温度转换：float → Modbus存储值（×10 + 1000偏移，支持零下温度）
static int16_t EnvSensor_ConvertTempToReg(float temp) {
    if (temp < -40.0f)
        temp = -40.0f;
    if (temp > 85.0f)
        temp = 85.0f;
    return (int16_t)(temp * 10.0f + 0.5f) + 1000;
}

// 湿度转换：float → Modbus存储值（×10，保留一位小数）
static uint16_t EnvSensor_ConvertHumiToReg(float humi) {
    if (humi < 0.0f)
        humi = 0.0f;
    if (humi > 100.0f)
        humi = 100.0f;
    return (uint16_t)(humi * 10.0f + 0.5f);
}

// 气压转换：float → Modbus存储值（×10，保留一位小数）
static uint16_t EnvSensor_ConvertPressToReg(float press) {
    if (press < 300.0f)
        press = 300.0f;
    if (press > 1100.0f)
        press = 1100.0f;
    return (uint16_t)(press * 10.0f + 0.5f);
}

// 读取传感器数据并更新Modbus寄存器（0x0000-0x0002）和状态位（0x0003 Bit4-5）
static void Device_ReadSensors(void) {
    float temp, humi, press;
    bool  ok = EnvSensor_Driver_ReadData(&temp, &humi, &press);

    if (ok) {
        // 更新传感器寄存器 0x0000-0x0002
        int16_t  temp_reg = EnvSensor_ConvertTempToReg(temp);
        uint16_t humi_reg = EnvSensor_ConvertHumiToReg(humi);
        uint16_t press_reg = EnvSensor_ConvertPressToReg(press);

        Modbus_InternalWriteRegister(0x0000, (uint16_t)temp_reg);
        Modbus_InternalWriteRegister(0x0001, humi_reg);
        Modbus_InternalWriteRegister(0x0002, press_reg);

#if DEVICE_SENSOR_DEBUG_LOG_ENABLE
        char buf[80];
        snprintf(buf, sizeof(buf), "[SENSOR] T=%.1fC, H=%.1f%%, P=%.1fhPa\r\n", temp, humi, press);
        UART2_Driver_DebugPrint(buf);
#endif
    } else {
        // 读取失败时的处理（可选：打印警告）
#if DEVICE_SENSOR_DEBUG_LOG_ENABLE
        UART2_Driver_DebugPrint("[SENSOR] Read failed, using last valid values\r\n");
#endif
    }

    // 更新状态寄存器 0x0003 的 Bit4-5（传感器故障标志）
    uint16_t status_reg;
    if (Modbus_ReadHoldingRegister(0x0003, &status_reg)) {
        uint8_t sensor_status = EnvSensor_Driver_GetStatus();
        status_reg &= ~((1U << 4) | (1U << 5));
        status_reg |= (sensor_status & ((1U << 4) | (1U << 5)));
        Modbus_InternalWriteRegister(0x0003, status_reg);
    }
}

void Start_Device_Task(void const* argument) {
    (void)argument;

    // 初始化传感器驱动
    EnvSensor_Driver_Init();

    // 初始化 LED 驱动（设置心跳模式）
    LED_Driver_Init();
    LED_Driver_SetMode(LED_MODE_HEARTBEAT);

    s_device_task_stop = 0U;
    s_last_led_tick = osKernelSysTick();
    s_last_sensor_tick = osKernelSysTick();

    for (;;) {
        if (s_device_task_stop != 0U)
            break;

        uint32_t current_tick = osKernelSysTick();

        // 1. LED 更新（Driver 层内部处理闪烁逻辑）
        if (current_tick - s_last_led_tick >= DEVICE_LED_UPDATE_INTERVAL_MS) {
            LED_Driver_Update();
            s_last_led_tick = current_tick;
        }

        // 2. 传感器读取（1000ms）
        if (current_tick - s_last_sensor_tick >= DEVICE_SENSOR_INTERVAL_MS) {
            Device_ReadSensors();
            s_last_sensor_tick = current_tick;
        }

        osDelay(DEVICE_TASK_BASE_DELAY_MS);
    }

    // 退出前关闭 LED
    LED_Driver_SetMode(LED_MODE_OFF);
    osThreadTerminate(NULL);
}

void Device_Task_RequestStop(void) {
    s_device_task_stop = 1U;
}
