//App/System/Inc/error_handler.h
#ifndef __ERROR_HANDLER_H__
#define __ERROR_HANDLER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ERROR_INFO,
    ERROR_WARNING,
    ERROR_ERROR,
    ERROR_CRITICAL
} ErrorLevel_t;//测试用

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

void ErrorLogRecord(ErrorType type, const char *file, int line);
void System_NotifyError(ErrorType type);
void System_EnterSafeMode(void);
void System_HandleFault(ErrorType type);

ErrorType Error_GetLastType(void);
uint32_t Error_GetCount(ErrorType type);

// 错误处理函数声明测试
void ErrorHandler(const char* msg, ErrorLevel_t level);

#ifdef __cplusplus
}
#endif

#endif /* __ERROR_HANDLER_H__ */
