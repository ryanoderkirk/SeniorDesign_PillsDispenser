/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : app_freertos.c
 * Description        : FreeRTOS applicative file
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
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "dispenseControl.h"
#include "filesystem.h"
#include "logging.h"
#include "UI.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityAboveNormal4,
  .stack_size = 1024 * 4
};
/* Definitions for UI_Task */
osThreadId_t UI_TaskHandle;
const osThreadAttr_t UI_Task_attributes = {
  .name = "UI_Task",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 1024 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of UI_Task */
  UI_TaskHandle = osThreadNew(UI_Task, NULL, &UI_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief Function implementing the defaultTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  /* Infinite loop */
  if (filesystemInit() != 0) {
     LogError("Filesystem init failed\n");
  }
  /*
      if (filesystemInit() != 0) {
      LogError("Filesystem init failed\n");
    }
    if (initDailyLog() != 0) {
      LogError("Could not write log file\n");
    }
    deleteAllLogs();
    */
  /* Infinite loop */

  run270Servo(*Get_PWM_Dispense_Handle(), TIM_CHANNEL_1, 90);
  run270Servo(*Get_PWM_Dispense_Handle(), TIM_CHANNEL_2, 90);
  run270Servo(*Get_PWM_Dispense_Handle(), TIM_CHANNEL_3, 90);
  run270Servo(*Get_PWM_Dispense_Handle(), TIM_CHANNEL_4, 90);
  run270Servo(*Get_PWM_Gate_Handle(), TIM_CHANNEL_1, 90);
  run270Servo(*Get_PWM_Gate_Handle(), TIM_CHANNEL_2, 90);
  // Configure ADC to listen to channel specified
  ADC_HandleTypeDef *hadc = Get_ADC_Handle();
  ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};
  ADC_ChannelConfTypeDef sConfig = {0};
  AnalogWDGConfig.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
  AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
  AnalogWDGConfig.Channel = ADC_CHANNEL_3;
  AnalogWDGConfig.ITMode = ENABLE;
  AnalogWDGConfig.HighThreshold = 4095;
  AnalogWDGConfig.LowThreshold = 620;
  AnalogWDGConfig.FilteringConfig = ADC_AWD_FILTERING_8SAMPLES;
  if (HAL_ADC_AnalogWDGConfig(hadc, &AnalogWDGConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
    Error_Handler();
  }

  // Timer 3 needs to be started to trigger ADC conversions
  HAL_TIM_Base_Start(Get_ADC_TIM_Handle());
  HAL_ADC_Start_IT(hadc);

  for (;;) {

    listLogFiles();

    uint32_t adcValue = HAL_ADC_GetValue(hadc);
    if (isPillDetected()) {
      LogInfo("\ndetected! (ADC: %lu)\n", adcValue);
      resetPillFlag();
    } else {
      LogInfo("\nnot detected! (ADC: %lu)\n", adcValue);
    }
    osDelay(2000);
  }
  /* USER CODE END defaultTask */
}

/* USER CODE BEGIN Header_UI_Task */
/**
* @brief Function implementing the UI_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_UI_Task */
void UI_Task(void *argument)
{
  /* USER CODE BEGIN UI_Task */
	//UI_init();
  /* Infinite loop */
  for(;;)
  {
	UI_handleInput();
    osDelay(100);
  }
  /* USER CODE END UI_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

