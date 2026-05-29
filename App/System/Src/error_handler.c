// App/System/Src/error_handler.c
#include "error_handler.h"
#include "driver_uart.h"
#include "FreeRTOS.h"
#include "led.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static volatile ErrorType s_last_error = ERROR_NONE; // 最近错误类型，关中断保护
static volatile uint32_t  s_error_counter[ERROR_TYPE_COUNT]; // 各错误类型累计计数器

// 通过UART发送文本日志（受SYSTEM_UART_TEXT_LOG_ENABLE宏控制）
static void Error_SendText(const char* text) {
    if (text == NULL) {
        return;
    }

#if (SYSTEM_UART_TEXT_LOG_ENABLE == 1U)
    (void)UART2_Driver_DebugPrint(text);
#else
    (void)text; // 避免未使用参数警告
#endif
}

// HAL库错误入口：记录错误并进入安全模式
void Error_Handler(void) {
    ErrorLogRecord(ERROR_HAL, __FILE__, __LINE__);
    System_NotifyError(ERROR_HAL);
    System_EnterSafeMode();
}

// FreeRTOS栈溢出钩子：记录错误并处理故障
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    ErrorLogRecord(ERROR_STACK_OVERFLOW, __FILE__, __LINE__);
    System_HandleFault(ERROR_STACK_OVERFLOW);
}

// 获取最近一次发生的错误类型
ErrorType Error_GetLastType(void) {
    return s_last_error;
}

// 获取指定错误类型的累计发生次数
uint32_t Error_GetCount(ErrorType type) {
    if ((uint32_t)type >= ERROR_TYPE_COUNT) {
        return 0U;
    }
    return s_error_counter[type];
}

// 线程安全记录错误日志（中断上下文中跳过打印）
void ErrorLogRecord(ErrorType type, const char* file, int line) {
    if ((uint32_t)type >= ERROR_TYPE_COUNT) {
        return;
    }

    s_last_error = type;
    s_error_counter[type]++;

    // 中断上下文：仅递增计数器，跳过日志打印
    if (__get_IPSR() != 0U) {
        return;
    }

    char buf[96];
    int  n = snprintf(buf, sizeof(buf), "ERR[%u] %s:%d\r\n", (unsigned)type,
                      (file != NULL) ? file : "unknown", line);
    if (n > 0) {
        Error_SendText(buf);
    }
}

// 通知系统进入错误状态（串口告警+LED常亮）
void System_NotifyError(ErrorType type) {
    char buf[32];
    int  n = snprintf(buf, sizeof(buf), "SAFE MODE: %u\r\n", (unsigned)type);
    if (n > 0) {
        Error_SendText(buf);
    }
    BSP_LED_On();
}

// 进入安全模式：关中断+LED常亮+死循环（不可恢复）
void System_EnterSafeMode(void) {
    char buf[32];
    int  n = snprintf(buf, sizeof(buf), "SAFE_MODE_ENTER: ERR_TYPE=%u\r\n", (unsigned)s_last_error);
    if (n > 0) {
        Error_SendText(buf);
    }

    __disable_irq();
    BSP_LED_On();
    while (1) {
        // 安全停机，需硬件复位退出
    }
}

// 忙等待延时（禁用中断时使用，不依赖SysTick）
static void BusyDelay(volatile uint32_t loops) {
    while (loops-- != 0U) {
        __NOP();
    }
}

// LED闪烁模式：blink_count次亮灭 + 组间间隔
static void BlinkPattern(uint32_t blink_count) {
    for (uint32_t i = 0U; i < blink_count; i++) {
        BSP_LED_On();
        BusyDelay(220000U);
        BSP_LED_Off();
        BusyDelay(220000U);
    }
    BusyDelay(650000U); // 组间间隔
}

// 处理致命故障：根据错误类型执行不同LED闪烁模式（关中断+死循环）
void System_HandleFault(ErrorType type) {
    __disable_irq();
    s_last_error = type;
    if ((uint32_t)type < ERROR_TYPE_COUNT) {
        s_error_counter[type]++;
    }

    while (1) {
        switch (type) {
        case ERROR_STACK_OVERFLOW:
            BSP_LED_On(); // 栈溢出：LED常亮
            BusyDelay(900000U);
            break;
        case ERROR_HARDFAULT:
            BlinkPattern(1U); // HardFault：闪烁1次
            break;
        case ERROR_MEMMANAGE:
            BlinkPattern(2U); // MemManage：闪烁2次
            break;
        case ERROR_BUSFAULT:
            BlinkPattern(3U); // BusFault：闪烁3次
            break;
        case ERROR_USAGEFAULT:
            BlinkPattern(4U); // UsageFault：闪烁4次
            break;
        default:
            BlinkPattern(5U); // 未知/系统错误：闪烁5次
            break;
        }
    }
}
