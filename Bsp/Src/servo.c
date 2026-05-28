// Bsp/Src/servo.c
#include "servo.h"
#include "error_handler.h"
#include "stm32f1xx_hal_tim.h"

static TIM_HandleTypeDef htim_servo;

// 初始化舵机 PWM (TIM3_CH1/PA6, TIM3_CH2/PA7)
void BSP_Servo_Init(void) {
    GPIO_InitTypeDef   GPIO_InitStruct = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    // 使能 TIM3 和 GPIOA 时钟
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 配置 PA6/PA7 为复用推挽输出
    GPIO_InitStruct.Pin = SERVO_CH1_GPIO_PIN | SERVO_CH2_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 配置 TIM3 参数
    htim_servo.Instance = SERVO_TIM_INSTANCE;
    htim_servo.Init.Prescaler = SERVO_TIM_PRESCALER;
    htim_servo.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_servo.Init.Period = SERVO_TIM_PERIOD;
    htim_servo.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_PWM_Init(&htim_servo) != HAL_OK) {
        Error_Handler();
    }

    // 配置 PWM 通道参数
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0; // 初始占空比为 0
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    // 配置通道 1 (PA6)
    if (HAL_TIM_PWM_ConfigChannel(&htim_servo, &sConfigOC, SERVO_CH1_CHANNEL) != HAL_OK) {
        Error_Handler();
    }

    // 配置通道 2 (PA7)
    if (HAL_TIM_PWM_ConfigChannel(&htim_servo, &sConfigOC, SERVO_CH2_CHANNEL) != HAL_OK) {
        Error_Handler();
    }

    // 启动 PWM 输出
    HAL_TIM_PWM_Start(&htim_servo, SERVO_CH1_CHANNEL);
    HAL_TIM_PWM_Start(&htim_servo, SERVO_CH2_CHANNEL);
}

// 设置指定通道舵机的角度 (0-180度)，使用线性映射算法
void BSP_Servo_SetAngle(uint8_t channel, uint16_t angle) {
    uint16_t pulse;
    uint32_t tim_channel;

    // 边界检查：确保角度在 0-180 之间
    if (angle > SERVO_ANGLE_MAX) {
        angle = SERVO_ANGLE_MAX;
    }

    // 线性映射：pulse = MIN + (angle * RANGE / MAX_ANGLE)
    pulse =
        SERVO_PULSE_MIN_US + (uint16_t)(((uint32_t)angle * SERVO_PULSE_RANGE_US) / SERVO_ANGLE_MAX);

    // 根据通道选择对应的定时器通道常量
    if (channel == 1U) {
        tim_channel = SERVO_CH1_CHANNEL;
    } else if (channel == 2U) {
        tim_channel = SERVO_CH2_CHANNEL;
    } else {
        return; // 无效通道
    }

    // 更新比较寄存器
    __HAL_TIM_SET_COMPARE(&htim_servo, tim_channel, pulse);
}

// 直接设置指定通道的脉冲宽度 (单位：微秒)
void BSP_Servo_SetPulseWidth(uint8_t channel, uint16_t pulse_us) {
    uint32_t tim_channel;

    // 边界检查：确保脉宽在安全范围内
    if (pulse_us < SERVO_PULSE_MIN_US)
        pulse_us = SERVO_PULSE_MIN_US;
    if (pulse_us > SERVO_PULSE_MAX_US)
        pulse_us = SERVO_PULSE_MAX_US;

    // 根据通道选择对应的定时器通道常量
    if (channel == 1U) {
        tim_channel = SERVO_CH1_CHANNEL;
    } else if (channel == 2U) {
        tim_channel = SERVO_CH2_CHANNEL;
    } else {
        return;
    }

    // 计数器频率为 1MHz，所以 1us = 1 个计数值
    __HAL_TIM_SET_COMPARE(&htim_servo, tim_channel, (uint32_t)pulse_us);
}
