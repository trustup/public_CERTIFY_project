/**
  ******************************************************************************
  * @file    cellular_service_task.c
  * @author  MCD Application Team
  * @brief   Wrapper to STM32 timing
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#if (USE_MBEDTLS == 1)
#include <time.h>
#include <rtc.h>

#include "mbedtls_timedate.h"
#include "time_date.h"
#include "net_connect.h"
#include "mbedtls_credentials.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** Host from which the time/date will be retrieved.*/
#ifdef USE_CLEAR_TIMEDATE
#define TIME_SOURCE_HTTP_HOST   "www.st.com"
#define TIME_SOURCE_HTTP_PORT   80
#define TIME_SOURCE_HTTP_PROTO  NET_PROTO_TCP
#else
#define TIME_SOURCE_HTTP_HOST   "www.gandi.net"
#define TIME_SOURCE_HTTP_PORT   443
#define TIME_SOURCE_HTTP_PROTO  NET_PROTO_TLS
#endif  /* USE_CLEAR_TIMEDATE */

/** Maximum number of DNS lookup or connection trials */
#define MBEBTLS_TIME_NET_MAX_RETRY  4
#define MBEDTLS_TIME_SYNC_SYSTEM    1446741778


/** Size of the HTTP read buffer.
  *  Should be large enough to contain a complete HTTP response header. */
#define NET_BUF_SIZE  1000U


/* Private function prototypes -----------------------------------------------*/
static uint32_t mbedtls_timedate_find_str(uint8_t *buf, const uint8_t *str_find);

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private Functions Definition ------------------------------------------------------*/
static uint32_t mbedtls_timedate_find_str(uint8_t *buf,
                                          const uint8_t *str_find)
{
  uint32_t i;
  uint32_t j;
  uint32_t offset;
  uint32_t buf_len;
  uint32_t str_find_len;

  offset = 0;
  buf_len      = crs_strlen(buf);
  str_find_len = crs_strlen(str_find);

  if (buf_len > str_find_len)
  {
    for (i = 0U ; (i < (buf_len - str_find_len)) && (i < NET_BUF_SIZE); i++)
    {
      if (buf[i] == str_find[0])
      {
        bool loop_end = false;
        j = 1;
        while (loop_end == false)
        {
          uint32_t indice = i + j;
          if ((j < str_find_len) && (indice < NET_BUF_SIZE))
          {
            if (buf[indice] != str_find[j])
            {
              loop_end = true;
            }
            else
            {
              j++;
            }
          }
          else
          {
            loop_end = true;
          }
        }

        if (j == str_find_len)
        {
          offset = i + j;
          break;
        }
      }
    }
  }
  return offset;
}

/* External Functions Definition ------------------------------------------------------*/

/* Function called by mbedtls to return time (in second) */
time_t time(time_t *pointer)
{
  UNUSED(pointer);
  static const time_t          timeSyncSystem = MBEDTLS_TIME_SYNC_SYSTEM;
  time_t        returnTime;
  struct tm    *pCalendar;
  uint32_t  ret;

  RTC_DateTypeDef sdatestructure;
  RTC_TimeTypeDef stimestructure;
  returnTime = 0;
  pCalendar             = gmtime(&timeSyncSystem);
  ret  = (uint32_t)HAL_RTC_GetTime(&hrtc, &stimestructure, FORMAT_BIN);
  ret |= (uint32_t)HAL_RTC_GetDate(&hrtc, &sdatestructure, FORMAT_BIN);

  if (ret == 0U)
  {
    pCalendar->tm_year           = (int32_t)sdatestructure.Year + 100;
    pCalendar->tm_mon            = (int32_t)sdatestructure.Month - 1;
    pCalendar->tm_mday           = (int32_t)sdatestructure.Date;
    pCalendar->tm_wday           = (int32_t)sdatestructure.WeekDay - 1;
    pCalendar->tm_hour           = (int32_t)stimestructure.Hours;
    pCalendar->tm_min            = (int32_t)stimestructure.Minutes;
    pCalendar->tm_sec            = (int32_t)stimestructure.Seconds;
    pCalendar->tm_isdst          = 0;

    returnTime        = mktime(pCalendar);
  }
  return returnTime;
}




/**
  * @brief Set the RTC time and date from an HTTP response.
  * @param In:  force_apply    Force applying the time/date retrieved from the server,
  *                            even if the server certificate verification failed.
  *                            Useful for initializing the RTC when it is not backed up by a battery.
  * @note  Pre-conditions:
  *   . Wifi network connected
  *   . One free socket
  * @retval  Error code
  *            TD_OK
  *            TD_ERR_CONNECT   Could not connect to the network and join the web server.
  *            TD_ERR_HTTP      Could not parse the time and date from the web server response.
  *            TD_ERR_RTC       Could not set the RTC.
  *            TD_ERR_TLS_CERT  The server certificate verification failed. Applicable only when force_apply is false.
  *                             .
  */
int32_t mbedtls_timedate_set_from_network(void)
{
  static const uint8_t *str_date = "Date: ";
  static uint8_t *http_request = (uint8_t *)"HEAD / HTTP/1.1\r\nHost: "TIME_SOURCE_HTTP_HOST"\r\n\r\n";
  int32_t rc;
  uint32_t retu;
  int32_t ret;
  int32_t sock;
  int32_t count;
  int32_t len ;

  rc    = TD_OK;
  count = MBEBTLS_TIME_NET_MAX_RETRY;
  CRC_CHAR_t buffer[NET_BUF_SIZE +
                    1]; /* +1 to be sure that the buffer is closed by a \0,
                             so that it may be parsed by string commands. */
  (void)memset(buffer, 0, sizeof(buffer));

  sockaddr_in_t addr;
  addr.sin_len = (int32_t)sizeof(sockaddr_in_t);

  ret = net_if_gethostbyname(NULL, (sockaddr_t *)&addr, (char_t *)TIME_SOURCE_HTTP_HOST);
  while ((ret < 0) && (count > 0))
  {
    /*    msg_warning("Could not find hostname ipaddr %s\nRetrying...\n",TIME_SOURCE_HTTP_HOST);  */
    count--;
    HAL_Delay(1000);
    ret = net_if_gethostbyname(NULL, (sockaddr_t *)&addr, (char_t *)TIME_SOURCE_HTTP_HOST);
  }

  if (count <= 0)
  {
    /*  msg_error("Could not find hostname ipaddr %s\nAbandon.\n",TIME_SOURCE_HTTP_HOST); */
    rc = TD_ERR_CONNECT;
  }
  else
  {
    (void)memset(buffer, 0, sizeof(buffer));

    addr.sin_port = NET_HTONS(TIME_SOURCE_HTTP_PORT);
    sock = net_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);

    retu   = (uint32_t)NET_OK;
    if (sock < 0)
    {
      /* msg_error("Could not create the socket.\n");  */
    }
    else
    {
#define NET_READ_TIMEOUT  5000
      uint32_t timeout = NET_READ_TIMEOUT;

#ifdef USE_CLEAR_TIMEDATE
      retu = (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_RCVTIMEO, (void *) &timeout, sizeof(uint32_t));
#else
#define NET_SEND_TIMEOUT  1000
      mbedtls_credentials_config_t    *timedate_tls_credentials_config;
      uint32_t timeout_send = NET_SEND_TIMEOUT;
      bool false_val = false;

      timedate_tls_credentials_config = mbedtls_credentials_get_config();
      retu  = (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_SECURE, NULL, 0);
      retu |= (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_RCVTIMEO,
                                       (const void *)&timeout, sizeof(uint32_t));
      retu |= (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_SNDTIMEO,
                                       (void *) &timeout_send, sizeof(uint32_t));
      retu |= (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_CA_CERT,
                                       (void *)timedate_tls_credentials_config->tls_root_ca_cert,
                                       strlen((CRC_CHAR_t *)timedate_tls_credentials_config->tls_root_ca_cert) + 1U);
      retu |= (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_SERVER_NAME,
                                       (void *) TIME_SOURCE_HTTP_HOST, strlen(TIME_SOURCE_HTTP_HOST) + 1U);
      retu |= (uint32_t)net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_SERVER_VERIFICATION,
                                       (void *) &false_val, sizeof(bool));
#endif  /* USE_CLEAR_TIMEDATE */
    }

    if (retu == (uint32_t)NET_OK)
    {
      count = MBEBTLS_TIME_NET_MAX_RETRY;
      ret = net_connect(sock, (sockaddr_t *)&addr, sizeof(addr));
      while ((ret < 0) && (count > 0))
      {
        /* msg_warning("Could not open the connection.\n"); */
        if (ret == NET_ERROR_AUTH_FAILURE)
        {
          /* msg_error("An incorrect system time may have resulted in a TLS authentication error.\n"); */
          rc = TD_ERR_TLS_CERT;
          break;
        }
        /*  msg_warning("Retrying...\n"); */
        HAL_Delay(1000);
        count--;
        ret = net_connect(sock, (sockaddr_t *)&addr, sizeof(addr));
      }
    }

    if ((ret == NET_OK) && (rc == TD_OK))
    {
      len   = (int32_t)crs_strlen(http_request);
      ret = net_send(sock, (uint8_t *) http_request, (uint32_t)len, 0);
      if (ret == len)
      {
        uint32_t offset_date;

        int32_t read = 0;
        do
        {
          len = net_recv(sock, (uint8_t *) buffer + read, (NET_BUF_SIZE - (uint32_t)read), 0);
          if (len > 0)
          {
            read += len;
          }
          offset_date = mbedtls_timedate_find_str((uint8_t *)buffer, str_date);
        } while (((len >= 0) || (len == NET_TIMEOUT)) && (offset_date == 0U));

        if (offset_date == 0U)
        {
          /* msg_error("No 'Date:' line found in the HTTP response header.\n"); */
          rc = TD_ERR_HTTP;
        }
        else
        {
          (void)timedate_set_from_http((uint8_t *)&buffer[offset_date]);
        }
      }

      ret = net_closesocket(sock);
    }

    /* Translate a socket closure error in network error. */
    if ((rc == TD_OK) && (ret != NET_OK))
    {
      rc = TD_ERR_CONNECT;
    }
  }

  return rc;
}
#endif  /* (USE_MBEDTLS == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
