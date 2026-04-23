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
	  
	// 1. 创建互斥量（在调度器启动后调用 Init 才能成功，或者在 main 中先创建）
    osMutexDef(UartMutex);
    uart_mutex = osMutexCreate(osMutex(UartMutex));

    // 2. 配置 GPIO (PA9 TX, PA10 RX)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BSP_UART_TX_PIN | BSP_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
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

    // 4. 开启空闲中断 (IDLE Interrupt) - 关键步骤
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    
    // 5. 启动首次接收（触发后续的中断链）
    HAL_UART_Receive_IT(&huart1, &rx_buffer[0], 1);

    // 【调试用】强制发送一个字符 'A' (0x41)
    // 如果这里能看到波形，说明硬件和时钟没问题，是上层逻辑没调用 Send
    HAL_UART_Transmit(&huart1, (uint8_t*)"A", 1, 100);
}

// 延迟初始化 Mutex，确保在 RTOS 运行后调用
static void UART_EnsureMutex(void) {
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
    if (huart->Instance == USART1) {
        if (rx_count < BSP_UART_RX_BUF_SIZE - 1) { // 留一个字节余量
            rx_count++;
            HAL_UART_Receive_IT(huart, &rx_buffer[rx_count], 1);
        }
        // 如果满了，停止接收，等待帧处理完成后在 ReadFrame 中重启
    }
}

// 串口空闲中断处理（在 stm32f1xx_it.c 中调用，或者在这里处理）
// 注意：HAL 库的空闲中断通常需要用户在 IT_Handler 中清除标志位并调用此逻辑
void BSP_UART_IRQHandler(void)
{
    // 1. 处理空闲中断 (Frame End Detection)
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1); // 清除标志位
        // 此时 rx_count 已经记录了接收到的字节数
        if (rx_count > 0)
        {
            frame_ready = true;
        }
    }

    // 2. 调用 HAL 库的标准中断处理（处理 RXNE, TC 等常规中断）
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
