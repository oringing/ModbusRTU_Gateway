// App/System/Inc/error_handler.h
#ifndef __ERROR_HANDLER_H__
#define __ERROR_HANDLER_H__

#include <stdint.h>

// ---- 错误日志配置（性能调优）----
#define ERROR_LOG_SEND_TIMEOUT_MS (20U) // 错误日志发送超时(ms)，避免阻塞故障处理流程

// ---- LED故障指示时序参数（忙等待循环，禁用中断时使用）----
#define ERROR_LED_BLINK_DELAY_LOOPS (220000U)       // 单次LED亮/灭延时(循环次数)
#define ERROR_LED_GROUP_GAP_LOOPS (650000U)         // 闪烁组间间隔(循环次数)
#define ERROR_STACK_OVERFLOW_HOLD_LOOPS (900000U)   // 栈溢出常亮保持时间(循环次数)
#define ERROR_LED_BLINK_COUNT_DEFAULT (5U)          // 默认闪烁次数（未知错误）

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   错误类型枚举
 * @note    新增错误类型需在ERROR_MAX前添加，并更新System_HandleFault()的switch分支
 */
typedef enum {
    ERROR_NONE = 0,           // 无错误
    ERROR_HAL,                // HAL库错误
    ERROR_UART,               // UART通信错误
    ERROR_MODBUS,             // Modbus协议错误
    ERROR_STACK_OVERFLOW,     // 任务栈溢出
    ERROR_SYSTEM,             // 系统级错误
    ERROR_HARDFAULT,          // 硬件故障（HardFault异常）
    ERROR_MEMMANAGE,          // 内存管理故障（MemManage异常）
    ERROR_BUSFAULT,           // 总线故障（BusFault异常）
    ERROR_USAGEFAULT,         // 用法故障（UsageFault异常）
    ERROR_MAX                 // 错误类型总数（用于数组边界检查）
} ErrorType;

// 内部常量：错误类型数量（必须在枚举后定义）
#define ERROR_TYPE_COUNT ((uint32_t)ERROR_MAX)

/**
 * @brief   记录错误日志（线程安全）
 * @param   type 错误类型
 * @param   file 源文件名（__FILE__）
 * @param   line 行号（__LINE__）
 * @warning 中断上下文中跳过日志打印，仅递增计数器
 */
void ErrorLogRecord(ErrorType type, const char* file, int line);

/**
 * @brief   通知系统进入错误状态（触发LED指示+串口告警）
 * @param   type 错误类型
 * @warning 此函数不阻塞，立即返回
 */
void System_NotifyError(ErrorType type);

/**
 * @brief   进入安全模式（关中断+LED常亮+死循环）
 * @warning 不可恢复操作，需硬件复位退出
 */
void System_EnterSafeMode(void);

/**
 * @brief   处理致命故障（根据错误类型执行不同LED闪烁模式）
 * @param   type 错误类型
 *          - ERROR_STACK_OVERFLOW: LED常亮
 *          - ERROR_HARDFAULT: 闪烁1次
 *          - ERROR_MEMMANAGE: 闪烁2次
 *          - ERROR_BUSFAULT: 闪烁3次
 *          - ERROR_USAGEFAULT: 闪烁4次
 *          - 其他: 闪烁5次
 * @warning 关中断+死循环，需硬件复位退出
 */
void System_HandleFault(ErrorType type);

/**
 * @brief   获取最后一次错误类型
 * @return  最近记录的错误类型，ERROR_NONE表示无错误
 */
ErrorType Error_GetLastType(void);

/**
 * @brief   获取指定错误类型的累计发生次数
 * @param   type 错误类型
 * @return  累计次数，type无效时返回0
 */
uint32_t Error_GetCount(ErrorType type);

/**
 * @brief   通用错误处理器（HAL库回调入口）
 * @warning 记录ERROR_HAL并进入安全模式
 */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __ERROR_HANDLER_H__ */
