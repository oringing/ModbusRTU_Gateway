// Core/Src/main.c
#include "main.h"
#include "cmsis_os.h"
#include "task.h"

/* USER CODE BEGIN Includes */
#include "error_handler.h"
#include "led.h"
#include "modbus.h"
#include "servo.h"
#include "stdio.h"
#include "string.h"
#include "system_config.h"
#include "system_ctrl.h"
#include "uart.h"
#include "soft_i2c.h"
#include "aht20.h"
#include "bmp280.h"

/* USER CODE END Includes */

void SystemClock_Config(void);

// 系统启动入口：硬件初始化→模块初始化→任务创建→启动调度器
int main(void) {
    // 1. HAL库初始化（时钟、NVIC、DMA基础配置）
    HAL_Init();
    
    // 2. 系统时钟配置（HSE+PLL，72MHz主频）
    SystemClock_Config();

    /* USER CODE BEGIN Init */
    // 3. BSP层硬件初始化（LED/UART/舵机PWM）
    BSP_LED_Init();
    BSP_UART_Init();
    BSP_Servo_Init();
    I2C_Bus_Init();
    AHT20_Init();
    BMP280_Init();

    // 4. 协议层初始化（Modbus寄存器+回调）
    Modbus_Init();
    
    // 5. 系统控制层初始化（配置校验+看门狗）
    System_Ctrl_Init();

    // 6. 创建所有应用任务（LED/UART/Monitor）
    if (System_StartTasks() != SYSTEM_OK) {
        ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__);
        Error_Handler(); // 任务创建失败，进入安全模式
    }

    // 7. 启动FreeRTOS调度器（永不返回）
    osKernelStart();
    /* USER CODE END Init */
}

/**
 * @brief   系统时钟配置（HSE 8MHz → PLL 9倍频 → 72MHz系统时钟）
 * @note    时钟树配置：SYSCLK=72MHz, HCLK=72MHz, PCLK1=36MHz, PCLK2=72MHz
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // 配置振荡器：HSE 8MHz + PLL 9倍频
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // 配置时钟分频：AHB=72MHz, APB1=36MHz, APB2=72MHz
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN EFP */
// ✅ 新增：传感器系统初始化函数声明（后续实现）
void Sensor_System_Init(void);

/* USER CODE END EFP */

/* USER CODE BEGIN 4 */
// ✅ 新增：传感器系统初始化函数实现（后续补充具体代码）
void Sensor_System_Init(void) {
    // TODO: 后续在此处添加 AHT20/BMP280 初始化代码
    // if (AHT20_Init() != HAL_OK) {
    //     Error_Handler();
    // }
    // if (BMP280_Init() != HAL_OK) {
    //     Error_Handler();
    // }
}
/* USER CODE END 4 */
