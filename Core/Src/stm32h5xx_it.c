/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32h5xx_it.c
 * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_it.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "buttons.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern DMA_HandleTypeDef handle_GPDMA1_Channel2;
extern DMA_HandleTypeDef handle_GPDMA1_Channel1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel0;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1) {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1) {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1) {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void) {
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1) {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1) {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h5xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles EXTI Line0 interrupt.
 */
void EXTI0_IRQHandler(void) {
  /* USER CODE BEGIN EXTI0_IRQn 0 */
  selectButtonCallback();
  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(SELECT_Pin);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line1 interrupt.
 */
void EXTI1_IRQHandler(void) {
  /* USER CODE BEGIN EXTI1_IRQn 0 */
  backButtonCallback();
  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(BACK_Pin);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line2 interrupt.
 */
void EXTI2_IRQHandler(void) {
  /* USER CODE BEGIN EXTI2_IRQn 0 */
  ENCACallback();
  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(ENCA_Pin);
  /* USER CODE BEGIN EXTI2_IRQn 1 */

  /* USER CODE END EXTI2_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line11 interrupt.
 */
void EXTI11_IRQHandler(void) {
  /* USER CODE BEGIN EXTI11_IRQn 0 */
  for (uint32_t i = 0; i < 1000; i++) {
    for (uint32_t j = 0; j < 10; j++) {
      __NOP();
    }
  }
  if (HAL_GPIO_ReadPin(CHANNEL4_GPIO_Port, CHANNEL4_Pin) == GPIO_PIN_SET) {
    GPIOpillDetected[3] = 1;
  }
  /* USER CODE END EXTI11_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(CHANNEL4_Pin);
  /* USER CODE BEGIN EXTI11_IRQn 1 */

  /* USER CODE END EXTI11_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line12 interrupt.
 */
void EXTI12_IRQHandler(void) {
  /* USER CODE BEGIN EXTI12_IRQn 0 */

  /* USER CODE END EXTI12_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(SPI_RDY_Pin);
  /* USER CODE BEGIN EXTI12_IRQn 1 */

  /* USER CODE END EXTI12_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line13 interrupt.
 */
void EXTI13_IRQHandler(void) {
  /* USER CODE BEGIN EXTI13_IRQn 0 */
  for (uint32_t i = 0; i < 1000; i++) {
    for (uint32_t j = 0; j < 10; j++) {
      __NOP();
    }
  }
  if (HAL_GPIO_ReadPin(CHANNEL3_GPIO_Port, CHANNEL3_Pin) == GPIO_PIN_SET) {
    GPIOpillDetected[2] = 1;
  }
  /* USER CODE END EXTI13_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(CHANNEL3_Pin);
  /* USER CODE BEGIN EXTI13_IRQn 1 */

  /* USER CODE END EXTI13_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line14 interrupt.
 */
void EXTI14_IRQHandler(void) {
  /* USER CODE BEGIN EXTI14_IRQn 0 */
  for (uint32_t i = 0; i < 1000; i++) {
    for (uint32_t j = 0; j < 20; j++) {
      __NOP();
    }
  }
  if (HAL_GPIO_ReadPin(CHANNEL2_GPIO_Port, CHANNEL2_Pin) == GPIO_PIN_SET) {
    GPIOpillDetected[2 - 1] = 1;
  }
  /* USER CODE END EXTI14_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(CHANNEL2_Pin);
  /* USER CODE BEGIN EXTI14_IRQn 1 */

  /* USER CODE END EXTI14_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line15 interrupt.
 */
void EXTI15_IRQHandler(void) {
  /* USER CODE BEGIN EXTI15_IRQn 0 */
  for (uint32_t i = 0; i < 1000; i++) {
    for (uint32_t j = 0; j < 10; j++) {
      __NOP();
    }
  }
  if (HAL_GPIO_ReadPin(CHANNEL1_GPIO_Port, CHANNEL1_Pin) == GPIO_PIN_SET) {
    GPIOpillDetected[1 - 1] = 1;
  }
  /* USER CODE END EXTI15_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(CHANNEL1_Pin);
  /* USER CODE BEGIN EXTI15_IRQn 1 */

  /* USER CODE END EXTI15_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 0 global interrupt.
 */
void GPDMA1_Channel0_IRQHandler(void) {
  /* USER CODE BEGIN GPDMA1_Channel0_IRQn 0 */

  /* USER CODE END GPDMA1_Channel0_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel0);
  /* USER CODE BEGIN GPDMA1_Channel0_IRQn 1 */

  /* USER CODE END GPDMA1_Channel0_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 1 global interrupt.
 */
void GPDMA1_Channel1_IRQHandler(void) {
  /* USER CODE BEGIN GPDMA1_Channel1_IRQn 0 */

  /* USER CODE END GPDMA1_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel1);
  /* USER CODE BEGIN GPDMA1_Channel1_IRQn 1 */

  /* USER CODE END GPDMA1_Channel1_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 2 global interrupt.
 */
void GPDMA1_Channel2_IRQHandler(void) {
  /* USER CODE BEGIN GPDMA1_Channel2_IRQn 0 */

  /* USER CODE END GPDMA1_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel2);
  /* USER CODE BEGIN GPDMA1_Channel2_IRQn 1 */

  /* USER CODE END GPDMA1_Channel2_IRQn 1 */
}

/**
 * @brief This function handles ADC1 global interrupt.
 */
void ADC1_IRQHandler(void) {
  /* USER CODE BEGIN ADC1_IRQn 0 */

  /* USER CODE END ADC1_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC1_IRQn 1 */

  /* USER CODE END ADC1_IRQn 1 */
}

/**
 * @brief This function handles TIM1 Update interrupt.
 */
void TIM1_UP_IRQHandler(void) {
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */

  /* USER CODE END TIM1_UP_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_IRQn 1 */

  /* USER CODE END TIM1_UP_IRQn 1 */
}

/**
 * @brief This function handles SPI2 global interrupt.
 */
void SPI2_IRQHandler(void) {
  /* USER CODE BEGIN SPI2_IRQn 0 */

  /* USER CODE END SPI2_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi2);
  /* USER CODE BEGIN SPI2_IRQn 1 */

  /* USER CODE END SPI2_IRQn 1 */
}

/**
 * @brief This function handles SPI3 global interrupt.
 */
void SPI3_IRQHandler(void) {
  /* USER CODE BEGIN SPI3_IRQn 0 */

  /* USER CODE END SPI3_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi3);
  /* USER CODE BEGIN SPI3_IRQn 1 */

  /* USER CODE END SPI3_IRQn 1 */
}

/**
 * @brief This function handles USART3 global interrupt.
 */
void USART3_IRQHandler(void) {
  /* USER CODE BEGIN USART3_IRQn 0 */

  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

  /* USER CODE END USART3_IRQn 1 */
}

/**
 * @brief This function handles ADC2 global interrupt.
 */
void ADC2_IRQHandler(void) {
  /* USER CODE BEGIN ADC2_IRQn 0 */

  /* USER CODE END ADC2_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc2);
  /* USER CODE BEGIN ADC2_IRQn 1 */

  /* USER CODE END ADC2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
