// Bsp/Src/uart.c
#include "uart.h"
#include "error_handler.h"
#include <string.h>

// 内部私有变量
static UART_HandleTypeDef huart1; 
static uint8_t s_rx_buffer_a[BSP_UART_RX_BUF_SIZE];
static uint8_t s_rx_buffer_b[BSP_UART_RX_BUF_SIZE];
static uint8_t *s_active_rx_buffer = s_rx_buffer_a;
static uint8_t *s_ready_rx_buffer = s_rx_buffer_b;
static volatile uint16_t rx_count = 0;
static volatile uint16_t s_ready_frame_len = 0U;
static volatile bool frame_ready = false;
static volatile bool s_rx_need_recovery = false;
static volatile uint32_t s_uart_error_count = 0U;
static volatile uint32_t s_uart_rx_overflow_count = 0U;
static volatile uint32_t s_uart_ore_count = 0U;
static volatile uint32_t s_uart_fe_count = 0U;
static volatile uint32_t s_uart_ne_count = 0U;
static volatile uint32_t s_uart_pe_count = 0U;
static volatile uint32_t s_uart_error_streak = 0U;
static volatile uint32_t s_uart_irq_reentry_count = 0U;
static volatile uint8_t s_uart_irq_busy = 0U;
static volatile uint8_t s_uart_tx_in_progress = 0U;
static uint32_t s_last_error_log_tick = 0U;
static uint32_t s_last_recover_try_tick = 0U;
static uint32_t s_last_recover_fail_log_tick = 0U;
static uint32_t s_last_post_frame_restart_log_tick = 0U;



static HAL_StatusTypeDef BSP_UART_StartReceiveByteRaw(void)
{
    return HAL_UART_Receive_IT(&huart1, &s_active_rx_buffer[rx_count], 1U);
}

static bool BSP_UART_StartReceiveByte(void)
{
    return (BSP_UART_StartReceiveByteRaw() == HAL_OK);
}

static bool BSP_UART_RestartReceiveFromBufferHead(HAL_StatusTypeDef *restart_status)
{
    HAL_StatusTypeDef st = HAL_UART_AbortReceive(&huart1);
    if ((st != HAL_OK) && (st != HAL_BUSY)) {
        if (restart_status != NULL) {
            *restart_status = st;
        }
        return false;
    }

    /* Caller has already swapped buffers and cleared the frame state.
     * This helper should only re-arm RX on the new active buffer head. */
    st = BSP_UART_StartReceiveByteRaw();
    if (restart_status != NULL) {
        *restart_status = st;
    }
    return (st == HAL_OK);
}

static bool BSP_UART_ResetStateAndRestartReceive(HAL_StatusTypeDef *restart_status)
{
    __disable_irq();
    rx_count = 0U;
    s_ready_frame_len = 0U;
    frame_ready = false;
    __enable_irq();

    return BSP_UART_RestartReceiveFromBufferHead(restart_status);
}

static void BSP_UART_RequestRecoveryFromISR(void)
{
    s_rx_need_recovery = true;
}

static void BSP_UART_FinalizeFrameFromISR(void)
{
    HAL_StatusTypeDef restart_status = HAL_ERROR;
    uint8_t *sealed_buffer = NULL;
    uint8_t *new_active_buffer = NULL;

    if (rx_count == 0U) {
        return;
    }

    /* Only one ready slot exists. If the previous frame is still pending when
     * another IDLE arrives, drop the current capture and recover the RX chain
     * rather than mixing completed frames. */
    if (frame_ready) {
        s_uart_rx_overflow_count++;
        BSP_UART_RequestRecoveryFromISR();
        return;
    }

    sealed_buffer = s_active_rx_buffer;
    new_active_buffer = s_ready_rx_buffer;
    s_active_rx_buffer = new_active_buffer;
    s_ready_rx_buffer = sealed_buffer;
    s_ready_frame_len = rx_count;
    rx_count = 0U;
    frame_ready = true;

    if (!BSP_UART_RestartReceiveFromBufferHead(&restart_status)) {
        s_rx_need_recovery = true;
        if ((restart_status != HAL_BUSY) &&
            ((HAL_GetTick() - s_last_post_frame_restart_log_tick) >= 1000U)) {
            s_last_post_frame_restart_log_tick = HAL_GetTick();
            ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
        }
    }
}

static bool BSP_UART_ShouldThrottleLog(void)
{
    uint32_t now = HAL_GetTick();
    if ((now - s_last_error_log_tick) >= UART_ERROR_LOG_THROTTLE_MS) {
        s_last_error_log_tick = now;
        return false;
    }
    return true;
}

static void BSP_UART_ClassifyAndHandleErrorsFromISR(UART_HandleTypeDef *huart)
{
    uint32_t error_code = 0U;
    bool has_error = false;
    bool need_immediate_recover = false;

    if (huart == NULL) {
        return;
    }

    error_code = huart->ErrorCode;
    if (error_code == HAL_UART_ERROR_NONE) {
        s_uart_error_streak = 0U;
        return;
    }

    if ((error_code & HAL_UART_ERROR_ORE) != 0U) {
        s_uart_ore_count++;
        has_error = true;
        need_immediate_recover = true;
    }
    if ((error_code & HAL_UART_ERROR_FE) != 0U) {
        s_uart_fe_count++;
        has_error = true;
    }
    if ((error_code & HAL_UART_ERROR_NE) != 0U) {
        s_uart_ne_count++;
        has_error = true;
    }
    if ((error_code & HAL_UART_ERROR_PE) != 0U) {
        s_uart_pe_count++;
        has_error = true;
    }

    if (!has_error) {
        s_uart_error_streak = 0U;
        return;
    }

    s_uart_error_count++;
    s_uart_error_streak++;

    /* Clear PE/FE/NE/ORE chain on F1 series before re-arming RX. */
    __HAL_UART_CLEAR_PEFLAG(huart);

    if (need_immediate_recover || (s_uart_error_streak >= UART_ERROR_STREAK_RECOVER_TH)) {
        BSP_UART_RequestRecoveryFromISR();
    }

    if (!BSP_UART_ShouldThrottleLog()) {
        ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
    }
}

static void BSP_UART_RecoveryIfNeeded(void)
{
    HAL_StatusTypeDef restart_status = HAL_ERROR;

    if (!s_rx_need_recovery) {
        return;
    }

    /* Keep the sealed frame available to upper layers first. If recovery was
     * requested after that frame was published, defer the reset until the task
     * has drained the ready buffer. */
    if (frame_ready) {
        return;
    }

    /* Avoid hot-loop retries in task polling path. */
    if ((HAL_GetTick() - s_last_recover_try_tick) < 5U) {
        return;
    }
    s_last_recover_try_tick = HAL_GetTick();

    s_rx_need_recovery = false;

    if (!BSP_UART_ResetStateAndRestartReceive(&restart_status)) {
        /* HAL_BUSY is expected transiently when RX state has not fully released. */
        s_rx_need_recovery = true;
        if (restart_status != HAL_BUSY) {
            s_uart_error_count++;
            if ((HAL_GetTick() - s_last_recover_fail_log_tick) >= 1000U) {
                s_last_recover_fail_log_tick = HAL_GetTick();
                ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
            }
        }
    }
}

void BSP_UART_Init(void)
{
    // 1. 使能时钟
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
	  
    // 2. 配置 GPIO (分开配置 TX 和 RX，确保 RX 强上拉)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // --- 配置 TX (PA9) ---
    GPIO_InitStruct.Pin = BSP_UART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_UART_GPIO_PORT, &GPIO_InitStruct);

    // --- 配置 RX (PA10) ---
    GPIO_InitStruct.Pin = BSP_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // 显式设为输入
    GPIO_InitStruct.Pull = GPIO_PULLUP;       // 强上拉，对抗干扰
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_UART_GPIO_PORT, &GPIO_InitStruct);
    
    // 3. 配置 UART
    huart1.Instance = BSP_UART_INSTANCE;
    huart1.Init.BaudRate = BSP_UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 新增：开启 USART1 NVIC */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    // 4. 开启空闲中断 (IDLE Interrupt) - 关键步骤
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    
    // 5. 启动首次接收（触发后续的中断链）
    rx_count = 0U;
    s_ready_frame_len = 0U;
    frame_ready = false;
    s_rx_need_recovery = false;
    if (!BSP_UART_StartReceiveByte())
    {
        s_uart_error_count++;
        ErrorLogRecord(ERROR_UART, __FILE__, __LINE__);
        Error_Handler();
    }

}

bool BSP_UART_Send(const uint8_t *data, uint16_t len, uint32_t timeout)
{
    bool status = false;
    if ((data != NULL) && (len > 0U)) {
        /* Auto-direction RS485 modules may loop back TX bytes into RX path.
         * Mark TX window to drop self-echo bytes in RX callback. */
        s_uart_tx_in_progress = 1U;
        status = (HAL_UART_Transmit(&huart1, (uint8_t*)data, len, timeout) == HAL_OK);
        s_uart_tx_in_progress = 0U;
    }
    return status;
}



void BSP_UART_PrintString(const char *str)
{
    if (str != NULL) {
        (void)BSP_UART_Send((const uint8_t*)str, (uint16_t)strlen(str), BSP_UART_TX_TIMEOUT);
    }
}

// 串口接收中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) 
    {
        /* Drop local TX echo bytes to avoid "request/echo" alternating frames. */
        if (s_uart_tx_in_progress != 0U) {
            if (HAL_UART_Receive_IT(huart, &s_active_rx_buffer[rx_count], 1U) != HAL_OK) {
                BSP_UART_RequestRecoveryFromISR();
            }
            return;
        }

        if (rx_count < BSP_UART_RX_BUF_SIZE)
        {
            rx_count++;
            if (rx_count < BSP_UART_RX_BUF_SIZE)
            {
                if (HAL_UART_Receive_IT(huart, &s_active_rx_buffer[rx_count], 1U) != HAL_OK)
                {
                    if (huart->RxState == HAL_UART_STATE_BUSY_RX) {
                        BSP_UART_RequestRecoveryFromISR();
                    } else {
                        s_uart_error_count++;
                        BSP_UART_RequestRecoveryFromISR();
                    }
                }
            }
            else
            {
                s_uart_rx_overflow_count++;
                BSP_UART_RequestRecoveryFromISR();
            }
        }
        else
        {
            s_uart_rx_overflow_count++;
            BSP_UART_RequestRecoveryFromISR();
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL || huart->Instance != USART1) {
        return;
    }

    BSP_UART_ClassifyAndHandleErrorsFromISR(huart);
}
// 串口空闲中断处理（在 stm32f1xx_it.c 中调用，或者在这里处理）
// 注意：HAL 库的空闲中断通常需要用户在 IT_Handler 中清除标志位并调用此逻辑
void BSP_UART_IRQHandler(void)
{
    /* Guard the whole USART1 path, not just the HAL call, so IDLE sealing and
     * HAL RX/error handling always run as one consistent critical section. */
    __disable_irq();  // 关中断确保原子性
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

    HAL_UART_IRQHandler(&huart1);

    /* Let HAL drain the last RXNE byte first, then seal the frame on IDLE and
     * immediately re-arm reception on the other buffer to shrink the gap
     * between two back-to-back master requests. */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        BSP_UART_FinalizeFrameFromISR();
    }

    s_uart_irq_busy = 0U;
}


bool BSP_UART_IsFrameReady(void)
{
    BSP_UART_RecoveryIfNeeded();
    return frame_ready;
}

uint16_t BSP_UART_ReadFrame(uint8_t *buffer, uint16_t max_len)
{
    uint16_t snapshot_len = 0U;
    uint16_t copy_len = 0U;

    BSP_UART_RecoveryIfNeeded();

    if (buffer == NULL || max_len == 0U) {
        return 0U;
    }

    __disable_irq();
    if (frame_ready) {
        snapshot_len = s_ready_frame_len;
        s_ready_frame_len = 0U;
        frame_ready = false;
    }
    __enable_irq();

    if (snapshot_len > 0U) {
        copy_len = (snapshot_len < max_len) ? snapshot_len : max_len;
        memcpy(buffer, s_ready_rx_buffer, copy_len);
    }

    return copy_len;
}

