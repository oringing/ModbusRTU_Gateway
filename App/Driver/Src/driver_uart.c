// App/Src/driver/driver_uart.c
#include "driver_uart.h"
#include "cmsis_os.h"
#include "task.h"
#include "uart.h"

static osMutexId s_uart_mutex = NULL;  // UART1(Modbus)互斥锁，调度器启动后创建
static osMutexId s_uart2_mutex = NULL; // UART2(调试日志)互斥锁，调度器启动后创建

// 懒加载创建 UART1 互斥锁，避免 FreeRTOS 启动顺序依赖
static void UART1_Driver_EnsureMutex(void) {
    if (s_uart_mutex != NULL) {
        return;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        osMutexDef(DriverUartMutex);
        s_uart_mutex = osMutexCreate(osMutex(DriverUartMutex));
    }
}

// 懒加载创建 UART2 互斥锁，避免 FreeRTOS 启动顺序依赖
static void UART2_Driver_EnsureDebugMutex(void) {
    if (s_uart2_mutex != NULL) {
        return;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        osMutexDef(DriverUart2Mutex);
        s_uart2_mutex = osMutexCreate(osMutex(DriverUart2Mutex));
    }
}

bool UART_Driver_Init(void) {
    UART1_Driver_EnsureMutex();
    UART2_Driver_EnsureDebugMutex();
    return true;
}

bool UART2_Driver_DebugPrint(const char* str) {
    if (str == NULL) {
        return false;
    }

    UART2_Driver_EnsureDebugMutex();

    if (s_uart2_mutex == NULL) {
        BSP_UART2_DebugPrint(str);
        return true;
    }

    if (osMutexWait(s_uart2_mutex, 20U) != osOK) {
        return false;
    }

    BSP_UART2_DebugPrint(str);
    osMutexRelease(s_uart2_mutex);
    return true;
}

// 线程安全发送：调度器未启动时直调BSP，启动后加锁保护
bool UART1_Driver_ModbusSend(const uint8_t* data, uint16_t len, uint32_t timeout) {
    if (data == NULL || len == 0U) {
        return false;
    }

    UART1_Driver_EnsureMutex();

    // 调度器未启动，直接发送（无锁保护）
    if (s_uart_mutex == NULL) {
        return BSP_UART1_ModbusSend(data, len, timeout);
    }

    if (osMutexWait(s_uart_mutex, timeout) != osOK) {
        return false;
    }

    bool ok = BSP_UART1_ModbusSend(data, len, timeout);
    osMutexRelease(s_uart_mutex);
    return ok;
}

// 超时轮询接收：timeout=0非阻塞查询，>0时每1ms检查一次
uint16_t UART_Driver_Receive(uint8_t* buffer, uint16_t max_len, uint32_t timeout) {
    if (buffer == NULL || max_len == 0U) {
        return 0;
    }

    uint32_t start = HAL_GetTick();
    if (timeout == 0U) {
        // 非阻塞模式：立即返回
        if (BSP_UART_IsFrameReady()) {
            return BSP_UART_ReadFrame(buffer, max_len);
        }
        return 0U;
    }

    // 阻塞轮询：每1ms检查一次帧就绪状态
    while ((HAL_GetTick() - start) <= timeout) {
        if (BSP_UART_IsFrameReady()) {
            return BSP_UART_ReadFrame(buffer, max_len);
        }
        osDelay(1);
    }

    return 0;
}
