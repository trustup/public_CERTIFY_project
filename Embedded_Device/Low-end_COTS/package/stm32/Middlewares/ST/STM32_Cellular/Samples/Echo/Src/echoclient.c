/**
  ******************************************************************************
  * @file    echoclient.c
  * @author  MCD Application Team
  * @brief   Example of echo client to interact with different distant server
  *          by using different socket protocols
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
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

#if (USE_ECHO_CLIENT == 1)

#include <string.h>
#include <stdbool.h>

#include "echoclient.h"

#include "rtosal.h"
#include "error_handler.h"

#if (USE_NETWORK_LIBRARY == 1)
#include "net_connect.h" /* includes all other includes */
#else /* USE_NETWORK_LIBRARY == 0 */
#include "com_sockets.h" /* includes all other includes */
#endif /* USE_NETWORK_LIBRARY == 1 */

#include "dc_common.h"
#include "cellular_mngt.h"
#include "cellular_datacache.h"
#include "cellular_runtime_custom.h"

#include "setup.h"
#include "app_select.h"
#include "menu_utils.h"

#if (USE_RTC == 1)
#include "time_date.h"
#endif /* (USE_RTC == 1) */

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* USE_CMD_CONSOLE == 1 */

/* Private typedef -----------------------------------------------------------*/
/* Echo client socket state */
typedef enum
{
  ECHOCLIENT_SOCKET_INVALID = 0,
  ECHOCLIENT_SOCKET_CREATED,
  ECHOCLIENT_SOCKET_CONNECTED,
  ECHOCLIENT_SOCKET_SENDING,
  ECHOCLIENT_SOCKET_WAITING_RSP,
  ECHOCLIENT_SOCKET_CLOSING
} echoclient_socket_state_t;

/* Managed Protocol */
typedef enum
{
  ECHOCLIENT_SOCKET_TCP_PROTO = 0,     /* create, connect, send,   recv     */
  ECHOCLIENT_SOCKET_UDP_PROTO,         /* create, connect, send,   recv     */
  ECHOCLIENT_SOCKET_UDP_SERVICE_PROTO, /* create,   NA   , sendto, recvfrom */
  ECHOCLIENT_SOCKET_PROTO_MAX          /* Must be always at the end */
} echoclient_socket_protocol_t;

/* Echo distant server type */
typedef enum
{
  ECHOCLIENT_MBED_SERVER = 0,
  ECHOCLIENT_UBLOX_SERVER,
  ECHOCLIENT_LOCAL_SERVER,
  ECHOCLIENT_SERVER_MAX,
} echoclient_distant_server_t;

/* Statistics counters */
typedef struct
{
  uint32_t ok;   /* count number ok */
  uint32_t ko;   /* count number ko */
} echoclient_counter_t;

/* Statistics counters */
typedef struct
{
  uint32_t count;                 /* count number of echo done */
  echoclient_counter_t connect;   /* connect count number      */
  echoclient_counter_t send;      /* send    count number      */
  echoclient_counter_t receive;   /* receive count number      */
  echoclient_counter_t close;     /* close   count number      */
} echoclient_stat_t;

/* Socket descriptor */
typedef struct
{
  int32_t id;                      /* Socket id = result of com_socket() */
  echoclient_socket_state_t state; /* Socket state */
  bool closing;                    /* false: socket don't need to be closed, true: socket need to be closed */
} echoclient_socket_desc_t;

/* Performance result structure */
typedef struct
{
  uint16_t iter_ok;
  uint32_t total_time;
} echoclient_performance_result_t;

/* Private defines -----------------------------------------------------------*/
#if (USE_TRACE_ECHO_CLIENT == 1U)

#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_ECHOCLIENT, DBL_LVL_P0, format "\n\r", ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_ECHOCLIENT, DBL_LVL_P0, "Echoclt: " format "\n\r", ## args)
#define PRINT_DBG(format, args...) \
  TRACE_PRINT(DBG_CHAN_ECHOCLIENT, DBL_LVL_P1, "Echoclt: " format "\n\r", ## args)
#define PRINT_ERR(format, args...) \
  TRACE_PRINT(DBG_CHAN_ECHOCLIENT, DBL_LVL_ERR, "Echoclt ERROR: " format "\n\r", ## args)

#else /* USE_PRINTF == 1U */
#include <stdio.h>
#define PRINT_FORCE(format, args...)     (void)printf(format "\n\r", ## args);
#define PRINT_INFO(format, args...)      (void)printf("Echoclt: " format "\n\r", ## args);
/*#define PRINT_DBG(format, args...)       (void)printf("Echo: " format "\n\r", ## args);*/
/* To reduce trace PRINT_DBG is deactivated when using printf */
#define PRINT_DBG(...)                   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)       (void)printf("Echoclt ERROR: " format "\n\r", ## args);

#endif /* USE_PRINTF == 0U */

#else /* USE_TRACE_ECHO_CLIENT == 0U */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_ECHOCLIENT, DBL_LVL_P0, format "\n\r", ## args)
#else /* USE_PRINTF == 1U */
#include <stdio.h>
#define PRINT_FORCE(format, args...)     (void)printf(format "\n\r", ## args);
#endif /* USE_PRINTF == 0U */
#define PRINT_INFO(...)      __NOP(); /* Nothing to do */
#define PRINT_DBG(...)       __NOP(); /* Nothing to do */
#define PRINT_ERR(...)       __NOP(); /* Nothing to do */

#endif /* USE_TRACE_ECHO_CLIENT == 1U */

#define ECHOCLIENT_DISTANT_UNKNOWN_DATE_TIME_PORT  ((uint16_t)0xFFFFU)

/*
$ host echo.mbedcloudtesting.com
echo.mbedcloudtesting.com has address 52.215.34.155
echo.mbedcloudtesting.com has IPv6 address 2a05:d018:21f:3800:8584:60f8:bc9f:e614
echo.mbedcloudtesting.com has echo server on port 7
echo.mbedcloudtesting.com has date time server on port 13
*/
#define ECHOCLIENT_DISTANT_MBED_NAME                 ((uint8_t *)"echo.mbedcloudtesting.com")
#define ECHOCLIENT_DISTANT_MBED_IP                   ((uint32_t)0x9B22D734U)   /*  52.215.34.155 */
#define ECHOCLIENT_DISTANT_MBED_PORT                 ((uint16_t)7U)
/* A first welcome msg is send / or not by the distant server */
/* In case of TCP or UDP connection, MBED don't send a first message */
//#define ECHOCLIENT_DISTANT_MBED_TCP_WELCOME_MSG      NULL /* No welcome msg in TCP */
#define ECHOCLIENT_DISTANT_MBED_TCP_WELCOME_MSG      ((uint8_t *)"Hello World !")
#define ECHOCLIENT_DISTANT_MBED_UDP_WELCOME_MSG      NULL /* No welcome msg in UDP */
/* Important : check echoclient_obtain_date_time() for re-ordering Date Time parameter */
#define ECHOCLIENT_DISTANT_MBED_DATE_TIME_PORT       ((uint16_t)13U)

/*
$ host echo.u-blox.com
echo.u-blox.com has address 195.34.89.241
echo.u-blox.com has echo server on port 7
*/
#define ECHOCLIENT_DISTANT_UBLOX_NAME                ((uint8_t *)"echo.u-blox.com")
#define ECHOCLIENT_DISTANT_UBLOX_IP                  ((uint32_t)0xF15922C3U)   /*  195.34.89.241 */
#define ECHOCLIENT_DISTANT_UBLOX_PORT                ((uint16_t)7U)
/* A first welcome msg is send / or not by the distant server */
/* In case of TCP connection, u-blox send a first message before echoing the send data */
#define ECHOCLIENT_DISTANT_UBLOX_TCP_WELCOME_MSG     (uint8_t *)"U-Blox AG TCP/UDP test service\n"
/* In case of UDP connection, u-blox don't send a first message */
#define ECHOCLIENT_DISTANT_UBLOX_UDP_WELCOME_MSG      NULL  /* No welcome msg in UDP */
/* Important : check echoclient_obtain_date_time() for re-ordering Date Time parameter */
/* Re-ordering date-time is unknown */
#define ECHOCLIENT_DISTANT_UBLOX_DATE_TIME_PORT      (ECHOCLIENT_DISTANT_UNKNOWN_DATE_TIME_PORT)

/*
$ host stmicroelectronics local server
stmicroelectronics local server has address 195.34.89.241
stmicroelectronics local server has echo server on port 7
*/
#define ECHOCLIENT_DISTANT_LOCAL_NAME                ((uint8_t *)"\0") /* Set to to by-pass DNS resolution
                                                                          DNS server ignore local server IP */
//#define ECHOCLIENT_DISTANT_LOCAL_IP                  ((uint32_t)0x0102A8C0U)   /*  192.168.2.1 */
#define ECHOCLIENT_DISTANT_LOCAL_IP                  ((uint32_t)0x51B4A193U)   /*  147.161.180.81 */
#define ECHOCLIENT_DISTANT_LOCAL_PORT                ((uint16_t)9500U)
/* In case of TCP / UDP connection, local server don't send a first message */
#define ECHOCLIENT_DISTANT_LOCAL_TCP_WELCOME_MSG     NULL  /* No welcome msg in TCP */
#define ECHOCLIENT_DISTANT_LOCAL_UDP_WELCOME_MSG     NULL  /* No welcome msg in UDP */
/* Important : check echoclient_obtain_date_time() for re-ordering Date Time parameter */
/* Re-ordering date-time is unknown */
#define ECHOCLIENT_DISTANT_LOCAL_DATE_TIME_PORT      (ECHOCLIENT_DISTANT_UNKNOWN_DATE_TIME_PORT)

#define ECHOCLIENT_NAME_SIZE_MAX  (uint8_t)(64U) /* MAX_SIZE_IPADDR of cellular_service.h */

#define ECHOCLIENT_LABEL  ((uint8_t*)"Echoclt")

#define ECHOCLIENT_DEFAULT_PROCESSING_PERIOD    2000U /* default period between two send in ms. */
#define ECHOCLIENT_SND_RCV_MAX_SIZE             1500U /* send/receive buffer size max in bytes */
#if (USE_RTC == 1)
#define ECHOCLIENT_SND_RCV_MIN_SIZE               21U /* %02d:%02d:%02d - %04d/%02d/%02d */
#else /* USE_RTC != 1 */
#define ECHOCLIENT_SND_RCV_MIN_SIZE               16U
#endif /* USE_RTC == 1 */

#define ECHOCLIENT_PERF_MIN_ITER                  10U
#define ECHOCLIENT_PERF_MAX_ITER                 255U

#define ECHOCLIENT_SND_RCV_TIMEOUT             20000U /* Timeout to send/receive data in ms */

/* Error counter implementation : nb of consecutive errors before to start NFM feature */
#define ECHOCLIENT_NFM_ERROR_LIMIT_SHORT_MAX        5U

/* By default Echo client enabled after boot */
#define ECHOCLIENT_DEFAULT_ENABLED    ((uint8_t*)"1")
/* Size max of the enable value e.g "0" or "1" */
#define ECHOCLIENT_ENABLED_SIZE_MAX  (uint8_t)(2U)

/* Number of default Echo client parameters */
#define ECHOCLIENT_DEFAULT_PARAM_NB   1U

#if (USE_DEFAULT_SETUP == 0)
#define ECHOCLIENT_SETUP_VERSION      1U
#endif /* USE_DEFAULT_SETUP == 0 */

/* Private macro -------------------------------------------------------------*/
#define ECHOCLIENT_MIN(a,b) (((a) < (b))  ? (a) : (b))
#define ECHOCLIENT_MAX(a,b) (((a) >= (b)) ? (a) : (b))

/* Define to access or modify an IP */
#define ECHOCLIENT_GET_DISTANTIP(a) ((a).addr)
#define ECHOCLIENT_SET_DISTANTIP(a, b) ((a)->addr = (b))
#define ECHOCLIENT_SET_DISTANTIP_NULL(a) ((a)->addr = (uint32_t)0U)

/* Private variables ---------------------------------------------------------*/
/* When a callback function is called,
 * a message is send to the queue to ask echo thread to treat the event */
/* Used to send 'network is up' indication */
static osMessageQId echoclient_queue;

/* Echo Distant server type */
static echoclient_distant_server_t echoclient_distant_server;
/* Echo Distant new server type requested */
static echoclient_distant_server_t echoclient_distant_new_server;
/* Echo Distant name to contact */
static uint8_t *echoclient_distantname;
/* Echo Distant IP to contact */
#if (USE_NETWORK_LIBRARY == 1)
static net_ip_addr_t echoclient_distantip;
#else /* USE_NETWORK_LIBRARY == 0 */
static com_ip_addr_t echoclient_distantip;
#endif /* USE_NETWORK_LIBRARY == 1 */
/* Echo Distant Port to contact */
static uint16_t echoclient_distantport;
/* Echo Distant Port to contact to obtain date and time */
static uint16_t echoclient_distant_date_time_port;
/* Pointer on potential welcome message send by the server
 * If = NULL no welcome message is expected */
static uint8_t *echoclient_distant_tcp_welcome_msg; /* tcp welcome message */
static uint8_t *echoclient_distant_udp_welcome_msg; /* udp welcome message */
static uint8_t *echoclient_distant_welcome_msg;     /* tcp or udp welcome message according to socket protocol */

/* socket_desc[0]: use for periodic process, socket_desc[1]: use for performance test */
static echoclient_socket_desc_t echoclient_socket_desc[2];

/* Protocol used */
static echoclient_socket_protocol_t echoclient_socket_protocol;
/* New protocol requested */
static echoclient_socket_protocol_t echoclient_socket_new_protocol;

/* Buffer to send the request to the distant server */
static uint8_t    echoclient_snd_buffer[ECHOCLIENT_SND_RCV_MAX_SIZE];
/* Buffer to store the response of the distant server */
static uint8_t    echoclient_rcv_buffer[ECHOCLIENT_SND_RCV_MAX_SIZE];
/* Size of the buffer to send
 * in an echo feature size to receive = size which was send */
static uint16_t   echoclient_snd_rcv_buffer_size;

/* NFM feature variables */
/* Limit nb of errors before to activate NFM */
static uint8_t echoclient_nfm_nb_error_limit_short;
/* Current nb of errors in NFM feature */
static uint8_t echoclient_nfm_nb_error_short;
/* Sleep timer index in the NFM array */
static uint8_t echoclient_nfm_sleep_timer_index;

/* Current status of echo process */
static bool echoclient_process_flag; /* false: inactive, true: active */
/* Requested status of echo process */
static bool echoclient_next_process_flag; /* false: inactive, true: active */
/* Current status of echo performance */
static bool echoclient_perf_start; /* false: inactive, true: active */
/* Performance iteration number */
static uint16_t echoclient_perf_iter_nb;

/* Period of echo processing */
static uint32_t echoclient_processing_period;

/* State of Network connection */
static bool echoclient_network_is_on; /* false: network is off,
                                         true:  network is on,
                                         default value : see echoclient_init() */

#if (USE_RTC == 1)
/* Set or not Date/Time => a specific request will be send to the echo distant server */
static bool echoclient_set_date_time; /* false: date time has not to be requested,
                                         true:  date time has to be requested,
                                         default value : see echoclient_init() */
#endif /* (USE_RTC == 1) */

/* Echo client statistics */
static echoclient_stat_t echoclient_stat;

/* String used to display server */
static uint8_t *echoclient_distant_server_string[ECHOCLIENT_SERVER_MAX] =
{
  (uint8_t *)"MBED",
  (uint8_t *)"UBLOX",
  (uint8_t *)"LOCAL"
};

/* String used to display the protocol */
static uint8_t *echoclient_protocol_string[ECHOCLIENT_SOCKET_PROTO_MAX] =
{
  (uint8_t *)"TCP",
  (uint8_t *)"UDP mode connected",
  (uint8_t *)"UDP mode not-connected"
};

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Callback */
#if (USE_NETWORK_LIBRARY == 1)
static void echoclient_net_notify(void *p_context, uint32_t event_class, uint32_t event_id, void *p_event_data);
#else /* USE_NETWORK_LIBRARY == 0 */
static void echoclient_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);
#endif /* USE_NETWORK_LIBRARY == 1 */

static void echoclient_check_distantname(void);
static void echoclient_distant_server_set(echoclient_distant_server_t distant_server);
static bool echoclient_obtain_socket(echoclient_socket_desc_t *p_socket);
static void echoclient_close_socket(echoclient_socket_desc_t *p_socket);
static uint16_t echoclient_format_buffer(uint16_t length);
static bool echoclient_process(echoclient_socket_desc_t *p_socket,
                               const uint8_t *p_buf_snd, uint8_t *p_buf_rsp, uint32_t *p_snd_rcv_time);
static void echoclient_performance_iteration(echoclient_socket_desc_t *p_socket,
                                             uint16_t iteration_nb, uint16_t trame_size,
                                             echoclient_performance_result_t *p_perf_result);
static void echoclient_performance(echoclient_socket_desc_t *p_socket);
static void echoclient_socket_thread(void *p_argument);
static bool echoclient_is_nfm_sleep_requested(void);
static uint32_t echoclient_get_nfmc(uint8_t index);
#if (USE_RTC == 1)
static void echoclient_obtain_date_time(echoclient_socket_desc_t *p_socket);
static void echoclient_analyze_date_time_str(uint8_t rcv_len, uint8_t const *p_rcv);
#endif /* (USE_RTC == 1) */
#if (USE_CMD_CONSOLE == 1)
static void echoclient_cmd_help(void);
static cmd_status_t echoclient_cmd(uint8_t *p_cmd_line);
#endif /* USE_CMD_CONSOLE == 1 */

/* Private functions ---------------------------------------------------------*/

#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  help cmd management
  * @param  -
  * @retval -
  */
static void echoclient_cmd_help(void)
{
  /* Display information about echo cmd and its supported parameters */
  CMD_print_help((uint8_t *)"echoclient");
  PRINT_FORCE("echoclient help")
  PRINT_FORCE("echoclient on         : enable echo client process")
  PRINT_FORCE("echoclient off        : disable echo client process")
  PRINT_FORCE("echoclient period <n> : set the processing period to n (ms)")
  PRINT_FORCE("echoclient size <n>   : set buffer size to n (bytes)")

  /* UDP not-connected mode supported
   * always in LwIP
   * in Modem only if UDP_SERVICE_SUPPORTED == 1U */
#if ((USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) && (UDP_SERVICE_SUPPORTED == 0U))
  PRINT_FORCE("echoclient protocol [TCP|UDP] : set socket protocol to:")
  PRINT_FORCE("                               TCP|UDP: connected mode")
#else
  PRINT_FORCE("echoclient protocol [TCP|UDP|UDPSERVICE] : set protocol to:")
  PRINT_FORCE("                                           TCP|UDP: connected mode")
  PRINT_FORCE("                                           UDPSERVICE: UDP not-connected mode")
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) && (UDP_SERVICE_SUPPORTED == 0U) */
  PRINT_FORCE("echoclient status     : display Server Name, IP and Port,")
  PRINT_FORCE("                                Protocol, Period, Size value and Echoclt state")

  PRINT_FORCE("echoclient server   [%s|%s|%s] : set distant Server to %s|%s|%s",
              echoclient_distant_server_string[ECHOCLIENT_MBED_SERVER],
              echoclient_distant_server_string[ECHOCLIENT_UBLOX_SERVER],
              echoclient_distant_server_string[ECHOCLIENT_LOCAL_SERVER],
              echoclient_distant_server_string[ECHOCLIENT_MBED_SERVER],
              echoclient_distant_server_string[ECHOCLIENT_UBLOX_SERVER],
              echoclient_distant_server_string[ECHOCLIENT_LOCAL_SERVER])
  PRINT_FORCE("echoclient perf       : performance snd/rcv test with default iterations")
  PRINT_FORCE("echoclient perf <n>   : performance snd/rcv test with only n iterations")
  PRINT_FORCE("echoclient stat       : display statistic")
  PRINT_FORCE("echoclient stat reset : reset statistic")
}

/**
  * @brief  cmd management
  * @param  p_cmd_line   - pointer on command parameters
  * @retval cmd_status_t - status of cmd management CMD_OK or CMD_SYNTAX_ERROR
  */
static cmd_status_t echoclient_cmd(uint8_t *p_cmd_line)
{
  uint8_t len; /* length parameter */
  uint32_t argc;
  uint8_t  *p_argv[10];
  const uint8_t *p_cmd;
  int32_t  atoi_res;
  uint32_t total_ko; /* calcul number of echo ko */

  p_cmd = (uint8_t *)strtok((CRC_CHAR_t *)p_cmd_line, " \t");
  if (p_cmd != NULL)
  {
    len = (uint8_t)crs_strlen(p_cmd);

    /* Is it a command echoclient ? */
    if (memcmp((const CRC_CHAR_t *)p_cmd, "echoclient", len) == 0)
    {
      /* Parameters parsing */
      for (argc = 0U; argc < 10U; argc++)
      {
        p_argv[argc] = (uint8_t *)strtok(NULL, " \t");
        if (p_argv[argc] == NULL)
        {
          break;
        }
      }

      /* No parameters ? */
      if (argc == 0U)
      {
        /* Display help */
        echoclient_cmd_help();
      }

      /*  At least one parameter provided */
      else
      {
        len = (uint8_t)crs_strlen(p_argv[0]);
        if (memcmp((CRC_CHAR_t *)p_argv[0], "help", len) == 0)
        {
          /* cmd 'echo help': display help */
          echoclient_cmd_help();
        }
        else if (echoclient_perf_start == true)
        {
          PRINT_FORCE("<<< Echoclt Performance in progress... Please wait its end!>>>")
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "on", len) == 0)
        {
          /* cmd 'echo on': start echo if not already started */
          if (echoclient_process_flag == false)
          {
            PRINT_FORCE("<<< Echoclt Start requested ...>>>")
          }
          else
          {
            PRINT_FORCE("<<< Echoclt already started ...>>>")
          }
          echoclient_next_process_flag = true;
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "off", len) == 0)
        {
          /* cmd 'echo off': stop echo if previously started */
          if ((echoclient_process_flag == true)
              || (echoclient_next_process_flag == true))
          {
            PRINT_FORCE("<<< Echoclt Stop in progress ...>>>")
            if ((echoclient_process_flag == false)
                && (echoclient_next_process_flag == true))
            {
              echoclient_next_process_flag = false;
              PRINT_FORCE("<<< Echoclt Start cancelled>>>")
            }
          }
          else
          {
            PRINT_FORCE("<<< Echoclt already stopped ...>>>")
          }
          echoclient_next_process_flag = false;
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "perf", len) == 0)
        {
          /* cmd 'echo perf ...': start performance snd/rcv test */
          /* Echoclient must be off before */
          if ((echoclient_process_flag == true)
              || (echoclient_next_process_flag == true))
          {
            if (echoclient_next_process_flag == true)
            {
              PRINT_FORCE("Echoclt is running or is starting. Please stop it before!")
            }
            else
            {
              PRINT_FORCE("Echoclt stop requested. Please wait socket is closing!")
            }
          }
          else if (echoclient_perf_start == true)
          {
            PRINT_FORCE("Echoclt performance test already started!")
          }
          else /* Echoclient state ok to continue */
          {
            /* cmd 'echo perf' : start performance snd/rcv test with the array iterations */
            if (argc == 1U)
            {
              echoclient_perf_iter_nb = 0U;
              PRINT_FORCE("<<< Echoclt Performance started ...>>>")
              echoclient_perf_start = true;
            }
            else
            {
              /* cmd 'echo perf <n>': start performance snd/rcv test with n iterations */
              atoi_res = crs_atoi(p_argv[1]);
              if ((atoi_res > 0)
                  && (atoi_res >= ((int32_t)ECHOCLIENT_PERF_MIN_ITER))
                  && (atoi_res <= ((int32_t)(ECHOCLIENT_PERF_MAX_ITER))))
              {
                echoclient_perf_iter_nb = (uint8_t)atoi_res;
                PRINT_FORCE("<<< Echoclt Performance started ...>>>")
                echoclient_perf_start = true;
              }
              else
              {
                /* Display a reminder about iter [min,max] */
                PRINT_FORCE("Echoclt: parameter iter must be [%d,%d]",
                            ECHOCLIENT_PERF_MIN_ITER, ECHOCLIENT_PERF_MAX_ITER)
              }
            }
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "size", len) == 0)
        {
          /* cmd 'echo size xxx': configure size of message to send */
          if (argc == 2U)
          {
            atoi_res = crs_atoi(p_argv[1]);
            /* res must be a number >= minimal size */
            /* reserve one byte for '\0' */
            if ((atoi_res > 0)
                && (atoi_res >= ((int32_t)ECHOCLIENT_SND_RCV_MIN_SIZE))
                && (atoi_res <= ((int32_t)(ECHOCLIENT_SND_RCV_MAX_SIZE - 1U))))
            {
              echoclient_snd_rcv_buffer_size = (uint16_t)atoi_res;
              PRINT_FORCE("Echoclt: new trame size:%d", echoclient_snd_rcv_buffer_size)
            }
            else
            {
              /* Display a reminder about size min, size max */
              PRINT_FORCE("Echoclt: parameter n must be [%d,%d]",
                          ECHOCLIENT_SND_RCV_MIN_SIZE, (ECHOCLIENT_SND_RCV_MAX_SIZE - 1U))
            }
          }
          else
          {
            /* Display a reminder about size min, size max */
            PRINT_FORCE("Echoclt: parameter n must be provided [%d,%d]",
                        ECHOCLIENT_SND_RCV_MIN_SIZE, (ECHOCLIENT_SND_RCV_MAX_SIZE - 1U))
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "period", len) == 0)
        {
          /* cmd 'echo period xxx': configure period between two sends */
          if (argc == 2U)
          {
            atoi_res = crs_atoi(p_argv[1]);
            /* res must be a number */
            if (atoi_res > 0)
            {
              echoclient_processing_period = (uint32_t)atoi_res;
              PRINT_FORCE("Echoclt: new processing period: %ld", echoclient_processing_period)
            }
            else
            {
              PRINT_FORCE("Echoclt: parameter 'period' must be > 0")
            }
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "protocol", len) == 0)
        {
          /* cmd 'echo protocol xxx': configure protocol used to address the echo server */
          if (argc == 2U)
          {
            /* Only one modification at a time */
            if (echoclient_socket_new_protocol != echoclient_socket_protocol)
            {
              PRINT_FORCE("rEchoclt: a protocol change already in progress. Please wait ...")
            }
            else
            {
              echoclient_socket_protocol_t echoclient_socket_tmp_protocol;

              /* Check protocol value */
              len = (uint8_t)crs_strlen(p_argv[1]);

              if (memcmp((CRC_CHAR_t *)p_argv[1], "TCP", len) == 0)
              {
                echoclient_socket_tmp_protocol = ECHOCLIENT_SOCKET_TCP_PROTO;
              }
              else if (memcmp((CRC_CHAR_t *)p_argv[1], "UDP", len) == 0)
              {
                echoclient_socket_tmp_protocol = ECHOCLIENT_SOCKET_UDP_PROTO;
              }
              else if (memcmp((CRC_CHAR_t *)p_argv[1], "UDPSERVICE", len) == 0)
              {
                /* UDP not-connected mode supported:
                 * in LwIP always
                 * in Modem only if (UDP_SERVICE_SUPPORTED == 1U) */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#if (UDP_SERVICE_SUPPORTED == 1U)
                echoclient_socket_tmp_protocol = ECHOCLIENT_SOCKET_UDP_SERVICE_PROTO;
#else /* UDP_SERVICE_SUPPORTED == 0U */
                /* Set Socket protocol server to an illegal value */
                echoclient_socket_tmp_protocol = ECHOCLIENT_SOCKET_PROTO_MAX;
#endif /* UDP_SERVICE_SUPPORTED == 1U */
#else /* USE_SOCKETS_TYPE != USE_SOCKETS_MODEM */
                echoclient_socket_tmp_protocol = ECHOCLIENT_SOCKET_UDP_SERVICE_PROTO;
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
              }
              else /* Protocol parameter invalid */
              {
                /* Set Socket protocol server to an illegal value */
                echoclient_socket_tmp_protocol = ECHOCLIENT_SOCKET_PROTO_MAX;
              }

              if (echoclient_socket_tmp_protocol != ECHOCLIENT_SOCKET_PROTO_MAX)
              {
                if (echoclient_socket_tmp_protocol != echoclient_socket_protocol)
                {
                  PRINT_FORCE("Echoclt: protocol set to %s in progress ...",
                              echoclient_protocol_string[echoclient_socket_tmp_protocol])
                  echoclient_socket_new_protocol = echoclient_socket_tmp_protocol;
                  if ((echoclient_socket_desc[0].state == ECHOCLIENT_SOCKET_INVALID)
                      && (echoclient_socket_desc[1].state == ECHOCLIENT_SOCKET_INVALID))
                  {
                    echoclient_socket_protocol = echoclient_socket_new_protocol;
                    /* Protocol change as no impact on distant server parameters */

                    PRINT_FORCE("Echoclt: protocol set to %s",
                                echoclient_protocol_string[echoclient_socket_tmp_protocol])
                  }
                }
                else
                {
                  PRINT_FORCE("Echoclt: socket protocol already %s!",
                              echoclient_protocol_string[echoclient_socket_protocol])
                }
              }
              else
              {
#if ((USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) && (UDP_SERVICE_SUPPORTED == 0U))
                PRINT_FORCE("Echoclt: protocol parameter possible value [TCP|UDP]")
                PRINT_FORCE("         TCP|UDP: mode connected")
#else
                PRINT_FORCE("Echoclt: protocol parameter possible value [TCP|UDP|UDPSERVICE]")
                PRINT_FORCE("         TCP|UDP: mode connected")
                PRINT_FORCE("         UDPSERVICE: mode not-connected)")
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) && (UDP_SERVICE_SUPPORTED == 0U) */
              }
            }
          }
          else /* protocol not provided */
          {
#if ((USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) && (UDP_SERVICE_SUPPORTED == 0U))
            PRINT_FORCE("Echoclt: protocol parameter must be provided [TCP|UDP]")
            PRINT_FORCE("         TCP|UDP: mode connected")
#else
            PRINT_FORCE("Echoclt: protocol parameter must be provided [TCP|UDP|UDPSERVICE]")
            PRINT_FORCE("         TCP|UDP: mode connected")
            PRINT_FORCE("         UDPSERVICE: mode not-connected)")
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) && (UDP_SERVICE_SUPPORTED == 0U) */
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "server", len) == 0)
        {
          /* cmd 'echo server xxx': configure server used */
          if (argc == 2U)
          {
            /* Only one modification at a time */
            if (echoclient_distant_new_server != echoclient_distant_server)
            {
              PRINT_FORCE("Echoclt: a distant server change already in progress. Please wait ...")
            }
            else
            {
              echoclient_distant_server_t echoclient_distant_tmp_server;

              /* Check server value */
              len = (uint8_t)crs_strlen(p_argv[1]);

              if (memcmp((CRC_CHAR_t *)p_argv[1], echoclient_distant_server_string[ECHOCLIENT_MBED_SERVER], len)
                  == 0)
              {
                echoclient_distant_tmp_server = ECHOCLIENT_MBED_SERVER;
              }
              else if (memcmp((CRC_CHAR_t *)p_argv[1], echoclient_distant_server_string[ECHOCLIENT_UBLOX_SERVER], len)
                       == 0)
              {
                echoclient_distant_tmp_server = ECHOCLIENT_UBLOX_SERVER;
              }
              else if (memcmp((CRC_CHAR_t *)p_argv[1], echoclient_distant_server_string[ECHOCLIENT_LOCAL_SERVER], len)
                       == 0)
              {
                echoclient_distant_tmp_server = ECHOCLIENT_LOCAL_SERVER;
              }
              else /* Server parameter invalid */
              {
                /* Set Distant server to a illegal value */
                echoclient_distant_tmp_server = ECHOCLIENT_SERVER_MAX;
              }

              if (echoclient_distant_tmp_server != ECHOCLIENT_SERVER_MAX)
              {
                if (echoclient_distant_tmp_server != echoclient_distant_server)
                {
                  PRINT_FORCE("Echoclt: server set to %s in progress ...",
                              echoclient_distant_server_string[echoclient_distant_tmp_server])
                  echoclient_distant_new_server = echoclient_distant_tmp_server;
                  if ((echoclient_socket_desc[0].state == ECHOCLIENT_SOCKET_INVALID)
                      && (echoclient_socket_desc[1].state == ECHOCLIENT_SOCKET_INVALID))
                  {
                    echoclient_distant_server = echoclient_distant_tmp_server;
                    /* Update distant server parameters */
                    echoclient_distant_server_set(echoclient_distant_server);

                    PRINT_FORCE("Echoclt: server set to %s",
                                echoclient_distant_server_string[echoclient_distant_tmp_server])
                  }
                }
                else
                {
                  PRINT_FORCE("Echoclt: distant server already on %s!",
                              echoclient_distant_server_string[echoclient_distant_server])
                }
              }
              else
              {
                PRINT_FORCE("Echoclt: server parameter possible values [%s|%s|%s]",
                            echoclient_distant_server_string[ECHOCLIENT_MBED_SERVER],
                            echoclient_distant_server_string[ECHOCLIENT_UBLOX_SERVER],
                            echoclient_distant_server_string[ECHOCLIENT_LOCAL_SERVER])
              }
            }
          }
          else /* protocol not provided */
          {
            PRINT_FORCE("Echoclt: server parameter must be provided [%s|%s|%s]",
                        echoclient_distant_server_string[ECHOCLIENT_MBED_SERVER],
                        echoclient_distant_server_string[ECHOCLIENT_UBLOX_SERVER],
                        echoclient_distant_server_string[ECHOCLIENT_LOCAL_SERVER])
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "stat", len) == 0)
        {
          if (argc == 1U)
          {
            /* cmd 'echo stat' : display echo statistics */
            PRINT_FORCE("<<< Being Echoclt Statistics >>>\r\n")
            total_ko =  echoclient_stat.connect.ko + echoclient_stat.close.ko
                        + echoclient_stat.send.ko + echoclient_stat.receive.ko;

            PRINT_FORCE("loop count:%ld ok:%ld ko:%ld", echoclient_stat.count, echoclient_stat.receive.ok, total_ko)
            PRINT_FORCE("connect:       ok:%ld ko:%ld", echoclient_stat.connect.ok, echoclient_stat.connect.ko)
            PRINT_FORCE("send   :       ok:%ld ko:%ld", echoclient_stat.send.ok, echoclient_stat.send.ko)
            PRINT_FORCE("receive:       ok:%ld ko:%ld", echoclient_stat.receive.ok, echoclient_stat.receive.ko)
            PRINT_FORCE("close  :       ok:%ld ko:%ld", echoclient_stat.close.ok, echoclient_stat.close.ko)
            PRINT_FORCE("<<< End   Echoclt Statistics >>>\r\n")
          }
          else if (memcmp((CRC_CHAR_t *)p_argv[1], "reset", len) == 0)
          {
            /* cmd 'echo stat reset' : reset echo statistics */
            (void)memset((void *)&echoclient_stat, 0, sizeof(echoclient_stat));
            PRINT_FORCE("Echoclt: statistics reset")
          }
          else
          {
            PRINT_FORCE("Echoclt: only 'stat' or 'stat reset' is supported")
          }
        }
        /* status must be after stat because len = length of the input */
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "status", len) == 0)
        {
          /* cmd 'echo status' : display status of echo
           * Distant Server Name and IP Port
           * Protocol - current and requested
           * Period between two send
           * Size of buffer send
           */
          PRINT_FORCE("<<< Begin Echoclient Status >>>")
          /* Is Distant Server IP known ? */
          if (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) == 0U)
          {
            /* Distant Server IP unknown */
            PRINT_FORCE("Distant server:%s Name:%s IP:Unknown Port:%d",
                        echoclient_distant_server_string[echoclient_distant_server],
                        echoclient_distantname, echoclient_distantport)
          }
          else
          {
            /* Distant Server IP known */
#if (USE_NETWORK_LIBRARY == 1)
            PRINT_FORCE("Distant server:%s Name:%s IP:%ld.%ld.%ld.%ld Port:%d",
                        echoclient_distant_server_string[echoclient_distant_server], echoclient_distantname,
                        (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x000000FFU) >> 0),
                        (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x0000FF00U) >> 8),
                        (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x00FF0000U) >> 16),
                        (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0xFF000000U) >> 24),
                        echoclient_distantport)
#else /* USE_NETWORK_LIBRARY == 0 */
            PRINT_FORCE("Distant server:%s Name:%s IP:%d.%d.%d.%d Port:%d",
                        echoclient_distant_server_string[echoclient_distant_server], echoclient_distantname,
                        COM_IP4_ADDR1(&echoclient_distantip), COM_IP4_ADDR2(&echoclient_distantip),
                        COM_IP4_ADDR3(&echoclient_distantip), COM_IP4_ADDR4(&echoclient_distantip),
                        echoclient_distantport)
#endif /* USE_NETWORK_LIBRARY == 1 */
          }
          /* Actual Protocol - Period and Size parameters displayed */
          PRINT_FORCE("Parameters: Protocol:%s Period:%ldms Size:%dbytes",
                      echoclient_protocol_string[echoclient_socket_protocol],
                      echoclient_processing_period, echoclient_snd_rcv_buffer_size)
          /* Requested Server displayed */
          if (echoclient_distant_server != echoclient_distant_new_server)
          {
            PRINT_FORCE("Distant server change in progress New value:%s",
                        echoclient_distant_server_string[echoclient_distant_new_server])
          }
          /* Requested Protocol displayed */
          if (echoclient_socket_protocol != echoclient_socket_new_protocol)
          {
            PRINT_FORCE("Protocol change in progress New value:%s",
                        echoclient_protocol_string[echoclient_socket_new_protocol])
          }
          /* State */
          if (echoclient_process_flag == true)
          {
            if (echoclient_next_process_flag == false)
            {
              PRINT_FORCE("Status: Running but Stop requested")
            }
            else
            {
              PRINT_FORCE("Status: Running")
            }
          }
          else
          {
            if (echoclient_next_process_flag == true)
            {
              PRINT_FORCE("Status: Start requested")
            }
            else
            {
              PRINT_FORCE("Status: Stop")
            }
          }
          PRINT_FORCE("<<< End   Echoclient Status >>>")
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "valid", len) == 0)
        {
          if (argc == 2U)
          {
            /* cmd 'echo valid xxx': use for automatic test to obtain the statistics */
            len = (uint8_t)crs_strlen(p_argv[1]);
            if (memcmp((CRC_CHAR_t *)p_argv[1], "stat", len) == 0)
            {
              /* cmd 'echo valid stat': use for automatic test to obtain the statistics */
              total_ko =  echoclient_stat.connect.ko + echoclient_stat.close.ko
                          + echoclient_stat.send.ko + echoclient_stat.receive.ko;
              /* Display is trace validation type */
              TRACE_VALID("@valid@:echo:stat:%ld/%ld\n\r", echoclient_stat.receive.ok, total_ko)
            }
            else
            {
              TRACE_VALID("@valid@:echo:Unrecognized parameter '%s'\n\r", (CRC_CHAR_t *)p_argv[1])
            }
          }
          else
          {
            TRACE_VALID("@valid@:echo:Incorrect syntax\n\r")
          }
        }

        else /* cmd 'echo xxx' with a param xxx not supported */
        {
          PRINT_FORCE("Echoclt: Unrecognised command. Usage:")
          /* Display help to remind command supported */
          echoclient_cmd_help();
        }
      }
    }
  }
  else
  {
    /* Display help to remind command supported */
    PRINT_FORCE("Echoclt: Unrecognised command. Usage:")
    echoclient_cmd_help();
  }
  return CMD_OK;
}
#endif /* USE_CMD_CONSOLE == 1 */

#if (USE_NETWORK_LIBRARY == 1)
/**
  * @brief  Callback called when a value in connect library change
  * @note   Used to know network status
  * @param  p_context - net_if_handle_t pointer
  * @param  event_class - event class
  * @param  event_id - event id
  * @param  p_event_data - event data pointer
  * @retval -
  */
void echoclient_net_notify(void *p_context, uint32_t event_class, uint32_t event_id, void  *p_event_data)
{
  /* net_if_handle_t *netif=context; */ /* Check if netif is Cellular is NET_INTERFACE_CLASS_CELLULAR */
  UNUSED(p_context);
  UNUSED(p_event_data);

  /* Is the event about a network state change ? */
  if ((uint32_t)NET_EVENT_STATE_CHANGE == event_class)
  {
    net_state_t new_state = (net_state_t)event_id;

    /* Network connected ? */
    if (new_state == NET_STATE_CONNECTED)
    {
      PRINT_FORCE("Echo: Network is up\n\r")
      echoclient_network_is_on = true;
      /* Inform Echo thread that network is on */
      (void)rtosalMessageQueuePut(echoclient_queue, (uint32_t)event_id, 0U);
    }
    else
    {
      PRINT_FORCE("Echo: Network is down\n\r")
      echoclient_network_is_on = false;
    }
  }
}
#else /* USE_NETWORK_LIBRARY == 0 */
/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @param  p_private_gui_data - pointer on value provided at service subscription
  * @note   Unused parameter
  * @retval -
  */
static void echoclient_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);

  /* Event to know Network status ? */
  if (dc_event_id == DC_CELLULAR_NIFMAN_INFO)
  {
    dc_nifman_info_t  dc_nifman_info;
    /* Is Network up or down ? */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO,
                      (void *)&dc_nifman_info,
                      sizeof(dc_nifman_info));

    if (dc_nifman_info.rt_state == DC_SERVICE_ON)
    {
      /* Network is up */
      PRINT_FORCE("Echo: Network is up\n\r")
      echoclient_network_is_on = true;
      /* Inform Echo thread that network is on */
      (void)rtosalMessageQueuePut(echoclient_queue, (uint32_t)dc_event_id, 0U);
    }
    else
    {
      PRINT_FORCE("Echo: Network is down\n\r")
      echoclient_network_is_on = false;
    }
  }
}
#endif /* USE_NETWORK_LIBRARY == 1 */

/**
  * @brief  Check if NFM sleep has to be done
  * @param  -
  * @retval bool - false/true NFM sleep hasn't to be done/has to be done
  */
static bool echoclient_is_nfm_sleep_requested(void)
{
  bool result;

  if (echoclient_nfm_nb_error_short >= echoclient_nfm_nb_error_limit_short)
  {
    result = true;
  }
  else
  {
    result = false;
  }

  return result;
}

/**
  * @brief  Check Distant name
  * @note   Decide if Network DNS resolver has to be called
  * @param  -
  * @retval -
  */
static void echoclient_check_distantname(void)
{
  /* If distantname is provided and distantip is unknown
     call DNS network resolution service */
  if ((crs_strlen(echoclient_distantname) > 0U)
      && (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) == 0U))
  {
    /* DNS network resolution request */
    PRINT_INFO("Distant Name provided %s. DNS resolution started", echoclient_distantname)
#if (USE_NETWORK_LIBRARY == 1)
    net_sockaddr_t distantaddr;
    distantaddr.sa_len = (uint8_t)sizeof(net_sockaddr_t);

    if (net_if_gethostbyname(NULL, &distantaddr, (char_t *)echoclient_distantname)
        == NET_OK)
    {
      /* DNS resolution ok - save IP in echoclient_distantip */
      ECHOCLIENT_SET_DISTANTIP(&echoclient_distantip, ((net_sockaddr_in_t *)&distantaddr)->sin_addr.s_addr);
      PRINT_INFO("DNS resolution OK - Echo Remote IP: %ld.%ld.%ld.%ld",
                 (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x000000FFU) >> 0),
                 (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x0000FF00U) >> 8),
                 (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x00FF0000U) >> 16),
                 (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0xFF000000U) >> 24))
      /* No reset of nb_error_short wait to see if distant can be reached */
    }
#else /* USE_NETWORK_LIBRARY == 0 */
    com_sockaddr_t distantaddr;

    if (com_gethostbyname(echoclient_distantname, &distantaddr)
        == COM_SOCKETS_ERR_OK)
    {
      /* DNS resolution ok - save IP in echoclient_distantip */
      /* Even next test doesn't suppress MISRA Warnings */
      /*
            if ((distantaddr.sa_len == (uint8_t)sizeof(com_sockaddr_in_t))
                && ((distantaddr.sa_family == (uint8_t)COM_AF_UNSPEC)
                    || (distantaddr.sa_family == (uint8_t)COM_AF_INET)))
      */
      {
        ECHOCLIENT_SET_DISTANTIP(&echoclient_distantip, ((com_sockaddr_in_t *)&distantaddr)->sin_addr.s_addr);
        PRINT_INFO("DNS resolution OK - Echo Remote IP: %d.%d.%d.%d",
                   COM_IP4_ADDR1(&echoclient_distantip), COM_IP4_ADDR2(&echoclient_distantip),
                   COM_IP4_ADDR3(&echoclient_distantip), COM_IP4_ADDR4(&echoclient_distantip))
        /* No reset of nb_error_short wait to see if distant can be reached */
      }
    }
#endif /* USE_NETWORK_LIBRARY == 1 */
    else
    {
      /* DNS resolution ko - increase fault counters */
      echoclient_stat.connect.ko++;
      echoclient_nfm_nb_error_short++;
      PRINT_ERR("DNS resolution NOK. Will retry later")
    }
  }
}

/**
  * @brief  Update Echo Distant Server data
  * @param  distant_server - Distant server type
  * @retval -
  */
static void echoclient_distant_server_set(echoclient_distant_server_t distant_server)
{
  switch (distant_server)
  {
    case ECHOCLIENT_MBED_SERVER  :
      /* Distant Server type */
      echoclient_distant_server = distant_server;
      /* Distant Name */
      echoclient_distantname = ECHOCLIENT_DISTANT_MBED_NAME;
      /* Distant IP */
      /* Uncomment next line to by-pass DNS resolution */
      /* ECHOCLIENT_SET_DISTANTIP(&echoclient_distantip, ECHOCLIENT_DISTANT_MBED_IP); */
      /* Distant ip set to 0 to force DNS resolution */
      ECHOCLIENT_SET_DISTANTIP_NULL(&echoclient_distantip);
      /* Distant Port */
      echoclient_distantport = ECHOCLIENT_DISTANT_MBED_PORT;
      /* Distant Welcome Msg TCP */
      echoclient_distant_tcp_welcome_msg = ECHOCLIENT_DISTANT_MBED_TCP_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      echoclient_distant_udp_welcome_msg = ECHOCLIENT_DISTANT_MBED_UDP_WELCOME_MSG;
      /* Distant Date Time Port */
      echoclient_distant_date_time_port = ECHOCLIENT_DISTANT_MBED_DATE_TIME_PORT;
      break;

    case ECHOCLIENT_UBLOX_SERVER :
      /* Distant Server type */
      echoclient_distant_server = distant_server;
      /* Distant Name */
      echoclient_distantname = ECHOCLIENT_DISTANT_UBLOX_NAME;
      /* Distant IP */
      /* Uncomment next line to by-pass DNS resolution */
      /* ECHOCLIENT_SET_DISTANTIP(&echoclient_distantip, ECHOCLIENT_DISTANT_UBLOX_IP); */
      /* Distant ip set to 0 to force DNS resolution */
      ECHOCLIENT_SET_DISTANTIP_NULL(&echoclient_distantip);
      /* Distant Port */
      echoclient_distantport = ECHOCLIENT_DISTANT_UBLOX_PORT;
      /* Distant Welcome Msg TCP */
      echoclient_distant_tcp_welcome_msg = ECHOCLIENT_DISTANT_UBLOX_TCP_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      echoclient_distant_udp_welcome_msg = ECHOCLIENT_DISTANT_UBLOX_UDP_WELCOME_MSG;
      /* Distant Date Time Port */
      echoclient_distant_date_time_port = ECHOCLIENT_DISTANT_UBLOX_DATE_TIME_PORT;
      break;

    case ECHOCLIENT_LOCAL_SERVER :
      /* Distant Server type */
      echoclient_distant_server = distant_server;
      /* Distant Name */
      echoclient_distantname = ECHOCLIENT_DISTANT_LOCAL_NAME;
      /* Distant IP */
      /* Local server is unknown from DNS server - force IP address to its value to by pass DNS resolution */
      ECHOCLIENT_SET_DISTANTIP(&echoclient_distantip, ECHOCLIENT_DISTANT_LOCAL_IP);
      /* Distant Port */
      echoclient_distantport = ECHOCLIENT_DISTANT_LOCAL_PORT;
      /* Distant Welcome Msg TCP */
      echoclient_distant_tcp_welcome_msg = ECHOCLIENT_DISTANT_LOCAL_TCP_WELCOME_MSG;
      /* Distant Welcome Msg UDP */
      echoclient_distant_udp_welcome_msg = ECHOCLIENT_DISTANT_LOCAL_UDP_WELCOME_MSG;
      /* Distant Date Time Port */
      echoclient_distant_date_time_port = ECHOCLIENT_DISTANT_LOCAL_DATE_TIME_PORT;
      break;

    default:
      /* Should not happen */
      __NOP();
      break;
  }
}

/**
  * @brief  Close socket
  * @note   If requested close and create socket
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
static void echoclient_close_socket(echoclient_socket_desc_t *p_socket)
{
  if (p_socket->state != ECHOCLIENT_SOCKET_INVALID)
  {
#if (USE_NETWORK_LIBRARY == 1)
    if (net_closesocket(p_socket->id) != NET_OK)
#else /* USE_NETWORK_LIBRARY == 0 */
    if (com_closesocket(p_socket->id) != COM_SOCKETS_ERR_OK)
#endif /* USE_NETWORK_LIBRARY == 1 */
    {
      /* close socket ko - increase fault counters and put socket state in closing */
      echoclient_stat.close.ko++;
      p_socket->state = ECHOCLIENT_SOCKET_CLOSING;
      PRINT_ERR("socket close NOK")
    }
    else
    {
      /* close socket ok - increase counters and put socket state in invalid */
      echoclient_stat.close.ok++;
      p_socket->state = ECHOCLIENT_SOCKET_INVALID;
#if (USE_NETWORK_LIBRARY == 1)
      p_socket->id = -1; /* no invalid value defined */
#else /* USE_NETWORK_LIBRARY == 0 */
      p_socket->id = COM_SOCKET_INVALID_ID;
#endif /* USE_NETWORK_LIBRARY == 1 */
      PRINT_INFO("socket close OK")
    }
    p_socket->closing = false;
  }
}

/**
  * @brief  Obtain socket
  * @note   If requested close and create socket
  * @param  p_socket   - pointer on the socket to use
  * @retval true/false - socket creation OK/NOK
  */
static bool echoclient_obtain_socket(echoclient_socket_desc_t *p_socket)
{
  bool result;
  uint32_t timeout;

  /* close the socket if:
   - internal close is requested
   - or previous close request not ok
   - or if socket protocol has been changed and socket is still open and socket is opened
   - or if distant server  has been changed and socket is still open and socket is opened
   */
  if ((p_socket->closing == true)
      || (p_socket->state == ECHOCLIENT_SOCKET_CLOSING)
      || (((echoclient_socket_protocol != echoclient_socket_new_protocol)
           || (echoclient_distant_server != echoclient_distant_new_server))
          && (p_socket->state != ECHOCLIENT_SOCKET_INVALID)))
  {
    PRINT_INFO("socket in closing mode request the close")
    echoclient_close_socket(p_socket);
    result = false; /* if close nok - socket can no more be used
                       if close ok  - socket has to be created   */
  }
  else /* socket is already closed or can still be used */
  {
    result = true;
  }

  /* If socket is closed, create it */
  if (p_socket->state == ECHOCLIENT_SOCKET_INVALID)
  {
    /* socket need to be created */
    result = false;
    /* Update protocol value */
    echoclient_socket_protocol = echoclient_socket_new_protocol;

    /* Distant server need to be updated ? */
    if (echoclient_distant_server != echoclient_distant_new_server)
    {
      /* Update distant server value */
      echoclient_distant_server = echoclient_distant_new_server;
      /* Update distant server parameters */
      echoclient_distant_server_set(echoclient_distant_server);
    }

    /* If distantip to contact is unknown, call DNS resolver service */
    if (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) == (uint32_t)0U)
    {
      echoclient_check_distantname();
    }

    /* If distantip to contact is known, execute rest of the process */
    if (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) != (uint32_t)0U)
    {
      /* Create a socket */
      PRINT_INFO("socket creation in progress")

      if (echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
      {
#if (USE_NETWORK_LIBRARY == 1)
        p_socket->id = net_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
#else /* USE_NETWORK_LIBRARY == 0 */
        p_socket->id = com_socket(COM_AF_INET, COM_SOCK_STREAM, COM_IPPROTO_TCP);
#endif /* USE_NETWORK_LIBRARY == 1 */
        echoclient_distant_welcome_msg = echoclient_distant_tcp_welcome_msg;
      }
      else if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO)
               || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_SERVICE_PROTO))
      {
#if (USE_NETWORK_LIBRARY == 1)
        p_socket->id = net_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
#else /* USE_NETWORK_LIBRARY == 0 */
        p_socket->id = com_socket(COM_AF_INET, COM_SOCK_DGRAM, COM_IPPROTO_UDP);
#endif /* USE_NETWORK_LIBRARY == 1 */
        echoclient_distant_welcome_msg = echoclient_distant_udp_welcome_msg;
      }
      else
      {
#if (USE_NETWORK_LIBRARY == 1)
        p_socket->id = -1; /* no invalid value defined */
#else /* USE_NETWORK_LIBRARY == 0 */
        p_socket->id = COM_SOCKET_INVALID_ID;
#endif /* USE_NETWORK_LIBRARY == 1 */
      }

      if (p_socket->id >= 0) /* no invalid value defined in network library */
      {
        /* Socket created, continue the process */
        PRINT_INFO("socket create OK")
        /* Call rcvtimeout and sdntimeout setsockopt */
        timeout = ECHOCLIENT_SND_RCV_TIMEOUT;

        PRINT_INFO("socket setsockopt in progress")
#if (USE_NETWORK_LIBRARY == 1)
        if (net_setsockopt(p_socket->id, NET_SOL_SOCKET, NET_SO_RCVTIMEO, &timeout, (int32_t)sizeof(timeout))
            == NET_OK)
#else /* USE_NETWORK_LIBRARY == 0 */
        if (com_setsockopt(p_socket->id, COM_SOL_SOCKET, COM_SO_RCVTIMEO, &timeout, (int32_t)sizeof(timeout))
            == COM_SOCKETS_ERR_OK)
#endif /* USE_NETWORK_LIBRARY == 1 */
        {
#if (USE_NETWORK_LIBRARY == 1)
          if (net_setsockopt(p_socket->id, NET_SOL_SOCKET, NET_SO_SNDTIMEO, &timeout, (int32_t)sizeof(timeout))
              == COM_SOCKETS_ERR_OK)
#else /* USE_NETWORK_LIBRARY == 0 */
          if (com_setsockopt(p_socket->id, COM_SOL_SOCKET, COM_SO_SNDTIMEO, &timeout, (int32_t)sizeof(timeout))
              == COM_SOCKETS_ERR_OK)
#endif /* USE_NETWORK_LIBRARY == 1 */
          {
            p_socket->state = ECHOCLIENT_SOCKET_CREATED;
            PRINT_INFO("socket setsockopt OK")
          }
          else
          {
            PRINT_ERR("socket setsockopt SNDTIMEO NOK")
          }
        }
        else
        {
          PRINT_ERR("socket setsockopt RCVTIMEO NOK")
        }

        if (p_socket->state != ECHOCLIENT_SOCKET_CREATED)
        {
          /* Issue during socket creation - close socket to restart properly */
          PRINT_ERR("socket setsockopt NOK - close the socket")
          echoclient_close_socket(p_socket);
        }
      }
      else
      {
        PRINT_INFO("socket create NOK")
      }
    }

    /* According to the socket Protocol call or not the connect */
    if (p_socket->state == ECHOCLIENT_SOCKET_CREATED)
    {
      if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
          || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO))
      {
        /* Connect must be called */
#if (USE_NETWORK_LIBRARY == 1)
        net_sockaddr_in_t address;
        address.sin_family = (uint8_t)NET_AF_INET;
        address.sin_port   = NET_HTONS(echoclient_distantport);
        address.sin_addr.s_addr = ECHOCLIENT_GET_DISTANTIP(echoclient_distantip);

        PRINT_INFO("socket connect rqt")
        if (net_connect(p_socket->id, (net_sockaddr_t *)&address, (int32_t)sizeof(net_sockaddr_in_t))
            == NET_OK)
#else /* USE_NETWORK_LIBRARY == 0 */
        com_sockaddr_in_t address;
        address.sin_family      = (uint8_t)COM_AF_INET;
        address.sin_port        = COM_HTONS(echoclient_distantport);
        address.sin_addr.s_addr = ECHOCLIENT_GET_DISTANTIP(echoclient_distantip);

        PRINT_INFO("socket connect rqt")
        if (com_connect(p_socket->id, (com_sockaddr_t const *)&address, (int32_t)sizeof(com_sockaddr_in_t))
            == COM_SOCKETS_ERR_OK)
#endif /* USE_NETWORK_LIBRARY == 1 */
        {
          /* Connect is ok, reset nfm counters, increase counters and put socket state in connected */
          echoclient_nfm_nb_error_short = 0U;
          echoclient_nfm_sleep_timer_index = 0U;
          echoclient_stat.connect.ok++;
          p_socket->state = ECHOCLIENT_SOCKET_CONNECTED;
          result = true;
          PRINT_INFO("socket connect OK")
        }
        else
        {
          /* Connect is ko, increase fault counters */
          echoclient_nfm_nb_error_short++;
          echoclient_stat.connect.ko++;
          PRINT_ERR("socket connect NOK closing the socket")
          /* Issue during socket connection - close socket to restart properly */
          echoclient_close_socket(p_socket);
          /* maybe distantip is no more ok
             if distantname is known, force next time a DNS network resolution */
          if (crs_strlen(echoclient_distantname) > 0U)
          {
            PRINT_INFO("distant ip is reset to force a new DNS network resolution of distant url")
            ECHOCLIENT_SET_DISTANTIP_NULL(&echoclient_distantip);
          }
        }
      }
      else /* echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDPSERVICE_PROTO */
      {
        /* Connect is not needed */
        result = true;
      }
    }
  }

  return (result);
}

/**
  * @brief  Analyze Date and Time string from the distant server
  * @note   read date & time network and update internal date & time
  *         %02ld/%02ld/%04ld - %02ld:%02ld:%02ld
  * @param  rcv_len - length of string provided by the distant server
  * @param  rcv - string provided by the distant server
  * @retval -
  */
#if (USE_RTC == 1)
static void echoclient_analyze_date_time_str(uint8_t rcv_len, uint8_t const *p_rcv)
{
  /* Internal Date and Time must be ordered like Day, Mday Month Year Hour Minutes Seconds */
  /* eg: Mon, 17 May 2019 13:50:10 */
  /* Received answer is Mon May 17 13:50:10 2019 */
  /* Day:0 Mday:1 Month:2 Year:3 Time:4 */
  uint8_t distant_date_time_order_received[5] = {0, 2, 1, 4, 3};

  uint8_t argv[5];
  uint8_t len[5];
  uint8_t date_time[75];
  uint8_t offset;
  uint8_t i = 0U;
  uint8_t j = 0U;
  uint8_t start = 0U;
  bool next = true;

  PRINT_INFO("date and time buffer reception %s", p_rcv)
  while ((i < rcv_len) && (j < 5U))
  {
    if ((p_rcv[i] == (uint8_t)' ')
        || (p_rcv[i] == (uint8_t)'\n')
        || (p_rcv[i] == (uint8_t)'\r'))
    {
      if (next == false)
      {
        len[j] = i - start;
        j++;
      }
      next = true;
    }
    else
    {
      if (next == true)
      {
        argv[j] = i;
        next = false;
        start = i;
      }
    }
    i++;
  }

  /* Find enough parameter ? */
  if (j == 5U)
  {
    offset = 0U;
    for (uint8_t k = 0U; k < j; k++)
    {
      (void)memcpy(&date_time[offset],
                   &p_rcv[argv[distant_date_time_order_received[k]]],
                   len[distant_date_time_order_received[k]]);
      offset += len[distant_date_time_order_received[k]];
      if (k == 0U)
      {
        date_time[offset] = (uint8_t)',';
        offset++;
      }
      date_time[offset] = (uint8_t)' ';
      offset++;
    }
    PRINT_INFO("Update date and time...")
    if (timedate_set_from_http((uint8_t *)&date_time[0]) != TIMEDATE_STATUS_OK)
    {
      PRINT_INFO("Update date and time NOK")
    }
    else
    {
      PRINT_INFO("Update date and time OK")
    }
  }
  else
  {
    PRINT_INFO("Update date and time NOK - not enough information in rsp")
  }
}

/**
  * @brief  Obtain Date and Time from the distant server
  * @note   Open and Close a socket according to parameter
  *         read date & time network and update internal date & time
  *         %02ld/%02ld/%04ld - %02ld:%02ld:%02ld
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
static void echoclient_obtain_date_time(echoclient_socket_desc_t *p_socket)
{
  /* Is re-ordering date-time known ? */
  if (echoclient_distant_date_time_port != ECHOCLIENT_DISTANT_UNKNOWN_DATE_TIME_PORT)
  {
    int32_t ret;

    /* If distantip to contact is unknown, call DNS resolver service */
    if (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) == (uint32_t)0U)
    {
      echoclient_check_distantname();
    }

    /* If distantip is known, send the request to obtain the dat time */
    if (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) != (uint32_t)0U)
    {
      bool connected;
      connected = false;

      /* Create socket */
#if (USE_NETWORK_LIBRARY == 1)
      p_socket->id = net_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
#else /* USE_NETWORK_LIBRARY == 0 */
      p_socket->id = com_socket(COM_AF_INET, COM_SOCK_STREAM, COM_IPPROTO_TCP);
#endif /* USE_NETWORK_LIBRARY == 1 */

      /* Connect to the distant server */
      if (p_socket->id >= 0)
      {
#if (USE_NETWORK_LIBRARY == 1)
        net_sockaddr_in_t address;
        address.sin_family      = (uint8_t)NET_AF_INET;
        address.sin_port        = NET_HTONS(echoclient_distant_date_time_port);
        address.sin_addr.s_addr = ECHOCLIENT_GET_DISTANTIP(echoclient_distantip);

        if (net_connect(p_socket->id, (net_sockaddr_t *)&address, (int32_t)sizeof(net_sockaddr_in_t))
            == NET_OK)
#else /* USE_NETWORK_LIBRARY == 0 */
        com_sockaddr_in_t address;
        address.sin_family      = (uint8_t)COM_AF_INET;
        address.sin_port        = COM_HTONS(echoclient_distant_date_time_port);
        address.sin_addr.s_addr = ECHOCLIENT_GET_DISTANTIP(echoclient_distantip);

        if (com_connect(p_socket->id, (com_sockaddr_t const *)&address, (int32_t)sizeof(com_sockaddr_in_t))
            == COM_SOCKETS_ERR_OK)
#endif /* USE_NETWORK_LIBRARY == 1 */
        {
          /* Connection is ok */
          connected = true;
        }
      }

      /* Is Connection ok ? */
      if (connected == true)
      {
        /* Send a trame e.g time to receive the date and time from the distant server */
#if (USE_NETWORK_LIBRARY == 1)
        ret = net_send(p_socket->id, (uint8_t *)"time", 4, 0x00);
#else /* USE_NETWORK_LIBRARY == 0 */
        ret = com_send(p_socket->id, (const com_char_t *)"time", 4, COM_MSG_WAIT);
#endif /* USE_NETWORK_LIBRARY == 1 */
        /* Is send ok ? */
        if (ret >= 0)
        {
          /* Send ok, wait for the answer */
          uint8_t receive[75];

          (void)memset(&receive, (int8_t)'\0', sizeof(receive));
#if (USE_NETWORK_LIBRARY == 1)
          ret = net_recv(p_socket->id, (uint8_t *)&receive[0], (int32_t)sizeof(receive), 0x00);
#else /* USE_NETWORK_LIBRARY == 0 */
          ret = com_recv(p_socket->id, (com_char_t *)&receive[0], (int32_t)sizeof(receive), COM_MSG_WAIT);
#endif /* USE_NETWORK_LIBRARY == 1 */
          /* If receive is ok ? */
          if (ret > 0)
          {
            /* Analyze the answer */
            echoclient_analyze_date_time_str((uint8_t)ret, &receive[0]);
          }
        }
      }

      /* Close the socket - only try one time */
      if (p_socket->id >= 0)
      {
#if (USE_NETWORK_LIBRARY == 1)
        (void)net_closesocket(p_socket->id);
#else /* USE_NETWORK_LIBRARY == 0 */
        (void)com_closesocket(p_socket->id);
#endif /* USE_NETWORK_LIBRARY == 1 */
      }
    }
  }
}
#endif /* (USE_RTC == 1) */

/**
  * @brief  Format buffer to send
  * @note   format the buffer to send :
  *         time & date
  *         %02ld:%02ld:%02ld - %04ld/%02ld/%02ld
  *         complete with optional data
  * @param  length - buffer length
  * @retval uint16_t - final length of buffer to send
  */
static uint16_t echoclient_format_buffer(uint16_t length)
{
  /* Will add in the trame number 0 to 9 */
  uint8_t  number;
  /* Indice in the trame */
  uint16_t i;

  /* First number is 0 */
  number = 0U;

#if (USE_RTC == 1)
  /* Date and Time managed through Datacache */
  timedate_t time;

  (void)timedate_get(&time, TIMEDATE_DATE_AND_TIME);

  (void)sprintf((CRC_CHAR_t *)&echoclient_snd_buffer[0], "%02d:%02d:%02d - %04d/%02d/%02d",
                time.hour, time.min, time.sec, time.year, time.month, time.mday);

  /* After setting potential default data, i must be updated */
  i = (uint16_t)crs_strlen(&echoclient_snd_buffer[0]);
#else /* USE_RTC == 0 */
  /* Date and time unknown */
  i = 0U;
#endif /* (USE_RTC == 1) */

  /* Pad the rest of trame with ASCII number 0x30 0x31 ... 0x39 0x30 ...*/
  while (i < length)
  {
    /* Add next number in the trame */
    echoclient_snd_buffer[i] = 0x30U + number;
    number++;
    /* If number = 10 restart to 0 */
    if (number == 10U)
    {
      number = 0U;
    }
    i++;
  }
  /* Last byte is '\0' */
  echoclient_snd_buffer[length] = (uint8_t)'\0';

  return ((uint16_t)crs_strlen(&echoclient_snd_buffer[0]));
}


/**
  * @brief  Process a request
  * @note   Create, Send, Receive and Close socket
  * @param  p_socket       - pointer on socket to use
  * @param  p_buf_snd      - pointer on data buffer to send
  * @param  p_buf_rsp      - pointer on data buffer for the response
  * @param  p_snd_rcv_time - pointer to save snd/rcv transaction time
  * @retval bool           - false/true : process NOK/OK
  */
static bool echoclient_process(echoclient_socket_desc_t *p_socket,
                               const uint8_t *p_buf_snd, uint8_t *p_buf_rsp, uint32_t *p_snd_rcv_time)
{
  bool result;
  int32_t read_buf_size;
  int32_t buf_snd_len;
  int32_t buf_rsp_len;
  int32_t address_len;
#if (USE_NETWORK_LIBRARY == 1)
  net_sockaddr_in_t address;
#else /* USE_NETWORK_LIBRARY == 0 */
  com_sockaddr_in_t address;
#endif /* USE_NETWORK_LIBRARY == 1 */
  uint32_t time_begin;
  uint32_t time_end;

  result = false;
  address_len = (int32_t)sizeof(address);

  /* Increase counter */
  echoclient_stat.count++;

  /* Obtain a new socket if parameters changed or continue to use the actual one */
  if (echoclient_obtain_socket(p_socket) == true)
  {
    buf_rsp_len = (int32_t)ECHOCLIENT_SND_RCV_MAX_SIZE;

    if (((p_socket->state == ECHOCLIENT_SOCKET_CONNECTED)
         || (p_socket->state == ECHOCLIENT_SOCKET_CREATED))
        && (echoclient_distant_welcome_msg != NULL)) /* before to send anything a welcome msg will be received */
    {
      /* Socket is in good state to continue, welcome msg is expected */
      p_socket->state = ECHOCLIENT_SOCKET_WAITING_RSP;

      /* Read welcome msg according to the socket protocol */
      if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
          || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO))
      {
        PRINT_INFO("socket rcv data waiting first msg")
#if (USE_NETWORK_LIBRARY == 1)
        read_buf_size = net_recv(p_socket->id, p_buf_rsp, buf_rsp_len, 0x00);
#else /* USE_NETWORK_LIBRARY == 0 */
        read_buf_size = com_recv(p_socket->id, p_buf_rsp, buf_rsp_len, COM_MSG_WAIT);
#endif /* USE_NETWORK_LIBRARY == 1 */
      }
      else
      {
        PRINT_INFO("socket rcvfrom data waiting first msg")
#if (USE_NETWORK_LIBRARY == 1)
        read_buf_size = net_recvfrom(p_socket->id, p_buf_rsp, buf_rsp_len, 0x00,
                                     (net_sockaddr_t *)&address, (uint32_t *)&address_len);
#else /* USE_NETWORK_LIBRARY == 0 */
        read_buf_size = com_recvfrom(p_socket->id, p_buf_rsp, buf_rsp_len, COM_MSG_WAIT,
                                     (com_sockaddr_t *)&address, &address_len);
#endif /* USE_NETWORK_LIBRARY == 1 */
      }
      PRINT_INFO("socket rcvfrom data : %s", p_buf_rsp)

      /* Check welcome msg is ok or not */
      if ((read_buf_size == (int32_t)crs_strlen(echoclient_distant_welcome_msg))
          && (memcmp((const void *)&p_buf_rsp[0], (const void *)&echoclient_distant_welcome_msg[0],
                     (size_t)read_buf_size)
              == 0))
      {
        if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
            || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO))
        {
          p_socket->state = ECHOCLIENT_SOCKET_CONNECTED;
        }
        else
        {
          p_socket->state = ECHOCLIENT_SOCKET_CREATED;
        }
        PRINT_INFO("socket first msg OK")
      }
      else
      {
        /* Welcome msg is nok, the socket will be closed */
        PRINT_INFO("socket first msg NOK - close the socket")
        p_socket->closing = true;
        p_socket->state = ECHOCLIENT_SOCKET_CLOSING;
      }
      /* Welcome msg expected only one time */
      echoclient_distant_welcome_msg = NULL;
    }

    /* Is it ok to continue the process ? */
    if ((p_socket->state == ECHOCLIENT_SOCKET_CONNECTED)
        || (p_socket->state == ECHOCLIENT_SOCKET_CREATED))
    {
      int32_t ret;

      /* Send data according to the socket protocol */
      buf_snd_len = (int32_t)crs_strlen((const uint8_t *)p_buf_snd);

      if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
          || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO))
      {
        PRINT_INFO("socket snd data in progress")
#if (USE_NETWORK_LIBRARY == 1)
        time_begin = HAL_GetTick();
        ret = net_send(p_socket->id, (uint8_t *)p_buf_snd, buf_snd_len, 0x00);
#else /* USE_NETWORK_LIBRARY == 0 */
        time_begin = HAL_GetTick();
        ret = com_send(p_socket->id, (const com_char_t *)p_buf_snd, buf_snd_len, COM_MSG_WAIT);
#endif /* USE_NETWORK_LIBRARY == 1 */
      }
      else
      {
#if (USE_NETWORK_LIBRARY == 1)
        address.sin_family      = (uint8_t)NET_AF_INET;
        address.sin_port        = NET_HTONS(echoclient_distantport);
        address.sin_addr.s_addr = ECHOCLIENT_GET_DISTANTIP(echoclient_distantip);

        PRINT_INFO("socket sndto data in progress")
        time_begin = HAL_GetTick();
        ret = net_sendto(p_socket->id, (uint8_t *)p_buf_snd, buf_snd_len, 0x00,
                         (net_sockaddr_t *)&address, (int32_t)sizeof(net_sockaddr_in_t));
#else /* USE_NETWORK_LIBRARY == 0 */
        address.sin_family      = (uint8_t)COM_AF_INET;
        address.sin_port        = COM_HTONS(echoclient_distantport);
        address.sin_addr.s_addr = ECHOCLIENT_GET_DISTANTIP(echoclient_distantip);

        PRINT_INFO("socket sndto data in progress")
        time_begin = HAL_GetTick();
        ret = com_sendto(p_socket->id, p_buf_snd, buf_snd_len, COM_MSG_WAIT,
                         (const com_sockaddr_t *)&address, (int32_t)sizeof(com_sockaddr_in_t));
#endif /* USE_NETWORK_LIBRARY == 1 */
      }

      /* Data send ok ? */
      if (ret == buf_snd_len)
      {
        int32_t total_read_size = 0; /* data can be received in several packets */

        /* Data send ok, reset nfm counters, increase counters */
        echoclient_nfm_nb_error_short = 0U;
        echoclient_nfm_sleep_timer_index = 0U;
        echoclient_stat.send.ok++;
        PRINT_INFO("socket send data OK")

        p_socket->state = ECHOCLIENT_SOCKET_WAITING_RSP;

        /* Receive response according to the protocol socket */
        if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
            || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO))
        {
          PRINT_INFO("socket rcv data waiting")
          bool exit = false;

          do
          {
#if (USE_NETWORK_LIBRARY == 1)
            read_buf_size = net_recv(p_socket->id, &p_buf_rsp[total_read_size], (buf_rsp_len - total_read_size),
                                     0x00);
#else /* USE_NETWORK_LIBRARY == 0 */
            read_buf_size = com_recv(p_socket->id, &p_buf_rsp[total_read_size], (buf_rsp_len - total_read_size),
                                     COM_MSG_WAIT);
#endif /* USE_NETWORK_LIBRARY == 1 */
            if (read_buf_size < 0) /* Error during data reception ? */
            {
              exit = true;
            }
            else /* Some data received */
            {
              total_read_size += read_buf_size;
              if (total_read_size < buf_snd_len)
              {
                PRINT_INFO("socket wait more data:%ld/%ld", total_read_size, buf_snd_len)
              }
            }
          } while ((total_read_size < buf_snd_len)          /* still some data expected */
                   && ((buf_rsp_len - total_read_size) > 0) /* check still some memory in rsp buffer available */
                   && (exit == false));                     /* no error during last reception */

          time_end = HAL_GetTick(); /* end of reception */
          PRINT_INFO("socket rcv data exit")
        }
        else
        {
          PRINT_INFO("socket rcvfrom data waiting")
          bool exit = false;

          do
          {

#if (USE_NETWORK_LIBRARY == 1)
            read_buf_size = net_recvfrom(p_socket->id, &p_buf_rsp[total_read_size], (buf_rsp_len - total_read_size),
                                         0x00, (net_sockaddr_t *)&address, (uint32_t *)&address_len);
#if (USE_TRACE_ECHO_CLIENT == 1U)
            /* Data received ? */
            if (read_buf_size > 0)
            {
              net_ip_addr_t distantip;
              ECHOCLIENT_SET_DISTANTIP(&distantip, address.sin_addr.s_addr);
              /* Data received, display the server IP */
              PRINT_INFO("socket recvdata from %ld.%ld.%ld.%ld %ld",
                         ((ECHOCLIENT_GET_DISTANTIP(distantip) & 0x000000ff) >> 0),
                         ((ECHOCLIENT_GET_DISTANTIP(distantip) & 0x0000ff00) >> 8),
                         ((ECHOCLIENT_GET_DISTANTIP(distantip) & 0x00ff0000) >> 16),
                         ((ECHOCLIENT_GET_DISTANTIP(distantip) & 0xff000000) >> 24),
                         NET_NTOHS(address.sin_port))
            }
#endif /* USE_TRACE_ECHO_CLIENT == 1U */
#else /* USE_NETWORK_LIBRARY == 0 */
            read_buf_size = com_recvfrom(p_socket->id, &p_buf_rsp[total_read_size], (buf_rsp_len - total_read_size),
                                         COM_MSG_WAIT, (com_sockaddr_t *)&address, &address_len);
#if (USE_TRACE_ECHO_CLIENT == 1U)
            /* Data received ? */
            if (read_buf_size > 0)
            {
              com_ip_addr_t distantip;
              ECHOCLIENT_SET_DISTANTIP(&distantip, address.sin_addr.s_addr);
              /* Data received, display the server IP */
              PRINT_INFO("socket recvdata from %d.%d.%d.%d %d", COM_IP4_ADDR1(&distantip), COM_IP4_ADDR2(&distantip),
                         COM_IP4_ADDR3(&distantip), COM_IP4_ADDR4(&distantip), COM_NTOHS(address.sin_port))
            }
#endif /* USE_TRACE_ECHO_CLIENT == 1U */
#endif /* USE_NETWORK_LIBRARY == 1 */
            if (read_buf_size < 0)
            {
              exit = true;
            }
            else
            {
              total_read_size += read_buf_size;
              if (total_read_size < buf_snd_len)
              {
                PRINT_INFO("socket wait more data:%ld/%ld", total_read_size, buf_snd_len)
              }
            }
          } while ((total_read_size < buf_snd_len)          /* still some data expected */
                   && ((buf_rsp_len - total_read_size) > 0) /* check still some memory in rsp buffer available */
                   && (exit == false));                     /* no error during last reception */

          time_end = HAL_GetTick(); /* end of reception */
          PRINT_INFO("socket rcv data exit")
        }
        /* all data send have been received ? */
        if (buf_snd_len == total_read_size)
        {
          /* Restore socket state at the end of exchange */
          if ((echoclient_socket_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
              || (echoclient_socket_protocol == ECHOCLIENT_SOCKET_UDP_PROTO))
          {
            p_socket->state = ECHOCLIENT_SOCKET_CONNECTED;
          }
          else
          {
            p_socket->state = ECHOCLIENT_SOCKET_CREATED;
          }

          PRINT_INFO("socket rcv data %s", p_buf_rsp)

          /* Check that data received are ok */
          if (memcmp((const void *)&p_buf_snd[0], (const void *)&p_buf_rsp[0], (size_t)buf_snd_len) == 0)
          {
            /* Data received are ok, increase counters */
            echoclient_stat.receive.ok++;
            PRINT_INFO("socket rsp received OK")
            result = true;
            if (p_snd_rcv_time != NULL)
            {
              *p_snd_rcv_time = time_end - time_begin;
            }
          }
          else
          {
            /* Data received are ko, increase fault counters, request close socket */
            echoclient_stat.receive.ko++;
            PRINT_FORCE("socket rsp received NOK memcmp error")
            p_socket->closing = true;
          }
        }
        else /* read_buf != buf_snd_len */
        {
          /* Data received are ko, increase fault counters, request close socket  */
          echoclient_stat.receive.ko++;
          PRINT_FORCE("socket response NOK error:%ld data:%ld/%ld", read_buf_size, total_read_size, buf_snd_len)
          p_socket->closing = true;
        }
      }
      else /* send data ret <=0 */
      {
        /* Send data is ko, increase fault counters, request close socket  */
        echoclient_stat.send.ko++;
        PRINT_FORCE("socket send data NOK error:%ld data:%ld", ret, buf_snd_len)
        echoclient_nfm_nb_error_short++;
        p_socket->closing = true;
      }
    }
    else
    {
      PRINT_ERR("socket availability NOK");
    }

    /* Is close socket requested ? */
    if ((p_socket->closing == true)
        || (p_socket->state == ECHOCLIENT_SOCKET_CLOSING))
    {
      PRINT_INFO("socket in closing mode request the close")
      /* Timeout to receive an answer or Closing has been requested */
      echoclient_close_socket(p_socket);
    }
  }
  return result;
}

/**
  * @brief  Process a performance test iteration loop
  * @param  p_socket      - pointer on the socket to use
  * @param  iteration_nb  - iteration number
  * @param  trame_size    - trame size
  * @param  p_perf_result - pointer on result
  * @retval -
  */
static void echoclient_performance_iteration(echoclient_socket_desc_t *p_socket,
                                             uint16_t iteration_nb, uint16_t trame_size,
                                             echoclient_performance_result_t *p_perf_result)
{
  bool exit;
  uint16_t i;
  uint32_t time_snd_rcv;

  i = 0U;
  /* Update buffer with new data and potentially new length */
  if (echoclient_format_buffer(trame_size) != 0U)
  {
    exit = false;
    while ((i < iteration_nb) && (exit == false))
    {
      if (echoclient_process(p_socket, echoclient_snd_buffer, echoclient_rcv_buffer, &time_snd_rcv)
          == true)
      {
        p_perf_result->iter_ok++;
        p_perf_result->total_time += time_snd_rcv;
      }
      else
      {
        /* Try next occurrence */
        __NOP();
        /* Or Exit */
        /* exit = true; */
      }
      i++;
    }
  }
}


/**
  * @brief  Process a performance test
  * @param  p_socket - pointer on the socket to use
  * @retval -
  */
static void echoclient_performance(echoclient_socket_desc_t *p_socket)
{
#define ECHOCLIENT_PERFORMANCE_NB_ITER 8U
#define ECHOCLIENT_PERFORMANCE_TCP_TRAME_MAX ((uint16_t)(1400U))
#define ECHOCLIENT_PERFORMANCE_UDP_TRAME_MAX ((uint16_t)(1400U))
  uint16_t trame_size_in_TCP[ECHOCLIENT_PERFORMANCE_NB_ITER] =
  {
    16U, 32U, 64U, 128U, 256U, 512U, 1024U, ECHOCLIENT_PERFORMANCE_TCP_TRAME_MAX
  };
  uint16_t trame_size_in_UDP[ECHOCLIENT_PERFORMANCE_NB_ITER] =
  {
    16U, 32U, 64U, 128U, 256U, 512U, 1024U, ECHOCLIENT_PERFORMANCE_UDP_TRAME_MAX
  };
  uint16_t iter[ECHOCLIENT_PERFORMANCE_NB_ITER] =
  {
    1000U, 1000U, 1000U, 1000U, 200U, 100U, 100U, 100U
  };
  uint16_t iter_ok;
  uint16_t iter_total;
  echoclient_performance_result_t perf_result[ECHOCLIENT_PERFORMANCE_NB_ITER];
  uint16_t *p_trame_size;

  if (echoclient_socket_new_protocol == ECHOCLIENT_SOCKET_TCP_PROTO)
  {
    p_trame_size = &trame_size_in_TCP[0];
  }
  else
  {
    p_trame_size = &trame_size_in_UDP[0];
  }

  /* perf_result initialization */
  for (uint8_t i = 0U; i < ECHOCLIENT_PERFORMANCE_NB_ITER; i++)
  {
    perf_result[i].iter_ok    = 0U;
    perf_result[i].total_time = 0U;
  }
  for (uint8_t i = 0U; i < ECHOCLIENT_PERFORMANCE_NB_ITER; i++)
  {
    if (echoclient_perf_iter_nb == 0U)
    {
      echoclient_performance_iteration(p_socket, iter[i], p_trame_size[i], &perf_result[i]);
    }
    else
    {
      echoclient_performance_iteration(p_socket, echoclient_perf_iter_nb, p_trame_size[i], &perf_result[i]);
    }
  }
  /* Close the performance test socket */
  echoclient_close_socket(p_socket);

  /* Display the result */
  if (ECHOCLIENT_GET_DISTANTIP(echoclient_distantip) == 0U)
  {
    /* Distant Server IP unknown */
    PRINT_FORCE("Distant server:%s Name:%s IP:Unknown Port:%d Protocol:%s",
                echoclient_distant_server_string[echoclient_distant_server], echoclient_distantname,
                echoclient_distantport, echoclient_protocol_string[echoclient_socket_protocol])
  }
  else
  {
    /* Distant Server IP known */
#if (USE_NETWORK_LIBRARY == 1)
    PRINT_FORCE("Distant server:%s Name:%s IP:%ld.%ld.%ld.%ld Port:%d Protocol:%s",
                echoclient_distant_server_string[echoclient_distant_server], echoclient_distantname,
                (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x000000FFU) >> 0),
                (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x0000FF00U) >> 8),
                (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0x00FF0000U) >> 16),
                (((ECHOCLIENT_GET_DISTANTIP(echoclient_distantip)) & 0xFF000000U) >> 24),
                echoclient_distantport, echoclient_protocol_string[echoclient_socket_protocol])
#else /* USE_NETWORK_LIBRARY == 0 */
    PRINT_FORCE("Distant server:%s Name:%s IP:%d.%d.%d.%d Port:%d Protocol:%s",
                echoclient_distant_server_string[echoclient_distant_server], echoclient_distantname,
                COM_IP4_ADDR1(&echoclient_distantip), COM_IP4_ADDR2(&echoclient_distantip),
                COM_IP4_ADDR3(&echoclient_distantip), COM_IP4_ADDR4(&echoclient_distantip),
                echoclient_distantport, echoclient_protocol_string[echoclient_socket_protocol])
#endif /* USE_NETWORK_LIBRARY == 1 */
  }

  PRINT_FORCE(" Size  IterMax  IterOK   Data(B)   Time(ms) Throughput(Byte/s)")

  iter_ok = 0U;
  iter_total = 0U;

  for (uint8_t i = 0U; i < ECHOCLIENT_PERFORMANCE_NB_ITER; i++)
  {
    uint32_t data_snd_rcv = (uint32_t)(p_trame_size[i]) * 2U * (uint32_t)(perf_result[i].iter_ok);
    if (echoclient_perf_iter_nb == 0U)
    {
      PRINT_FORCE("%5d\t%5d\t%5d\t%7ld   %7ld      %6ld",
                  p_trame_size[i], iter[i], perf_result[i].iter_ok, data_snd_rcv, perf_result[i].total_time,
                  ((data_snd_rcv * 1000U) / (perf_result[i].total_time)))
      /* Update trace valid data */
      iter_total += iter[i];
      iter_ok    += perf_result[i].iter_ok;
    }
    else
    {
      PRINT_FORCE("%5d\t%5d\t%5d\t%7ld   %7ld      %6ld",
                  p_trame_size[i], echoclient_perf_iter_nb, perf_result[i].iter_ok, data_snd_rcv,
                  perf_result[i].total_time, ((data_snd_rcv * 1000U) / (perf_result[i].total_time)))
      /* Update trace valid data */
      iter_total += echoclient_perf_iter_nb;
      iter_ok    += perf_result[i].iter_ok;
    }
  }
  TRACE_VALID("@valid@:echoclient:stat:%d/%d\n\r", iter_ok, iter_total)
}


/**
  * @brief  Get NFMC
  * @note   Return NFMC timer value according to the current index
  * @param  index - current index of NFMC
  * note    -
  * @retval uint32_t - NFMC timer value or 0U if NFMC not active
  */
static uint32_t echoclient_get_nfmc(uint8_t index)
{
  uint32_t result;
  dc_nfmc_info_t dc_nfmc_info;

  /* Read NFMC infos */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&dc_nfmc_info, sizeof(dc_nfmc_info));

  /* Is NFMC activated ? */
  if ((dc_nfmc_info.rt_state == DC_SERVICE_ON)
      && (dc_nfmc_info.activate == 1U))
  {
    result = dc_nfmc_info.tempo[index];
    PRINT_INFO("NFMC tempo: %d-%ld", index, result)
  }
  else
  {
    result = 0U;
  }

  return result;
}


#if (USE_DEFAULT_SETUP == 0)
/**
  * @brief  Setup handler
  * @note   At initialization update echo client parameters by menu
  * @param  -
  * @retval -
  */
static uint32_t echoclient_setup_handler(void)
{
  static uint8_t echoclient_enabled[2];

  /* Enable parameter request */
  menu_utils_get_string((uint8_t *)"Echo client enabled at boot (0: not enabled - 1: enabled)",
                        echoclient_enabled, ECHOCLIENT_ENABLED_SIZE_MAX);

  /* Is echoclient enabled ? */
  if (echoclient_enabled[0] == (uint8_t)'1')
  {
    echoclient_next_process_flag = true;
  }
  else
  {
    echoclient_next_process_flag = false;
  }

  echoclient_process_flag = false;

  return 0;
}

/**
  * @brief  Setup dump
  * @note   At initialization dump the value
  * @param  -
  * @retval -
  */
static void echoclient_setup_dump(void)
{
  if (echoclient_next_process_flag == true)
  {
    PRINT_FORCE("\r\n%s enabled after boot", ECHOCLIENT_LABEL)
  }
  else
  {
    PRINT_FORCE("\r\n%s disabled after boot", ECHOCLIENT_LABEL)
  }
}

/**
  * @brief  Setup help
  * @param  -
  * @retval -
  */
static void echoclient_setup_help(void)
{
  /* Display Echoclient setup help */
  PRINT_SETUP("\r\n")
  PRINT_SETUP("================================\r\n")
  PRINT_SETUP("Echoclt configuration help\r\n")
  PRINT_SETUP("================================\r\n")
  setup_version_help();
  PRINT_SETUP("enabled after boot: allow to enable echoclient application after boot\n\r")
}
#endif /* (USE_DEFAULT_SETUP == 1) */

static void echoclient_configuration(void)
{
  static uint8_t *echoclient_default_setup_table[ECHOCLIENT_DEFAULT_PARAM_NB] =
  {
    ECHOCLIENT_DEFAULT_ENABLED
  };
#if (USE_DEFAULT_SETUP == 0)
  (void)setup_record(SETUP_APPLI_ECHOCLIENT, ECHOCLIENT_SETUP_VERSION, ECHOCLIENT_LABEL,
                     echoclient_setup_handler, echoclient_setup_dump, echoclient_setup_help,
                     echoclient_default_setup_table, ECHOCLIENT_DEFAULT_PARAM_NB);

#else /* (USE_DEFAULT_SETUP == 1) */
  if (echoclient_default_setup_table[0][0] == (uint8_t)'1')
  {
    echoclient_next_process_flag = true;
  }
  else
  {
    echoclient_next_process_flag = false;
  }

  echoclient_process_flag = false;
#endif /* (USE_DEFAULT_SETUP == 1) */
}

/**
  * @brief  Socket thread
  * @note   Infinite loop Echo client body
  * @param  p_argument - parameter osThread pointer
  * @note   UNUSED
  * @retval -
  */
static void echoclient_socket_thread(void *p_argument)
{
  /* NFMC tempo value */
  uint32_t nfmc_tempo;
  /* Msg received from the queue */
  uint32_t msg_queue = 0U;

  UNUSED(p_argument);

  for (;;)
  {
    /* Wait Network is up indication */
    (void)rtosalMessageQueueGet(echoclient_queue, &msg_queue, RTOSAL_WAIT_FOREVER);

    (void)rtosalDelay(1000U); /* just to let apps to show menu */

    /* Network is Up */
#if (USE_RTC == 1)
    /* Check if date time is umpdate by echo */
    if (echoclient_set_date_time == true)
    {
      timedate_t time;
      timedate_status_t ret;
      ret = timedate_get(&time, TIMEDATE_DATE_AND_TIME);

      if (ret != TIMEDATE_STATUS_OK)
      {
        /* Send trame to obtain date and time */
        echoclient_obtain_date_time(&echoclient_socket_desc[0]);
      }
      /* Request the date/time to the distant server only one time whatever the result */
      echoclient_set_date_time = false;
    }
#endif /* (USE_RTC == 1) */

    while (echoclient_network_is_on == true)
    {
      /* Execute the performance test ? */
      if (echoclient_perf_start == true)
      {
        PRINT_FORCE("\n\r<<< ECHOCLIENT Performance Begin >>>\n\r")
        echoclient_performance(&echoclient_socket_desc[1]);
        echoclient_perf_start = false;
        PRINT_FORCE("\n\r<<< ECHOCLIENT Performance End >>>\n\r")
      }

      /* Update echoclient_process_flag value */
      if (echoclient_process_flag != echoclient_next_process_flag)
      {
        if (echoclient_next_process_flag == true)
        {
          /* If old session not correctly closed */
          if ((echoclient_socket_desc[0].closing == true)
              || (echoclient_socket_desc[0].state != ECHOCLIENT_SOCKET_INVALID))
          {
            PRINT_FORCE("Echoclt: Old socket must be closed properly before to start a new one")
          }
          else
          {
            PRINT_FORCE("\n\r<<< ECHOCLIENT STARTED >>>\n\r")
            echoclient_process_flag = true;
          }
        }
        else
        {
          PRINT_FORCE("\n\r<<< ECHOCLIENT STOPPED >>>\n\r")
          echoclient_process_flag = false;
        }
      }

      if (echoclient_process_flag == false)
      {
        for (uint8_t i = 0U; i < 2U; i++)
        {
          if ((echoclient_socket_desc[i].closing == true)
              || (echoclient_socket_desc[i].state != ECHOCLIENT_SOCKET_INVALID))
          {
            PRINT_INFO("socket in closing mode request the close")
            /* Timeout to receive an answer or Closing has been requested */
            echoclient_close_socket(&echoclient_socket_desc[i]);
          }
        }
        /* Echo client not active */
        (void)rtosalDelay(1000U);
      }
      else
      {
        /*  Echo client active but is NFM sleep requested ?  */
        if (echoclient_is_nfm_sleep_requested() == false)
        {
          /* Update buffer with new data and potentially new length */
          if (echoclient_format_buffer(echoclient_snd_rcv_buffer_size) != 0U)
          {
            (void)echoclient_process(&echoclient_socket_desc[0], echoclient_snd_buffer, echoclient_rcv_buffer, NULL);
          }
          else
          {
            PRINT_INFO("Buffer to send empty")
          }
        }
        else
        {
          /* NFM must be activated, get the NFMC value */
          nfmc_tempo = echoclient_get_nfmc(echoclient_nfm_sleep_timer_index);
          PRINT_ERR("Too many errors: error/limit:%d/%d - timer activation:%ld ms",
                    echoclient_nfm_nb_error_short, echoclient_nfm_nb_error_limit_short, nfmc_tempo)
          (void)rtosalDelay(nfmc_tempo);
          /* Reset NFM error number to 0 */
          echoclient_nfm_nb_error_short = 0U;
          /* Increase NFM index
             if maximum index value is reached maintain it to its maximum value */
          if (echoclient_nfm_sleep_timer_index < ((uint8_t)DC_NFMC_TEMPO_NB - 1U))
          {
            echoclient_nfm_sleep_timer_index++;
          }
        }

        PRINT_DBG("Delay")
        (void)rtosalDelay(echoclient_processing_period);
      }
    }

    /* Network is down - force a close when network will be back */
    if (echoclient_socket_desc[0].state != ECHOCLIENT_SOCKET_INVALID)
    {
      PRINT_INFO("Network is down put socket in closing mode")
      echoclient_socket_desc[0].closing = true;
    }
  }
}

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialization
  * @note   Echoclient initialization
  * @param  -
  * @retval -
  */
void echoclient_init(void)
{
  for (uint8_t i = 0U; i < 2U; i++)
  {
    /* Socket initialization */
    echoclient_socket_desc[i].id = COM_SOCKET_INVALID_ID;
    echoclient_socket_desc[i].state = ECHOCLIENT_SOCKET_INVALID;
    echoclient_socket_desc[i].closing = false;
  }

  echoclient_processing_period = ECHOCLIENT_DEFAULT_PROCESSING_PERIOD;

  /* Configuration by menu or default value */
  echoclient_configuration();

  /* Socket protocol initialization */
  /* Default protocol value is UDP not-connected if supported
   * if not supported then default protocol is UDP */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  /* Set default socket Protocol value according to modem capacity */
#if (UDP_SERVICE_SUPPORTED == 1U)
  echoclient_socket_protocol = ECHOCLIENT_SOCKET_UDP_SERVICE_PROTO;
  echoclient_distant_welcome_msg = echoclient_distant_udp_welcome_msg;
#else /* UDP_SERVICE_SUPPORTED == 0U */
  echoclient_socket_protocol = ECHOCLIENT_SOCKET_UDP_PROTO;
  echoclient_distant_welcome_msg = echoclient_distant_udp_welcome_msg;
#endif /* UDP_SERVICE_SUPPORTED == 1U */
#else /* USE_SOCKETS_TYPE != USE_SOCKETS_MODEM */
  echoclient_socket_protocol = ECHOCLIENT_SOCKET_UDP_SERVICE_PROTO;
  echoclient_distant_welcome_msg = echoclient_distant_udp_welcome_msg;
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
  /* Socket new protocol initialization */
  echoclient_socket_new_protocol = echoclient_socket_protocol;

  /* Performance test initialization */
  echoclient_perf_iter_nb = 0U;

  /* NFM initialization */
  echoclient_nfm_nb_error_limit_short = ECHOCLIENT_NFM_ERROR_LIMIT_SHORT_MAX;
  echoclient_nfm_nb_error_short = 0U;
  echoclient_nfm_sleep_timer_index = 0U;

  /* Network state */
  echoclient_network_is_on = false;

  /* Buffer initialization */
  echoclient_snd_buffer[0] = (uint8_t)'\0';
  echoclient_snd_rcv_buffer_size = ECHOCLIENT_SND_RCV_MIN_SIZE;

  /* Statistic initialization */
  (void)memset((void *)&echoclient_stat, 0, sizeof(echoclient_stat));

  /* Echo distant server initialization - Default value ECHOCLIENT_MBED_SERVER */
  echoclient_distant_server_set(ECHOCLIENT_MBED_SERVER);
  /* Echo distant new server initialization */
  echoclient_distant_new_server = echoclient_distant_server;

  /* Create message queue used to exchange information between callback and echoclient thread */
  echoclient_queue = rtosalMessageQueueNew(NULL, 1U);
  if (echoclient_queue == NULL)
  {
    ERROR_Handler(DBG_CHAN_ECHOCLIENT, 1, ERROR_FATAL);
  }
}

/**
  * @brief  Start
  * @note   Echoclient start
  * @param  -
  * @retval -
  */
void echoclient_start(void)
{
  static osThreadId echoClientTaskHandle;

#if (USE_NETWORK_LIBRARY == 1)
  /* Registration to network library - for Network On/Off */
  (void)cellular_net_register(echoclient_net_notify);
#else /* USE_NETWORK_LIBRARY == 0 */
  /* Registration to datacache - for Network On/Off */
  (void)dc_com_register_gen_event_cb(&dc_com_db, echoclient_notif_cb, (void *) NULL);
#endif /* USE_NETWORK_LIBRARY == 1 */

#if (USE_RTC == 1)
  /* Echo client set date/time to do */
  /* If Echo is deactivated then another Appli is in charge to set date/time */
  echoclient_set_date_time = echoclient_next_process_flag;
#endif /* (USE_RTC == 1) */

#if (USE_CMD_CONSOLE == 1)
  /* Registration to cmd module to support cmd 'ping param' */
  CMD_Declare((uint8_t *)"echoclient", echoclient_cmd, (uint8_t *)"echoclient commands");
#endif /* USE_CMD_CONSOLE == 1 */

  /* Create Echo Client thread  */
  echoClientTaskHandle = rtosalThreadNew((const rtosal_char_t *)"EchoCltThread", (os_pthread)echoclient_socket_thread,
                                         ECHOCLIENT_THREAD_PRIO, USED_ECHOCLIENT_THREAD_STACK_SIZE, NULL);
  if (echoClientTaskHandle == NULL)
  {
    ERROR_Handler(DBG_CHAN_ECHOCLIENT, 2, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    /* Update Stack analysis with echo thread data */
    (void)stackAnalysis_addStackSizeByHandle(echoClientTaskHandle, USED_ECHOCLIENT_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

#endif /* USE_ECHO_CLIENT == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
