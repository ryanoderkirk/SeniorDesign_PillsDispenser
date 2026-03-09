/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    httpserver.c
  * @author  GPM Application Team
  * @brief   Http server application.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "httpserver.h"
#include "main.h"

#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */

/* Please read the following Wiki on how to create the html_pages.h binary:
https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_HTTP_Server_Application */
#include "html_pages.h"

#include "FreeRTOS.h"
#include "event_groups.h"

/* USER CODE BEGIN Includes */
#include "filesystem.h"
/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  HTTP server response type
  */
typedef enum
{
  INDEX_HTML,
  GET_LOG,
  SET_LOG,
  GET_CONFIG,
  SET_CONFIG,
  GET_TIME,
  SET_TIME,
  GET_DATE,
  SET_DATE,
  ERROR_404_HTML,
  UNKNOWN_RESPONSE
} HttpServer_response_e;

/**
  * @brief  HTTP server response structure
  */
typedef struct
{
  HttpServer_response_e response_type;  /*!< Type of response */
  const char *request;                  /*!< Request string */
  const char *response;                 /*!< Response string */
} HttpServer_response_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** Priority of the user pins polling task */
#define PIN_POLLING_THREAD_PRIO        30

/** Stack size of the user pins polling task */
#define PIN_POLLING_TASK_STACK_SIZE    512

/** Priority of the web server child task */
#define WEBSERVER_CHILD_THREAD_PRIO    29

/** Stack size of the web server child task */
#define HTTP_CHILD_TASK_STACK_SIZE     2048

/** HTTP server port */
#define HTTP_PORT                      80

/** Socket timeout in ms */
#define SOCKET_TIMEOUT_MS              1000

/** Event flag for pin status update */
#define EVENT_FLAG_PIN                 (1<<1)

/** Timeout for the pin status update in ms */
#define PIN_TIMEOUT_MS                 9000

/** Buffer size for the child task */
#define HTTP_CHILD_TASK_BUFFER_SIZE    1024

/** Maximum bytes to send in one step */
#define MAX_BYTES_TO_SEND              4096

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Button and Leds states */

/** Event group handle for pin status update */
EventGroupHandle_t pin_handle;

/** Flag to indicate if the button state has changed */

char example_log_response[] =
{
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: application/json\r\n"
  "\r\n"
  "{\"logs\":[{\"category\":\"dispense\",\"time\":\"4PM\",\"channels\":\"1,2,3,4\"},{\"category\":\"containerOpened\",\"time\":\"4PM\",\"channels\":\"1\"},{\"category\":\"missedDispense\",\"time\":\"4PM\",\"channels\":\"2\"}]}"
};

char example_put_response[] =
{
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: application/json\r\n"
  "\r\n"
  "{\"logput\":[{\"category\":\"dispense\",\"time\":\"4PM\",\"channels\":\"1,2,3,4\"},{\"category\":\"containerOpened\",\"time\":\"4PM\",\"channels\":\"1\"},{\"category\":\"missedDispense\",\"time\":\"4PM\",\"channels\":\"2\"}]}"
};
/** Response content depending on the request */
HttpServer_response_t http_server_responses[] =
{
  {INDEX_HTML,      "GET / ",                                     response_index_html},
  {GET_LOG,         "GET /log",                                     example_log_response},
  {SET_LOG,         "PUT /log",                                     example_put_response},
  {GET_CONFIG,      "GET /config",                                     example_log_response},
  {SET_CONFIG,      "PUT /config",                                     example_put_response},
  {GET_TIME,        "GET /time",                                     example_log_response},
  {SET_TIME,        "PUT /time",                                     example_put_response},
  {GET_DATE,        "GET /date",                                     example_log_response},
  {SET_DATE,        "PUT /date",                                     example_put_response},
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Web server child task
  * @param  arg: pointer on argument(not used here)
  */
static void http_server_serve_task(void *arg);

/**
  * @brief  Write HTML data to client
  * @param  client: client information
  * @param  buffer: HTML data to send
  * @param  buffer_size: size of the data to send
  * @retval 0 if success, 1 otherwise
  */
static int32_t http_server_write(int32_t client, const char *buffer, size_t buffer_size);

/**
  * @brief  Close client connection
  * @param  client: client information
  * @retval 0
  */
static int32_t close_client(int32_t client);

/**
  * @brief  Process the HTTP response
  * @param  client: client information
  * @param  recv_buffer: received request data
  */
static void http_process_response(int32_t client, char *recv_buffer);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void http_server_socket(void *arg)
{
  (void)arg;
  const int32_t domain = AF_INET;
  int32_t sock = -1;
  uint16_t port = HTTP_PORT;
  struct sockaddr_in s_addr_in_t = { 0 };
  uint8_t ip_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};
  int32_t timeout = (int32_t)pdMS_TO_TICKS(SOCKET_TIMEOUT_MS);
  int32_t fct_start = 9;

  /* USER CODE BEGIN http_server_socket_1 */

  /* USER CODE END http_server_socket_1 */

  /* Get the soft-AP current IP address */
  if (W6X_Net_AP_GetIPAddress(ip_addr, netmask_addr) != W6X_STATUS_OK)
  {
    LogError("Get soft-AP IP failed\n");
    goto _err;
  }

  LogInfo("Soft-AP IP address : " IPSTR "\n", IP2STR(ip_addr));
  s_addr_in_t.sin_addr.s_addr = ATON(ip_addr);
  s_addr_in_t.sin_port = PP_HTONS(port);
  s_addr_in_t.sin_family = AF_INET;

  /* Create a new TCP server socket with port HTTP_PORT */
  sock = W6X_Net_Socket(domain, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
  {
    LogInfo("Could not create the socket.\n");
    goto _err;
  }

  /* Set the socket Receive timeout option */
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout)) != 0)
  {
    LogError("Could not set the socket options.\n");
    goto _err;
  }

  /* Set the socket Send timeout option */
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout)) != 0)
  {
    LogError("Could not set the socket options.\n");
    goto _err;
  }

  /* Set the socket Send timeout option */
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&timeout, sizeof(timeout)) != 0)
  {
    LogError("Could not set the socket options.\n");
    goto _err;
  }

  /* Bind the socket to the server address */
  if (W6X_Net_Bind(sock, (struct sockaddr *)&s_addr_in_t, sizeof(s_addr_in_t)) != 0)
  {
    LogInfo("\n LwIP Bind Fail\n");
    goto _err;
  }

  LogInfo("\n LwIP Bind Pass\n");

  /* Listen for incoming connections (TCP listen backlog = 5). */
  if (W6X_Net_Listen(sock, 5) != 0)
  {
    LogInfo("\n LwIP Listen Fail\n");
    goto _err;
  }

  LogInfo("\n LwIP Listen Pass\n");

  /* USER CODE BEGIN http_server_socket_2 */

  /* USER CODE END http_server_socket_2 */

  while (1)
  {
    struct sockaddr remotehost_t;
    uint32_t remotehost_size = sizeof(remotehost_t);

    /* Wait for an incoming client connection */
    int32_t newconn = W6X_Net_Accept(sock, (struct sockaddr *)&remotehost_t, (socklen_t *)&remotehost_size);
    if (newconn < 0)
    {
      vTaskDelay(200);
      LogInfo("\n Failed to accept new client requests.\n");
    }
    else
    {
      /* Create a temporary thread to process the incoming HTTP request */
      char thread_name[14];
      const size_t thread_name_len = sizeof(thread_name);
      snprintf(thread_name, thread_name_len, "HTTP_%08" PRIX32, newconn);
      thread_name[thread_name_len - 1] = '\0';

      LogDebug("\n Creation of temporary thread to process an incoming HTTP request : %" PRIi32 "\n", newconn);
      if (pdPASS != xTaskCreate((TaskFunction_t)http_server_serve_task, thread_name,
                                HTTP_CHILD_TASK_STACK_SIZE >> 2,
                                &newconn, WEBSERVER_CHILD_THREAD_PRIO, NULL))
      {
        LogInfo("%s task creation failed\n", thread_name);
      }
      /* Delay added to avoid that too many requests are processed in parallel */
      vTaskDelay(50);
    }
  }

  /* USER CODE BEGIN http_server_socket_last */

  /* USER CODE END http_server_socket_last */

_err:
  /* Error case */
  if ((sock >= 0) && (W6X_Net_Shutdown(sock, 1) != W6X_STATUS_OK))
  {
    LogError("Failed to close server socket\n");
  }
  return;
}

/* USER CODE BEGIN FD */
void build_http_200_response(char *output_buffer, size_t max_len, const char *body) {
    // 1. Calculate length of the body
    int body_len = strlen(body);

    // 2. Construct the response: Status Line + Headers + Blank Line + Body
    // \r\n is the standard line ending for HTTP
    snprintf(output_buffer, max_len,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n" // The "Magic" blank line that separates headers from body
             "%s", 
             body_len, body);
}

/**
 * @brief Constructs a standard HTTP error response
 * @param output_buffer: Where to store the full string
 * @param max_len: Size of the output buffer
 * @param status_code: HTTP code (400, 404, 500, etc.)
 * @param error_msg: The custom message body
 */
void build_http_error_response(char *output_buffer, size_t max_len, int status_code, const char *error_msg) {
    const char *status_text;

    // 1. Map codes to standard HTTP status strings
    switch(status_code) {
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default:  status_text = "Error"; break;
    }

    // 2. Body length for Content-Length header
    int body_len = strlen(error_msg);

    // 3. Format the full response
    // Using \r\n is mandatory for HTTP compliance
    snprintf(output_buffer, max_len,
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status_code, status_text, body_len, error_msg);
}
/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void http_server_serve_task(void *arg)
{
  int32_t client = *((int32_t *)arg);
  int32_t bytes_received;
  int32_t recv_total_len = 0;
  /* Allocate considering the biggest buffer the client can send */
  size_t recv_buffer_len = HTTP_CHILD_TASK_BUFFER_SIZE;
  char *recv_buffer = (char *)pvPortMalloc(recv_buffer_len);
  if (recv_buffer == NULL)
  {
    LogError("Unable to allocate recv buffer\n");
    goto _err;
  }

  /* USER CODE BEGIN http_server_serve_task_1 */

  /* USER CODE END http_server_serve_task_1 */

  /* Read in the request */
  recv_buffer[0] = '\0';
  do
  {
    bytes_received = W6X_Net_Recv(client, (uint8_t *)&recv_buffer[recv_total_len], recv_buffer_len, 0);
    if (bytes_received < 0) /* No data received or error */
    {
      break;
    }

    /* Case where we have receive less than the buffer allocated */
    if (bytes_received < recv_buffer_len)
    {
      recv_total_len += bytes_received;
      break;
    }

    recv_total_len += bytes_received;
    recv_buffer_len -= bytes_received;
  } while ((bytes_received != 0) && (recv_buffer_len > 0));

  if (recv_total_len == 0)
  {
    LogError("No data read on the socket, closing the socket\n");
    goto _close;
  }
  LogDebug("\n %" PRIi32 " >>> %" PRIi32 " <<<\n", client, recv_total_len);

  /* Count can be negative. */
  if (recv_total_len > 0)
  {
    recv_buffer[recv_total_len++] = '\0';
    LogDebug("\n %" PRIi32 " >>> %s <<<\n", client, recv_buffer);
  }

  /* USER CODE BEGIN http_server_serve_task_2 */

  /* USER CODE END http_server_serve_task_2 */

  /* Process the response */
  http_process_response(client, recv_buffer);

  /* USER CODE BEGIN http_server_serve_task_last */

  /* USER CODE END http_server_serve_task_last */

_close:
  vPortFree(recv_buffer);
_err:
  close_client(client);
  vTaskDelete(NULL);
}

static int32_t http_server_write(int32_t client, const char *buffer, size_t buffer_size)
{
  int32_t bytes_sent = 0;

  /* USER CODE BEGIN http_server_write_1 */

  /* USER CODE END http_server_write_1 */

  LogDebug("[%" PRIi32 "] *****> %s<*****\n", client, buffer);

  do /* Send the data. Can be done in multiple steps. */
  {
    bytes_sent = W6X_Net_Send(client, (void *)buffer, buffer_size, 0);
    if (bytes_sent < 0)
    {
      LogError("[%" PRIi32 "] *****> SEND ERROR <*****\n", client);
      return 1;
    }
    buffer_size -= bytes_sent;
    buffer += bytes_sent;
  } while (buffer_size > 0);

  /* USER CODE BEGIN http_server_write_last */

  /* USER CODE END http_server_write_last */

  return 0;
}

int32_t close_client(int32_t client)
{
  /* USER CODE BEGIN close_client_1 */

  /* USER CODE END close_client_1 */

  /* Close the connection */
  (void)W6X_Net_Close(client);
  LogDebug("!!! Closed connection <%" PRIi32 "> !!!\n", client);

  /* USER CODE BEGIN close_client_last */

  /* USER CODE END close_client_last */

  return 0;
}

static void http_process_response(int32_t client, char *recv_buffer)
{
  HttpServer_response_e response = UNKNOWN_RESPONSE;
  char *response_data = NULL;

  /* USER CODE BEGIN http_process_response_1 */

  /* USER CODE END http_process_response_1 */

  /* Check the request to determine the response */
  for (uint32_t i = 0; i < sizeof(http_server_responses) / sizeof(http_server_responses[0]); i++)
  {
    if (strncmp(recv_buffer, http_server_responses[i].request, strlen(http_server_responses[i].request)) == 0)
    {
      response = http_server_responses[i].response_type;
      response_data = (char *)http_server_responses[i].response;
      break;
    }
  }

  /* USER CODE BEGIN http_process_response_2 */

  static char response_body[256];
  static char full_response[256];
  /* USER CODE END http_process_response_2 */

  if (response == UNKNOWN_RESPONSE) /* Request not recognized, return 404 error */
  {
    response_data = (char *)response_error_404_html;
  }


  if (response == GET_LOG) /* Request not recognized, return 404 error */
  {
    LogEntry_t log;

    int result = readLog(&log);
    if (result == 0) {
      snprintf(response_body, sizeof(response_body),
                       "LOG EVENT: 20%02d-%02d-%02d %02d:%02d:%02d | Type: %d | Data: %d,%d,%d,%d",
                       log.year, log.month, log.day, log.hour, log.min, log.sec, 
                       log.logType, log.one, log.two, log.three, log.four);
      build_http_200_response(full_response, sizeof(full_response), response_body);
      response_data = full_response;
    }
    else if (result == -1) {
      build_http_error_response(full_response, sizeof(full_response), 404, "No log has been written today");
      response_data = full_response;
    }
    else {
      build_http_error_response(full_response, sizeof(full_response), 404, "Log read failed");
      response_data = full_response;
    }
  }

  if (response == SET_LOG) {
    LogEntry_t new_log = {0};
    int items_parsed;

    char *body = strstr(recv_buffer, "\r\n\r\n");

    if (body != NULL) {
      body += 4; // JUMP PAST THE NEWLINES

      int y, m, d, hh, mm, ss, type, d1, d2, d3, d4;

      items_parsed = sscanf(
          body, "LOG EVENT: 20%d-%d-%d %d:%d:%d | Type: %d | Data: %d,%d,%d,%d", &y,
          &m, &d, &hh, &mm, &ss, &type, &d1, &d2, &d3, &d4);

      if (items_parsed == 11) {
        new_log.year = (uint8_t)y;
        new_log.month = (uint8_t)m;
        new_log.day = (uint8_t)d;
        new_log.hour = (uint8_t)hh;
        new_log.min = (uint8_t)mm;
        new_log.sec = (uint8_t)ss;
        new_log.logType = (LogType_t)type;
        new_log.one = (uint8_t)d1;
        new_log.two = (uint8_t)d2;
        new_log.three = (uint8_t)d3;
        new_log.four = (uint8_t)d4;

        //temporarily rewrite log with system time instead of time supplied from log, for consistency
        RTC_DateTypeDef date;
        RTC_TimeTypeDef time;
        get_rtc_typedef(&date, &time);
        new_log.year = date.Year;
        new_log.month = date.Month;
        new_log.day = date.Date;
        new_log.hour = time.Hours;
        new_log.min = time.Minutes;
        new_log.sec = time.Seconds;


        int result = writeLog(&new_log);
        if (result == 0) {
          strcpy(full_response, "HTTP/1.1 200 OK\r\nContent-Length: "
                                "0\r\nConnection: close\r\n\r\n");
        } 
        else {
          build_http_error_response(full_response, sizeof(full_response), 404, "File write error");
          response_data = full_response;
        }
      } else {
        strcpy(full_response,
               "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
      }
    } else {
      strcpy(full_response,
             "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
    }
    response_data = full_response;
  }

  if (response == GET_CONFIG) /* Request not recognized, return 404 error */
  {
    int channel = -1;

    // 1. Find the first space (after "GET")
    char *uri_start = strchr(recv_buffer, ' ');
    if (uri_start == NULL)
      return;
    uri_start++; // Move past the space to the '/'

    // 2. Find the second space (before "HTTP/1.1")
    char *uri_end = strchr(uri_start, ' ');
    if (uri_end == NULL)
      return;

    // This turns "/config?ch=1 HTTP/1.1" into "/config?ch=1\0"
    char original_char = *uri_end;
    *uri_end = '\0';

    // 4. Now use strchr to find your '?' within the isolated URI
    char *query = strchr(uri_start, '?');
    if (query) {
      sscanf(query, "?ch=%d", &channel);
    }
    *uri_end = original_char;

    Config_t config;

    if (channel > 0 && channel < 5) {
      if (readConfig(&config, channel) != 0) {
        build_http_error_response(full_response, sizeof(full_response), 404, "config file has not been configured!");
        response_data = full_response;
      }
      else {
        snprintf(response_body, sizeof(response_body),
                 "CONFIG | Channel: %d | Count: %d | Pill: %s", config.channel,
                 config.pillCount, config.pillName);
        build_http_200_response(full_response, sizeof(full_response),
                                response_body);
        response_data = full_response;
      }
    }
    else {
        build_http_error_response(full_response, sizeof(full_response), 400, "Incorrect channel number!");
        response_data = full_response;
    }
  }

  if (response == SET_CONFIG) {
    Config_t new_config = {0};
    int items_parsed;

    char *body = strstr(recv_buffer, "\r\n\r\n");

    if (body != NULL) {
      body += 4; // JUMP PAST THE NEWLINES

      int ch, count;
      char name_tmp[64] = {0};

      items_parsed =
          sscanf(body, "CONFIG | Channel: %d | Count: %d | Pill: %63s", &ch,
                 &count, name_tmp);

      if (items_parsed == 3) {
        new_config.channel = (uint8_t)ch;
        new_config.pillCount = (uint8_t)count;

        // 3. Copy string safely into the struct
        strncpy((char *)new_config.pillName, name_tmp,
                sizeof(new_config.pillName) - 1);
        new_config.pillName[sizeof(new_config.pillName) - 1] =
            '\0'; // Force null terminator

        // 4. Write to LittleFS
        if (writeConfig(&new_config) == 0) {
          strcpy(full_response, "HTTP/1.1 200 OK\r\nContent-Length: "
                                "0\r\nConnection: close\r\n\r\n");
        } else {
          strcpy(full_response, "HTTP/1.1 500 Internal Server "
                                "Error\r\nContent-Length: 0\r\n\r\n");
        }

      } else {
        strcpy(full_response,
               "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
      }
    } else {
      strcpy(full_response,
             "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
    }
    response_data = full_response;
  }


  if (response == GET_TIME) {
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    get_rtc_typedef(&date, &time);

    snprintf(response_body, sizeof(response_body), "20%02d-%02d-%02d %02d:%02d:%02d", date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
    build_http_200_response(full_response, sizeof(full_response), response_body);
    response_data = full_response;
  }

  if (response == SET_TIME) {
    char *body = strstr(recv_buffer, "\r\n\r\n");

    if (body != NULL) {
      body += 4; // Jump past the \r\n\r\n

      int y, m, d, hh, mm, ss;
      // Parse: "TIME | 2026-03-07 18:30:00"
      int items =
          sscanf(body, "TIME | %d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss);

      if (items == 6) {
        // Don't accept silly times
        if (y >= 2000 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
          update_system_time(y, m, d, hh, mm, ss);
          build_http_200_response(full_response, sizeof(full_response),"Time updated successfully");
        } else {
          build_http_error_response(full_response, sizeof(full_response), 400,
                                    "Error: Invalid date/time values");
        }
      } else {
        build_http_error_response(
            full_response, sizeof(full_response), 400,
            "Error: Use format 'TIME | YYYY-MM-DD HH:MM:SS'");
      }
    } else {
      build_http_error_response(full_response, sizeof(full_response), 400,
                                "Error: No body found");
    }
    response_data = full_response;
  }

  /* Send the response */
  if (1 == http_server_write(client, response_data, strlen(response_data)))
  {
    return;
  }

  /* USER CODE BEGIN http_process_response_last */

  /* USER CODE END http_process_response_last */
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
