// Core/Src/main.c
#include "main.h"
#include "cmsis_os.h"
#include "task.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "led.h"
#include "uart.h"
#include "servo.h"
#include "modbus.h"
#include "system_ctrl.h"
#include "error_handler.h"
#include "system_config.h"

/* USER CODE END Includes */

void SystemClock_Config(void);

int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN Init */
    BSP_LED_Init();
    BSP_UART_Init();
	BSP_Servo_Init();
    
    Modbus_Init();
    System_Ctrl_Init();
    
    /* Create all application tasks via system control module */
    if (System_StartTasks() != SYSTEM_OK) {
        ErrorLogRecord(ERROR_SYSTEM, __FILE__, __LINE__);
        Error_Handler();
    }

    /* Start scheduler */
    osKernelStart();
    /* USER CODE END Init */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
      Error_Handler();
    }
}
