// App/System/Inc/error_handler.h
#ifndef __ERROR_HANDLER_H__
#define __ERROR_HANDLER_H__

#include <stdint.h>

/* ================= Error Handler Configuration ================= */
/* Timeout for sending error logs via UART (ms) */
#define ERROR_LOG_SEND_TIMEOUT_MS (20U)

/* LED blink timing parameters for fault indication (busy loops) */
#define ERROR_LED_BLINK_DELAY_LOOPS (220000U)
#define ERROR_LED_GROUP_GAP_LOOPS (650000U)
#define ERROR_STACK_OVERFLOW_HOLD_LOOPS (900000U)
#define ERROR_LED_BLINK_COUNT_DEFAULT (5U)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ERROR_NONE = 0,
    ERROR_HAL,
    ERROR_UART,
    ERROR_MODBUS,
    ERROR_STACK_OVERFLOW,
    ERROR_SYSTEM,
    ERROR_HARDFAULT,
    ERROR_MEMMANAGE,
    ERROR_BUSFAULT,
    ERROR_USAGEFAULT,
    ERROR_MAX
} ErrorType;

/* Internal constant: Number of error types (must be defined after enum) */
#define ERROR_TYPE_COUNT ((uint32_t)ERROR_MAX)

void ErrorLogRecord(ErrorType type, const char* file, int line);
void ErrorLogRecord(ErrorType type, const char* file, int line);
void System_NotifyError(ErrorType type);
void System_EnterSafeMode(void);
void System_HandleFault(ErrorType type);

ErrorType Error_GetLastType(void);
uint32_t  Error_GetCount(ErrorType type);
void      Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __ERROR_HANDLER_H__ */
