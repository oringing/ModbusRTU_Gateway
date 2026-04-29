//App/System/Src/error_handler.c
#include "error_handler.h"
#include "led.h"
#include "driver_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define ERROR_TYPE_COUNT ((uint32_t)ERROR_MAX)

static volatile ErrorType s_last_error = ERROR_NONE;
static volatile uint32_t s_error_counter[ERROR_TYPE_COUNT];

static void Error_SendText(const char *text)
{
    if (text == NULL) {
        return;
    }

    (void)UART_Driver_Send((const uint8_t *)text, (uint16_t)strlen(text), 20U);
}

void Error_Handler(void)
{
    ErrorLogRecord(ERROR_HAL, __FILE__, __LINE__);
    System_NotifyError(ERROR_HAL);
    System_EnterSafeMode();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    ErrorLogRecord(ERROR_STACK_OVERFLOW, __FILE__, __LINE__);
    System_HandleFault(ERROR_STACK_OVERFLOW);
}

ErrorType Error_GetLastType(void)
{
    return s_last_error;
}

uint32_t Error_GetCount(ErrorType type)
{
    if ((uint32_t)type >= ERROR_TYPE_COUNT) {
        return 0U;
    }
    return s_error_counter[type];
}

void ErrorLogRecord(ErrorType type, const char *file, int line)
{
    if ((uint32_t)type >= ERROR_TYPE_COUNT) {
        return;
    }

    s_last_error = type;
    s_error_counter[type]++;

    /* Avoid heavy log in interrupt context. */
    if (__get_IPSR() != 0U) {
        return;
    }

    char buf[96];
    int n = snprintf(buf, sizeof(buf), "ERR[%u] %s:%d\r\n", (unsigned)type,
                     (file != NULL) ? file : "unknown",
                     line);
    if (n > 0) {
        Error_SendText(buf);
    }
}

void System_NotifyError(ErrorType type)
{
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "SAFE MODE: %u\r\n", (unsigned)type);
    if (n > 0) {
        Error_SendText(buf);
    }
    BSP_LED_On();
}

void System_EnterSafeMode(void)
{
    __disable_irq();
    BSP_LED_On();
    while (1) {
        /* Safe stop */
    }
}

static void BusyDelay(volatile uint32_t loops)
{
    while (loops-- != 0U) {
        __NOP();
    }
}

static void BlinkPattern(uint32_t blink_count)
{
    for (uint32_t i = 0U; i < blink_count; i++) {
        BSP_LED_On();
        BusyDelay(220000U);
        BSP_LED_Off();
        BusyDelay(220000U);
    }
    BusyDelay(650000U); /* Gap between groups */
}

void System_HandleFault(ErrorType type)
{
    __disable_irq();
    s_last_error = type;
    if ((uint32_t)type < ERROR_TYPE_COUNT) {
        s_error_counter[type]++;
    }

    while (1) {
        switch (type) {
        case ERROR_STACK_OVERFLOW:
            BSP_LED_On();            /* Stack overflow: solid ON */
            BusyDelay(900000U);
            break;
        case ERROR_HARDFAULT:
            BlinkPattern(1U);        /* 1 blink */
            break;
        case ERROR_MEMMANAGE:
            BlinkPattern(2U);        /* 2 blinks */
            break;
        case ERROR_BUSFAULT:
            BlinkPattern(3U);        /* 3 blinks */
            break;
        case ERROR_USAGEFAULT:
            BlinkPattern(4U);        /* 4 blinks */
            break;
        default:
            BlinkPattern(5U);        /* unknown/system */
            break;
        }
    }
}
