/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    logshell_ctrl.c
  * @author  GPM Application Team
  * @brief   logshell_ctrl (uart interface)
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
#include <stdint.h>
#include <string.h>

/* Application */
#include "main.h"
#include "app_config.h"
#include "logshell_ctrl.h"
#include "bsp_conf.h"

#include "FreeRTOS.h"  /* "include FreeRTOS.h" must appear in source files before "include queue.h" */
#include "queue.h"
#include "event_groups.h"

#include <stdio.h> /* Standard I/O .h-file */
#if defined(__ICCARM__)
#include <LowLevelIOInterface.h>
#endif /* __ICCARM__ */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
#ifndef UART_HANDLE
#if (LOG_OUTPUT_MODE == LOG_OUTPUT_UART)
#error "UART instance not selected. Please use platform settings panel in STM32CubeMX GUI."
#endif /* LOG_OUTPUT_MODE */
#else
/** UART handle */
extern UART_HandleTypeDef UART_HANDLE;
#endif /* UART_HANDLE */

/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
#ifdef UART_HANDLE
/**
  * \def PUTCHAR_PROTOTYPE
  * Common macro to redirect console output to COM
  */
#if defined(__ICCARM__)
/* New definition from EWARM V9, compatible with EWARM8 */
/**
  * @brief  Redirect console output to COM
  * @param  ch: Character to send
  * @retval Character sent
  */
int iar_fputc(int ch);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)
#elif defined ( __GNUC__) && !defined(__clang__)
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
/**
  * @brief  Redirect console output to COM
  * @param  ch: Character to send
  * @retval Character sent
  */
int __io_putchar(int ch);
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
/**
  * @brief  Redirect console output to COM
  * @param  ch: Character to send
  * @param  f: Not used
  * @retval Character sent
  */
int fputc(int ch, FILE *f);
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __ICCARM__ */
#endif /* UART_HANDLE */

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
#if defined(UART_HANDLE)
#if defined(__ICCARM__)
/**
  * @brief  Retargets the C library __write function to the IAR function iar_fputc
  * @param  file: file descriptor
  * @param  ptr: pointer to the buffer where the data is stored
  * @param  len: length of the data to write in bytes
  * @retval length of the written data in bytes
  */
size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;
  /* USER CODE BEGIN __write_1 */

  /* USER CODE END __write_1 */

  for (idx = 0; idx < len; idx++)
  {
    iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
  /* USER CODE BEGIN __write_End */

  /* USER CODE END __write_End */
}
#endif /* __ICCARM__ */

PUTCHAR_PROTOTYPE
{
  /* USER CODE BEGIN PUTCHAR_PROTOTYPE_1 */

  /* USER CODE END PUTCHAR_PROTOTYPE_1 */
  HAL_UART_Transmit(&UART_HANDLE, (unsigned char *)&ch, 1, 1000);
  return ch;
  /* USER CODE BEGIN PUTCHAR_PROTOTYPE_End */

  /* USER CODE END PUTCHAR_PROTOTYPE_End */
}

#endif /* UART_HANDLE */

void LoggingInit(void)
{
  /* USER CODE BEGIN LoggingInit_1 */

  /* USER CODE END LoggingInit_1 */
}

void ShellInit(void)
{
  /* USER CODE BEGIN ShellInit_1 */

  /* USER CODE END ShellInit_1 */
}

/* USER CODE BEGIN vLoggingPrintf_example */
#include <stdarg.h>
int32_t vLoggingPrintf(uint32_t log_level, const uint8_t metadata_print, const uint32_t line_number,
                       const char *const p_file_name, const char *const p_format, ...);

int32_t vLoggingPrintf(uint32_t log_level, const uint8_t metadata_print, const uint32_t line_number,
                       const char *const p_file_name, const char *const p_format, ...)
{
  int32_t len = 0;
#ifdef UART_HANDLE
  va_list args;
  va_start(args, p_format);
  len= vprintf( p_format, args);
  va_end(args);
#endif /* UART_HANDLE */
  return len;
}
/* USER CODE END vLoggingPrintf_example */

/* USER CODE BEGIN FD */

/* USER CODE END FD */
