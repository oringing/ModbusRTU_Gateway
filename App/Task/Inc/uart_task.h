// App/Inc/uart_task.h
#ifndef __UART_TASK_H
#define __UART_TASK_H

#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   UART任务入口函数（处理Modbus协议）
 * @param   argument 任务参数（未使用）
 * @warning 调用UART_Task_RequestStop()后会自动退出并销毁任务
 */
void Start_UART_Task(void const* argument);

/**
 * @brief   请求UART任务停止（优雅退出）
 * @warning 设置停止标志后，任务会在处理完当前帧后退出
 */
void UART_Task_RequestStop(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_TASK_H */
