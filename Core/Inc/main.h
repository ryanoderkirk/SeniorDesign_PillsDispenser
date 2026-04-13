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
#include "cmsis_os2.h"
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

uint32_t isPillDetected();
uint32_t resetPillFlag();

extern uint32_t GPIOpillDetected[];
uint32_t GPIOisPillDetected(int channel);
uint32_t GPIOresetPillFlag(int channel);

// Dispenser handle functions
ADC_HandleTypeDef* Get_ADC_Handle(void);
TIM_HandleTypeDef* Get_PWM_Dispense_Handle(void);
TIM_HandleTypeDef* Get_PWM_Gate_Handle(void);
TIM_HandleTypeDef* Get_ADC_TIM_Handle(void);
UART_HandleTypeDef* Get_DEBUG_Handle(void);

osMutexId_t* getFlashMutex();
RTC_HandleTypeDef* getRTCHandle();
SPI_HandleTypeDef* getFlashSPIHandle();
void get_rtc_timestamp(char *buffer);
void update_system_time(int y, int m, int d, int hh, int mm, int ss);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SELECT_Pin GPIO_PIN_0
#define SELECT_GPIO_Port GPIOF
#define SELECT_EXTI_IRQn EXTI0_IRQn
#define BACK_Pin GPIO_PIN_1
#define BACK_GPIO_Port GPIOF
#define BACK_EXTI_IRQn EXTI1_IRQn
#define ENCA_Pin GPIO_PIN_2
#define ENCA_GPIO_Port GPIOF
#define ENCA_EXTI_IRQn EXTI2_IRQn
#define ENCB_Pin GPIO_PIN_3
#define ENCB_GPIO_Port GPIOF
#define LCD_RST_Pin GPIO_PIN_1
#define LCD_RST_GPIO_Port GPIOH
#define LCD_CS_Pin GPIO_PIN_0
#define LCD_CS_GPIO_Port GPIOC
#define LCD_DC_Pin GPIO_PIN_3
#define LCD_DC_GPIO_Port GPIOC
#define CHANNEL4_Pin GPIO_PIN_11
#define CHANNEL4_GPIO_Port GPIOE
#define CHANNEL4_EXTI_IRQn EXTI11_IRQn
#define CHANNEL3_Pin GPIO_PIN_13
#define CHANNEL3_GPIO_Port GPIOE
#define CHANNEL3_EXTI_IRQn EXTI13_IRQn
#define CHANNEL2_Pin GPIO_PIN_14
#define CHANNEL2_GPIO_Port GPIOE
#define CHANNEL2_EXTI_IRQn EXTI14_IRQn
#define CHANNEL1_Pin GPIO_PIN_15
#define CHANNEL1_GPIO_Port GPIOE
#define CHANNEL1_EXTI_IRQn EXTI15_IRQn
#define DEBUG_USART_TX_Pin GPIO_PIN_8
#define DEBUG_USART_TX_GPIO_Port GPIOD
#define DEBUG_USART_RX_Pin GPIO_PIN_9
#define DEBUG_USART_RX_GPIO_Port GPIOD
#define SPI_CS_Pin GPIO_PIN_9
#define SPI_CS_GPIO_Port GPIOA
#define BOOT_Pin GPIO_PIN_10
#define BOOT_GPIO_Port GPIOA
#define CHIP_EN_Pin GPIO_PIN_11
#define CHIP_EN_GPIO_Port GPIOA
#define SPI_RDY_Pin GPIO_PIN_12
#define SPI_RDY_GPIO_Port GPIOA
#define SPI_RDY_EXTI_IRQn EXTI12_IRQn
#define Flash_CS_Pin GPIO_PIN_12
#define Flash_CS_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
