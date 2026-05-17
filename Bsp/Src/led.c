// Bsp/Src/led.c
#include "led.h"

void BSP_LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = LED_C13_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_C13_GPIO_Port, &GPIO_InitStruct);

    // 初始状态：熄灭LED（PC13低电平点亮）
    HAL_GPIO_WritePin(LED_C13_GPIO_Port, LED_C13_Pin, GPIO_PIN_SET);
}

void BSP_LED_Toggle(void) {
    HAL_GPIO_TogglePin(LED_C13_GPIO_Port, LED_C13_Pin);
}

void BSP_LED_On(void) {
    HAL_GPIO_WritePin(LED_C13_GPIO_Port, LED_C13_Pin, GPIO_PIN_RESET);
}

void BSP_LED_Off(void) {
    HAL_GPIO_WritePin(LED_C13_GPIO_Port, LED_C13_Pin, GPIO_PIN_SET);
}
