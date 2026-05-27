// Bsp/Src/i2c.c
#include "i2c.h"
#include "error_handler.h"

static I2C_HandleTypeDef hi2c1; // I2C句柄，仅在BSP层内部使用

void BSP_I2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    // 配置GPIO：PB6/PB7为复用开漏输出（I2C标准）
    GPIO_InitStruct.Pin = BSP_I2C_SCL_PIN | BSP_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;       // 复用开漏输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // 上拉（配合外部4.7kΩ电阻）
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_I2C_GPIO_PORT, &GPIO_InitStruct);

    // 配置I2C参数
    hi2c1.Instance = BSP_I2C_INSTANCE;
    hi2c1.Init.ClockSpeed = BSP_I2C_SPEED;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    // 开启NVIC中断（优先级5，与UART保持一致）
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

bool BSP_I2C_Master_Transmit(uint8_t dev_addr, const uint8_t* data, uint16_t len) {
    if ((data == NULL) || (len == 0U)) {
        return false;
    }

    // 从机地址左移1位（留出R/W位）
    uint16_t i2c_addr = (uint16_t)(dev_addr << 1U);

    return (HAL_I2C_Master_Transmit(&hi2c1, i2c_addr, (uint8_t*)data, len, BSP_I2C_TIMEOUT_MS) ==
            HAL_OK);
}

bool BSP_I2C_Master_Receive(uint8_t dev_addr, uint8_t* data, uint16_t len) {
    if ((data == NULL) || (len == 0U)) {
        return false;
    }

    uint16_t i2c_addr = (uint16_t)(dev_addr << 1U);

    return (HAL_I2C_Master_Receive(&hi2c1, i2c_addr, data, len, BSP_I2C_TIMEOUT_MS) == HAL_OK);
}

bool BSP_I2C_Mem_Read(uint8_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size, uint8_t* data,
                      uint16_t len) {
    if ((data == NULL) || (len == 0U)) {
        return false;
    }

    uint16_t i2c_addr = (uint16_t)(dev_addr << 1U);

    return (HAL_I2C_Mem_Read(&hi2c1, i2c_addr, mem_addr, mem_addr_size, data, len,
                             BSP_I2C_TIMEOUT_MS) == HAL_OK);
}

bool BSP_I2C_Mem_Write(uint8_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size,
                       const uint8_t* data, uint16_t len) {
    if ((data == NULL) || (len == 0U)) {
        return false;
    }

    uint16_t i2c_addr = (uint16_t)(dev_addr << 1U);

    return (HAL_I2C_Mem_Write(&hi2c1, i2c_addr, mem_addr, mem_addr_size, (uint8_t*)data, len,
                              BSP_I2C_TIMEOUT_MS) == HAL_OK);
}
