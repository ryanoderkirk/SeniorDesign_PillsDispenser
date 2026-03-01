/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define WIFI2_SPI4_SCK_Pin GPIO_PIN_2
#define WIFI2_SPI4_SCK_GPIO_Port GPIOE
#define WIFI2_SPI_CS_Pin GPIO_PIN_4
#define WIFI2_SPI_CS_GPIO_Port GPIOE
#define WIFI2_SPI4_MISO_Pin GPIO_PIN_5
#define WIFI2_SPI4_MISO_GPIO_Port GPIOE
#define WIFI2_SPI4_MOSI_Pin GPIO_PIN_6
#define WIFI2_SPI4_MOSI_GPIO_Port GPIOE
#define WIFI2_SPI_RDY_Pin GPIO_PIN_13
#define WIFI2_SPI_RDY_GPIO_Port GPIOC
#define WIFI2_CHIP_EN_Pin GPIO_PIN_14
#define WIFI2_CHIP_EN_GPIO_Port GPIOC
#define WIFI2_BOOT_Pin GPIO_PIN_15
#define WIFI2_BOOT_GPIO_Port GPIOC
#define SELECT_Pin GPIO_PIN_0
#define SELECT_GPIO_Port GPIOF
#define BACK_Pin GPIO_PIN_1
#define BACK_GPIO_Port GPIOF
#define ENCA_Pin GPIO_PIN_2
#define ENCA_GPIO_Port GPIOF
#define ENCB_Pin GPIO_PIN_3
#define ENCB_GPIO_Port GPIOF
#define WIFI2_UART_RX_Pin GPIO_PIN_6
#define WIFI2_UART_RX_GPIO_Port GPIOF
#define WIFI2_UART_TX_Pin GPIO_PIN_7
#define WIFI2_UART_TX_GPIO_Port GPIOF
#define LCD_RST_Pin GPIO_PIN_1
#define LCD_RST_GPIO_Port GPIOH
#define LCD_CS_Pin GPIO_PIN_0
#define LCD_CS_GPIO_Port GPIOC
#define LCD_MOSI_Pin GPIO_PIN_1
#define LCD_MOSI_GPIO_Port GPIOC
#define LCD_MISO_Pin GPIO_PIN_2
#define LCD_MISO_GPIO_Port GPIOC
#define LCD_DC_Pin GPIO_PIN_3
#define LCD_DC_GPIO_Port GPIOC
#define CHIP_EN_Pin GPIO_PIN_0
#define CHIP_EN_GPIO_Port GPIOB
#define WIFI_MOSI_Pin GPIO_PIN_2
#define WIFI_MOSI_GPIO_Port GPIOB
#define POWER_INT_Pin GPIO_PIN_11
#define POWER_INT_GPIO_Port GPIOE
#define LCD_SCK_Pin GPIO_PIN_10
#define LCD_SCK_GPIO_Port GPIOB
#define FINGERPRINT_UART_RX_Pin GPIO_PIN_12
#define FINGERPRINT_UART_RX_GPIO_Port GPIOB
#define FINGERPRINT_UART_TX_Pin GPIO_PIN_13
#define FINGERPRINT_UART_TX_GPIO_Port GPIOB
#define DEBUG_USART_TX_Pin GPIO_PIN_8
#define DEBUG_USART_TX_GPIO_Port GPIOD
#define DEBUG_USART_RX_Pin GPIO_PIN_9
#define DEBUG_USART_RX_GPIO_Port GPIOD
#define WIFI_UART_RX_Pin GPIO_PIN_11
#define WIFI_UART_RX_GPIO_Port GPIOD
#define SPI_CS3_Pin GPIO_PIN_9
#define SPI_CS3_GPIO_Port GPIOA
#define BOOT3_Pin GPIO_PIN_10
#define BOOT3_GPIO_Port GPIOA
#define CHIP_EN3_Pin GPIO_PIN_11
#define CHIP_EN3_GPIO_Port GPIOA
#define SPI_RDY3_Pin GPIO_PIN_12
#define SPI_RDY3_GPIO_Port GPIOA
#define WIFI_SCK_Pin GPIO_PIN_10
#define WIFI_SCK_GPIO_Port GPIOC
#define WIFI_MISO_Pin GPIO_PIN_11
#define WIFI_MISO_GPIO_Port GPIOC
#define WIFI_UART_TX_Pin GPIO_PIN_1
#define WIFI_UART_TX_GPIO_Port GPIOD
#define BOOT_Pin GPIO_PIN_3
#define BOOT_GPIO_Port GPIOD
#define SPI_CS_Pin GPIO_PIN_4
#define SPI_CS_GPIO_Port GPIOD
#define Flash_MOSI_Pin GPIO_PIN_7
#define Flash_MOSI_GPIO_Port GPIOD
#define Flash_MISO_Pin GPIO_PIN_9
#define Flash_MISO_GPIO_Port GPIOG
#define Flash_SCK_Pin GPIO_PIN_11
#define Flash_SCK_GPIO_Port GPIOG
#define Flash_CS_Pin GPIO_PIN_12
#define Flash_CS_GPIO_Port GPIOG
#define SPI_RDY_Pin GPIO_PIN_7
#define SPI_RDY_GPIO_Port GPIOB
#define SPI_RDY_EXTI_IRQn EXTI7_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
