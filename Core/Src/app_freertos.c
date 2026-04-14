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
#include "Alert.h"
#include "servoControl.h"
#include "ILI9341_STM32_Driver.h"

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
/* Definitions for alert_Task */
osThreadId_t alert_TaskHandle;
const osThreadAttr_t alert_Task_attributes = {
  .name = "alert_Task",
  .priority = (osPriority_t) osPriorityNormal,
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

  /* creation of alert_Task */
  alert_TaskHandle = osThreadNew(alert_Task, NULL, &alert_Task_attributes);

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
  int result = filesystemInit();
  if (result != 0) {
     LogError("Filesystem init failed\n");
  }
  else {
    LogEntry_t log = {0};
    fillLogTimestamp(&log);
    log.logType = systemBoot;
    writeLog(&log);
  }
  /* Infinite loop */
  for (;;) {
    if(GPIOisPillDetected(3)) {
      LogDebug("GPIO style Pill detected!\n");
      osDelay(200);
      //GPIOresetPillFlag(3);
    }
    osDelay(20);
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
	  ILI9341_Init();
	  UI_init();

  /* Infinite loop */
  for(;;)
  {
	UI_handleInput();
    osDelay(100);
  }
  /* USER CODE END UI_Task */
}

/* USER CODE BEGIN Header_alert_Task */
/**
* @brief Function implementing the alert_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_alert_Task */
void alert_Task(void *argument)
{
  /* USER CODE BEGIN alert_Task */
  /* Infinite loop */
  for(;;)
  {
	  checkDosageAlert();
	  checkLowPillAlert();

	  //clearDosageAlert();
	  LogDebug("Dosage Alert:%d\n",dosageAlert);
	  for(int channel = 1; channel < 5; channel++){
		  uint8_t triggered = readLowPillAlert(channel);
		  LogDebug("Low Pill Alert - Channel %d: %d\n",channel,triggered);
		  //clearLowPillAlert(channel);
		  //resetLowPillAlert(channel);
	  }
    osDelay(1000*60);
  }
  /* USER CODE END alert_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

