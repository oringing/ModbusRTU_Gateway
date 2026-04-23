// Bsp/Src/uart.c
#include "uart.h"
#include "modbus.h"
#include <string.h>
#include "cmsis_os.h"
#include "task.h"   
#include "FreeRTOS.h"

// 内部私有变量
static UART_HandleTypeDef huart1; 
static uint8_t rx_buffer[BSP_UART_RX_BUF_SIZE];
static volatile uint16_t rx_count = 0;
static volatile bool frame_ready = false;
static osMutexId uart_mutex = NULL; // FreeRTOS 互斥量句柄

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
    HAL_UART_Receive_IT(&huart1, &rx_buffer[0], 1);

}

// 延迟初始化 Mutex，确保在 RTOS 运行后调用
static void UART_EnsureMutex(void) {
    //osMutexDef(UartMutex);
    //uart_mutex = osMutexCreate(osMutex(UartMutex));

    if (uart_mutex == NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        osMutexDef(UartMutex);
        uart_mutex = osMutexCreate(osMutex(UartMutex));
    }
}

bool BSP_UART_Send(const uint8_t *data, uint16_t len, uint32_t timeout)
{
    UART_EnsureMutex();
    
    // 如果 mutex 仍然为 NULL，尝试创建一次
    if (uart_mutex == NULL) {
        osMutexDef(UartMutex);
        uart_mutex = osMutexCreate(osMutex(UartMutex));
        if (uart_mutex == NULL) {
            return false; // 创建失败，返回错误
        }
    }

    bool status = false;
    if (osMutexWait(uart_mutex, timeout) == osOK) {
        status = (HAL_UART_Transmit(&huart1, (uint8_t*)data, len, timeout) == HAL_OK);
        osMutexRelease(uart_mutex);
    }
    return status;
}


void BSP_UART_PrintString(const char *str)
{
    BSP_UART_Send((const uint8_t*)str, strlen(str), BSP_UART_TX_TIMEOUT);
}

// 串口接收中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) 
    {        
        if (rx_count < BSP_UART_RX_BUF_SIZE)
        {
            rx_count++;
            if (rx_count < BSP_UART_RX_BUF_SIZE)
            {
                HAL_UART_Receive_IT(huart, &rx_buffer[rx_count], 1);
            }
            else
            {
                frame_ready = true;
            }
        }
    }
}
// 串口空闲中断处理（在 stm32f1xx_it.c 中调用，或者在这里处理）
// 注意：HAL 库的空闲中断通常需要用户在 IT_Handler 中清除标志位并调用此逻辑
void BSP_UART_IRQHandler(void)
{
    
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
       
        if (rx_count > 0)
        {
            frame_ready = true;
        }
    }

    HAL_UART_IRQHandler(&huart1);
}


bool BSP_UART_IsFrameReady(void)
{
    return frame_ready;
}

uint16_t BSP_UART_ReadFrame(uint8_t *buffer, uint16_t max_len)
{
    if (!frame_ready) return 0;
    
    uint16_t len = (rx_count < max_len) ? rx_count : max_len;
    memcpy(buffer, rx_buffer, len);
    
    // 重置并重启接收链
    rx_count = 0;
    frame_ready = false;
    HAL_UART_Receive_IT(&huart1, &rx_buffer[0], 1);
    
    return len;
}

