/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_conf.h
  * @brief   HAL configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* ########################## Module Selection ############################## */
/**
  * @brief This is the list of modules to be used in the HAL driver
  */

#define HAL_MODULE_ENABLED
  /*#define HAL_ADC_MODULE_ENABLED   */
/*#define HAL_CRYP_MODULE_ENABLED   */
/*#define HAL_CAN_MODULE_ENABLED   */
/*#define HAL_CAN_LEGACY_MODULE_ENABLED   */
/*#define HAL_CEC_MODULE_ENABLED   */
/*#define HAL_CORTEX_MODULE_ENABLED   */
/*#define HAL_CRC_MODULE_ENABLED   */
/*#define HAL_DAC_MODULE_ENABLED   */
/*#define HAL_DMA_MODULE_ENABLED   */
/*#define HAL_ETH_MODULE_ENABLED   */
/*#define HAL_FLASH_MODULE_ENABLED   */
#define HAL_GPIO_MODULE_ENABLED
/*#define HAL_I2C_MODULE_ENABLED   */
/*#define HAL_I2S_MODULE_ENABLED   */
/*#define HAL_IRDA_MODULE_ENABLED   */
#define HAL_IWDG_MODULE_ENABLED   
/*#define HAL_NOR_MODULE_ENABLED   */
/*#define HAL_NAND_MODULE_ENABLED   */
/*#define HAL_PCCARD_MODULE_ENABLED   */
/*#define HAL_PCD_MODULE_ENABLED   */
/*#define HAL_HCD_MODULE_ENABLED   */
/*#define HAL_PWR_MODULE_ENABLED   */
/*#define HAL_RCC_MODULE_ENABLED   */
/*#define HAL_RTC_MODULE_ENABLED   */
/*#define HAL_SD_MODULE_ENABLED   */
/*#define HAL_MMC_MODULE_ENABLED   */
/*#define HAL_SDRAM_MODULE_ENABLED   */
/*#define HAL_SMARTCARD_MODULE_ENABLED   */
/*#define HAL_SPI_MODULE_ENABLED   */
/*#define HAL_SRAM_MODULE_ENABLED   */
/*#define HAL_TIM_MODULE_ENABLED   */
#define HAL_UART_MODULE_ENABLED
/*#define HAL_USART_MODULE_ENABLED   */
/*#define HAL_WWDG_MODULE_ENABLED   */

#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED

/* ... existing code ... */