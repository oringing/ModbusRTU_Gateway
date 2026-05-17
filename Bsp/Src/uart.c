// Bsp/Src/uart.c
#include "uart.h"
#include "cmsis_os.h"
#include "error_handler.h"
#include "led.h"
#include "system_config.h"
#include <string.h>

// 私有变量：UART句柄及双缓冲区
static UART_HandleTypeDef huart1;
static uint8_t            s_rx_buffer_a[BSP_UART_RX_BUF_SIZE]; // 接收缓冲区A
static uint8_t            s_rx_buffer_b[BSP_UART_RX_BUF_SIZE]; // 接收缓冲区B
static uint8_t*           s_active_rx_buffer = s_rx_buffer_a;  // 当前活动缓冲区，仅ISR写入
static uint8_t*           s_ready_rx_buffer = s_rx_buffer_b;   // 已就绪缓冲区，任务读取后交换
static volatile uint16_t  rx_count = 0;                        // 当前帧接收字节计数
static volatile uint16_t  s_ready_frame_len = 0U;              // 已就绪帧长度
static volatile bool      frame_ready = false;                 // 帧就绪标志，受关中断保护
static volatile bool      s_rx_need_recovery = false;          // 恢复请求标志
static volatile uint32_t  s_uart_error_count = 0U;             // 总错误计数
static volatile uint32_t  s_uart_rx_overflow_count = 0U;       // 接收溢出计数
static volatile uint32_t  s_uart_ore_count = 0U;               // ORE错误计数
static volatile uint32_t  s_uart_fe_count = 0U;                // FE错误计数
static volatile uint32_t  s_uart_ne_count = 0U;                // NE错误计数
static volatile uint32_t  s_uart_pe_count = 0U;                // PE错误计数
static volatile uint32_t  s_uart_error_streak = 0U;            // 连续错误计数
static volatile uint32_t  s_uart_irq_reentry_count = 0U;       // IRQ重入计数
static volatile uint8_t   s_uart_irq_busy = 0U;                // IRQ忙标志，防止重入
static volatile uint32_t  s_uart_fe_streak = 0U;               // FE/NE连续错误计数
static volatile uint32_t  s_uart_pe_streak = 0U;               // PE连续错误计数
static volatile uint8_t   s_uart_tx_in_progress = 0U;          // TX进行中标志，用于过滤回显
static uint32_t           s_last_error_log_tick = 0U;          // 上次错误日志时间戳
static uint32_t           s_last_recover_try_tick = 0U;        // 上次恢复尝试时间戳
static uint32_t           s_last_recover_fail_log_tick = 0U;   // 上次恢复失败日志时间戳
static uint32_t           s_last_post_frame_restart_log_tick = 0U; // 上次帧后重启日志时间戳

static uint32_t s_recovery_retry_count = 0U;  // 恢复重试计数
static bool     s_uart_hw_fault = false;      // 硬件故障标志
static uint32_t s_success_rx_count = 0U;      // 连续成功接收计数

// 内部函数声明
static HAL_StatusTypeDef BSP_UART_StartReceiveByteRaw(void);
static bool BSP_UART_StartReceiveByte(void);
static bool BSP_UART_RestartReceiveFromBufferHead(HAL_StatusTypeDef* restart_status);
static bool BSP_UART_ResetStateAndRestartReceive(HAL_StatusTypeDef* restart_status);
static void BSP_UART_RequestRecoveryFromISR(void);
static void BSP_UART_FinalizeFrameFromISR(void);
static bool BSP_UART_ShouldThrottleLog(void);
static void BSP_UART_ClassifyAndHandleErrorsFromISR(UART_HandleTypeDef* huart);
static void BSP_UART_RecoveryIfNeeded(void);

// 启动单字节接收（裸接口，无状态检查）
static HAL_StatusTypeDef BSP_UART_StartReceiveByteRaw(void) {
    return HAL_UART_Receive_IT(&huart1, &s_active_rx_buffer[rx_count], 1U);
}

// 启动单字节接收（带返回值转换）
static bool BSP_UART_StartReceiveByte(void) {
    return (BSP_UART_StartReceiveByteRaw() == HAL_OK);
}

// 从缓冲区头部重启接收（调用者需先完成缓冲区交换）
static bool BSP_UART_RestartReceiveFromBufferHead(HAL_StatusTypeDef* restart_status) {
    HAL_StatusTypeDef st = HAL_UART_AbortReceive(&huart1);
    if ((st != HAL_OK) && (st != HAL_BUSY)) {
        if (restart_status != NULL) {
            *restart_status = st;
        }
        return false;
    }

    st = BSP_UART_StartReceiveByteRaw();
    if (restart_status != NULL) {
        *restart_status = st;
    }
    return (st == HAL_OK);
}

// 重置状态并重启接收（清零计数器+关闭中断保护）
static bool BSP_UART_ResetStateAndRestartReceive(HAL_StatusTypeDef* restart_status) {
    __disable_irq();
    rx_count = 0U;
    s_ready_frame_len = 0U;
    frame_ready = false;
    __enable_irq();

    return BSP_UART_RestartReceiveFromBufferHead(restart_status);
}

// 从中断上下文请求恢复
static void BSP_UART_RequestRecoveryFromISR(void) {
    s_rx_need_recovery = true;
}

// IDLE中断触发：封帧并交换缓冲区
static void BSP_UART_FinalizeFrameFromISR(void) {
    HAL_StatusTypeDef restart_status = HAL_OK;
    uint8_t*          sealed_buffer = NULL;
    uint8_t*          new_active_buffer = NULL;

    if (rx_count == 0U) {
        return;
    }

    // 防止帧覆盖：如果上一帧未读取，丢弃当前帧并请求恢复
    if (frame_ready) {
        s_uart_rx_overflow_count++;
        BSP_UART_RequestRecoveryFromISR();
        return;
    }

    // 交换双缓冲区
    sealed_buffer = s_active_rx_buffer;
    new_active_buffer = s_ready_rx_buffer;
    s_active_rx_buffer = new_active_buffer;
    s_ready_rx_buffer = sealed_buffer;
    s_ready_frame_len = rx_count;
    rx_count = 0U;
    frame_ready = true;

    // 检测硬件故障恢复：连续5次成功接收则清除故障标志
    if (s_uart_hw_fault) {
        s_success_rx_count++;
        if (s_success_rx_count >= UART_SUCCESS_RX_RECOVER_COUNT) {
            s_uart_hw_fault = false;
            s_recovery_retry_count = 0U;
            s_success_rx_count = 0U;
        }
    } else {
        s_success_rx_count = 0U;
    }

    // 重启接收链，失败则标记需要恢复
    if (!BSP_UART_RestartReceiveFromBufferHead(&restart_status)) {
        s_rx_need_recovery = true;
        if ((restart_status != HAL_BUSY) &&
            ((HAL_GetTick() - s_last_post_frame_restart_log_tick) >= 1000U)) {
            s_last_post_frame_restart_log_tick = HAL_GetTick();
            ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
        }
    }
}

// 日志限流：1秒内只记录一次错误
static bool BSP_UART_ShouldThrottleLog(void) {
    uint32_t now = HAL_GetTick();
    if ((now - s_last_error_log_tick) >= UART_ERROR_LOG_THROTTLE_MS) {
        s_last_error_log_tick = now;
        return false;
    }
    return true;
}

// 错误分级处理：根据错误类型决定是否立即恢复
static void BSP_UART_ClassifyAndHandleErrorsFromISR(UART_HandleTypeDef* huart) {
    uint32_t error_code = 0U;
    bool     has_error = false;
    bool     need_immediate_recover = false;

    if (huart == NULL) {
        return;
    }

    error_code = huart->ErrorCode;
    if (error_code == HAL_UART_ERROR_NONE) {
        s_uart_error_streak = 0U;
        s_uart_fe_streak = 0U;
        s_uart_pe_streak = 0U;
        return;
    }

    // ORE（溢出错误）：数据接收过快，立即恢复
    if ((error_code & HAL_UART_ERROR_ORE) != 0U) {
        s_uart_ore_count++;
        has_error = true;
        need_immediate_recover = true;
    }

    // FE/NE（帧/噪声错误）：连续3次触发恢复
    if ((error_code & (HAL_UART_ERROR_FE | HAL_UART_ERROR_NE)) != 0U) {
        if ((error_code & HAL_UART_ERROR_FE) != 0U) {
            s_uart_fe_count++;
        }
        if ((error_code & HAL_UART_ERROR_NE) != 0U) {
            s_uart_ne_count++;
        }
        s_uart_fe_streak++;
        has_error = true;

        if (s_uart_fe_streak >= UART_FE_RECOVER_STREAK_TH) {
            need_immediate_recover = true;
        }
    }

    // PE（校验错误）：连续5次触发恢复
    if ((error_code & HAL_UART_ERROR_PE) != 0U) {
        s_uart_pe_count++;
        s_uart_pe_streak++;
        has_error = true;

        if (s_uart_pe_streak >= UART_PE_RECOVER_STREAK_TH) {
            need_immediate_recover = true;
        }
    }

    if (!has_error) {
        s_uart_error_streak = 0U;
        return;
    }

    s_uart_error_count++;
    s_uart_error_streak++;

    // STM32F1系列需手动清除错误标志
    __HAL_UART_CLEAR_PEFLAG(huart);

    // 立即恢复或达到总阈值则触发恢复
    if (need_immediate_recover || (s_uart_error_streak >= UART_ERROR_STREAK_RECOVER_TH)) {
        BSP_UART_RequestRecoveryFromISR();
    }

    // 限流记录错误日志
    if (!BSP_UART_ShouldThrottleLog()) {
        ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
    }
}

// 任务层周期性调用：执行恢复逻辑
static void BSP_UART_RecoveryIfNeeded(void) {
    HAL_StatusTypeDef restart_status = HAL_OK;

    if (!s_rx_need_recovery) {
        return;
    }

    // 如果帧已就绪但未读取，延迟恢复直到任务取走数据
    if (frame_ready) {
        return;
    }

    // 硬件故障状态下停止自动恢复
    if (s_uart_hw_fault) {
        return;
    }

    // 防止热循环：最小间隔5ms
    if ((HAL_GetTick() - s_last_recover_try_tick) < 5U) {
        return;
    }
    s_last_recover_try_tick = HAL_GetTick();

    s_rx_need_recovery = false;

    if (!BSP_UART_ResetStateAndRestartReceive(&restart_status)) {
        s_recovery_retry_count++;

        // HAL_BUSY是暂时性状态，不计入错误
        if (restart_status != HAL_BUSY) {
            s_uart_error_count++;
            if ((HAL_GetTick() - s_last_recover_fail_log_tick) >= 1000U) {
                s_last_recover_fail_log_tick = HAL_GetTick();
                ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
            }
        }

        // 超过最大重试次数则判定为硬件故障
        if (s_recovery_retry_count >= UART_MAX_RECOVERY_RETRY) {
            s_uart_hw_fault = true;
            s_rx_need_recovery = false;

            // 故障反馈：记录日志 + LED快速闪烁
            ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__);
            for (uint32_t i = 0U; i < UART_FAULT_BLINK_COUNT; i++) {
                BSP_LED_On();
                HAL_Delay(UART_FAULT_BLINK_INTERVAL_MS);
                BSP_LED_Off();
                HAL_Delay(UART_FAULT_BLINK_INTERVAL_MS);
            }
        } else {
            s_rx_need_recovery = true; // 未超限，继续标记需要恢复
        }
    } else {
        // 恢复成功，重置计数器
        s_recovery_retry_count = 0U;
    }
}

void BSP_UART_Init(void) {
    // 使能时钟
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 配置GPIO：TX推挽输出，RX强上拉输入（抗干扰）
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = BSP_UART1_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_UART_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BSP_UART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_UART_GPIO_PORT, &GPIO_InitStruct);

    // 配置UART参数
    huart1.Instance = BSP_UART_INSTANCE;
    huart1.Init.BaudRate = BSP_UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    // 开启NVIC中断（优先级5）
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    // 开启IDLE中断
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    // 启动首次接收
    rx_count = 0U;
    s_ready_frame_len = 0U;
    frame_ready = false;
    s_rx_need_recovery = false;
    if (!BSP_UART_StartReceiveByte()) {
        s_uart_error_count++;
        ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
        Error_Handler();
    }
}

bool BSP_UART_Send(const uint8_t* data, uint16_t len, uint32_t timeout) {
    bool status = false;
    if ((data != NULL) && (len > 0U)) {
        // 标记TX窗口，过滤RS485回显字节
        s_uart_tx_in_progress = 1U;
        status = (HAL_UART_Transmit(&huart1, (uint8_t*)data, len, timeout) == HAL_OK);
        s_uart_tx_in_progress = 0U;
    }
    return status;
}

void BSP_UART_PrintString(const char* str) {
    if (str != NULL) {
        (void)BSP_UART_Send((const uint8_t*)str, (uint16_t)strlen(str), BSP_UART_TX_TIMEOUT);
    }
}

// RX完成回调：逐字节接收并检测溢出
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART1) {
        // 过滤TX回显字节（RS485半双工场景）
        if (s_uart_tx_in_progress != 0U) {
            if (HAL_UART_Receive_IT(huart, &s_active_rx_buffer[rx_count], 1U) != HAL_OK) {
                BSP_UART_RequestRecoveryFromISR();
            }
            return;
        }

        // 正常接收：递增计数并启动下一字节
        if (rx_count < BSP_UART_RX_BUF_SIZE) {
            rx_count++;
            if (rx_count < BSP_UART_RX_BUF_SIZE) {
                if (HAL_UART_Receive_IT(huart, &s_active_rx_buffer[rx_count], 1U) != HAL_OK) {
                    if (huart->RxState == HAL_UART_STATE_BUSY_RX) {
                        BSP_UART_RequestRecoveryFromISR();
                    } else {
                        s_uart_error_count++;
                        BSP_UART_RequestRecoveryFromISR();
                    }
                }
            } else {
                // 缓冲区溢出
                s_uart_rx_overflow_count++;
                BSP_UART_RequestRecoveryFromISR();
            }
        } else {
            s_uart_rx_overflow_count++;
            BSP_UART_RequestRecoveryFromISR();
        }
    }
}

// 错误回调：分类处理FE/NE/PE/ORE
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if (huart == NULL || huart->Instance != USART1) {
        return;
    }

    BSP_UART_ClassifyAndHandleErrorsFromISR(huart);
}

// IDLE中断入口：关中断保护原子性操作
void BSP_UART_IRQHandler(void) {
    // 防止IRQ重入
    __disable_irq();
    if (s_uart_irq_busy != 0U) {
        s_uart_irq_reentry_count++;
        __enable_irq();
        if (s_uart_irq_reentry_count >= UART_IRQ_REENTRY_RECOVER_TH) {
            s_uart_error_count++;
            __HAL_UART_CLEAR_PEFLAG(&huart1);
            BSP_UART_RequestRecoveryFromISR();
            s_uart_irq_reentry_count = 0U;
        }
        return;
    }
    s_uart_irq_busy = 1U;
    __enable_irq();

    // 分发到HAL库处理RX/TX回调
    HAL_UART_IRQHandler(&huart1);

    // IDLE标志检测：封帧并立即重启接收
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        BSP_UART_FinalizeFrameFromISR();
    }

    s_uart_irq_busy = 0U;
}

bool BSP_UART_IsFrameReady(void) {
    BSP_UART_RecoveryIfNeeded();
    return frame_ready;
}

uint16_t BSP_UART_ReadFrame(uint8_t* buffer, uint16_t max_len) {
    uint16_t snapshot_len = 0U;
    uint16_t copy_len = 0U;

    BSP_UART_RecoveryIfNeeded();

    // 参数验证
    if (buffer == NULL || max_len == 0U || max_len > BSP_UART_RX_BUF_SIZE) {
        return 0U;
    }

    // 关中断保护原子性读取
    __disable_irq();
    if (frame_ready) {
        snapshot_len = s_ready_frame_len;
        s_ready_frame_len = 0U;
        frame_ready = false;
    }
    __enable_irq();

    if (snapshot_len > 0U) {
        // 防止越界：取最小值
        copy_len = (snapshot_len < max_len) ? snapshot_len : max_len;
        if (copy_len > BSP_UART_RX_BUF_SIZE) {
            copy_len = BSP_UART_RX_BUF_SIZE;
        }
        memcpy(buffer, s_ready_rx_buffer, copy_len);
    }

    return copy_len;
}
