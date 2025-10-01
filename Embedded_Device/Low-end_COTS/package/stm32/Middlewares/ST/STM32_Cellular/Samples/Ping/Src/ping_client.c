/**
  ******************************************************************************
  * @file    ping_client.c
  * @author  MCD Application Team
  * @brief   Ping a remote host
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

#if (USE_PING_CLIENT == 1)

#include <stdbool.h>

#include "ping_client.h"
#include "ping_client_config.h"

#include "rtosal.h"

#include "dc_common.h"
#include "cellular_datacache.h"
#include "cellular_runtime_custom.h"

#include "error_handler.h"
#include "setup.h"
#include "menu_utils.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

#if (USE_BUTTONS == 1)
#include "board_buttons.h"
#endif /* USE_BUTTONS == 1 */

#include "com_sockets.h"

/* Private defines -----------------------------------------------------------*/

/* Ping request number for each loop */
#define PING_NUMBER_MAX         10U

/* Ping period between each request in a loop of ping request */
#define PING_SEND_PERIOD       500U  /* in millisecond */

/* Ping receive timeout */
#define PING_RCV_TIMEO_SEC      10U  /* 10 seconds */

/* Number of remote host ip to ping */
#define PING_REMOTE_HOSTIP_NB    3U

/* Long duration test : Ping send automatically periodically */
#define PING_LONG_DURATION_TEST  0U  /* 0: feature not activated
                                        1: feature activated */

/* Application default parameters number for setup menu */
#define PING_DEFAULT_PARAM_NB       2U

#if (USE_DEFAULT_SETUP == 0)
/* Application version for setup menu */
#define PING_SETUP_VERSION          (uint16_t)1
/* Application label for setup menu */
#define PING_LABEL                  ((uint8_t*)"Ping")
/* Application remote IP size max for setup menu */
/* SIZE_MAX for remote host IP - even if IPv6 not supported
   IPv4 : xxx.xxx.xxx.xxx=15+/0
   IPv6 : xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx = 39+/0*/
#define PING_REMOTE_IP_SIZE_MAX     40U
#endif /* USE_DEFAULT_SETUP */

#if (USE_CMD_CONSOLE == 1)
#define PING_INDEX_CMD  2U
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_PING_CLIENT == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_PING, DBL_LVL_P0, "" format, ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_PING, DBL_LVL_P0, "" format, ## args)
#define PRINT_DBG(format, args...) \
  TRACE_PRINT(DBG_CHAN_PING, DBL_LVL_P1, "Ping: " format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#define PRINT_INFO(format, args...)  (void)printf("" format, ## args);
#define PRINT_DBG(format, args...)   (void)printf("\n\r Ping: " format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */
#else /* USE_TRACE_PING_CLIENT == 0 */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_PING, DBL_LVL_P0, "" format, ## args)
#else
#include <stdio.h>
#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#endif  /* (USE_PRINTF == 0U) */
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_PING_CLIENT */

/* Private variables ---------------------------------------------------------*/
/* When a callback function is called,
 * a message is send to the queue to ask ping thread to treat the event */
/* Used to send 'network is up' indication */
static osMessageQId pingclient_queue;

/* State of Ping process */
static bool ping_process_flag; /* false: inactive/stopped,
                                  true: active
                                  default value : see ping_client_init() */
/* Targeted state of Ping process */
static bool ping_next_process_flag; /* false: inactive/stopped,
                                       true: active
                                       default value : ping_process_flag */

/* Index of remote host to ping */
static uint8_t ping_remote_hostip_index;    /* 0: ping_remote_host_ip[0]
                                               1: ping_remote_host_ip[1]
                                               ... */

/* array of the remote host ip to ping */
/* index 0: IP1
 * index 1: IP2
 * index 3: Dynamic IP - set by cmd ping xxx.xxx.xxx.xxx
 */
static uint32_t ping_remote_hostip[PING_REMOTE_HOSTIP_NB][4];

/* Number of Ping to do for each loop */
static uint8_t ping_number;

/* Ping handle = result of com_ping() */
static int32_t ping_handle;

/* Status of Network */
static bool ping_network_is_on; /* false: network is off
                                 * true:  network is on */

#if (PING_LONG_DURATION_TEST == 1U)
/* To display the ping occurrence number during long duration test */
static uint16_t ping_occurrence;
#endif /* PING_LONG_DURATION_TEST */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Callback */
static void ping_client_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);

static void ping_client_socket_thread(void *p_argument);

static void ping_client_configuration(void);

#if (USE_CMD_CONSOLE == 1)
static void ping_client_cmd_help(void);
static cmd_status_t ping_client_cmd_processing(void);
static cmd_status_t ping_client_cmd(uint8_t *cmd_line_p);
#endif /* USE_CMD_CONSOLE == 0 */

#if (USE_DEFAULT_SETUP == 0)
static uint32_t ping_client_setup_handler(void);
static void ping_client_setup_dump(void);
static void ping_client_setup_help(void);
#endif /* USE_DEFAULT_SETUP == 0 */

/* Private functions ---------------------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  help cmd management
  * @param  -
  * @retval -
  */
static void ping_client_cmd_help(void)
{
  /* Display information about ping cmd and its supported parameters */
  CMD_print_help((uint8_t *)"ping");
  PRINT_APP("ping help\n\r")
  PRINT_APP("ping                 : if no ping in progress,\n\r")
  PRINT_APP("                       start a 10 pings session to IP address pointed by Ping index\n\r")
  PRINT_APP("                       else stop the ping session and set Ping index to the next defined IP\n\r")
  PRINT_APP("ping ip1             : if no ping in progress, set Ping index to IP1 and start a 10 pings session\n\r")
  PRINT_APP("ping ip2             : if no ping in progress, set Ping index to IP2 and start a 10 pings session\n\r")
  PRINT_APP("ping ddd.ddd.ddd.ddd : if no ping in progress, \n\r")
  PRINT_APP("                       set Dynamic IP address to ddd.ddd.ddd.ddd, \n\r")
  PRINT_APP("                       set Ping index to Dynamic IP and start a 10 pings session\n\r")
  PRINT_APP("ping status          : display addresses for IP1, IP2, Dynamic IP, current Ping index and Ping state\n\r")
}

/**
  * @brief  ping client cmd processing
  * @param  -
  * @retval cmd_status_t - CMD_OK
  */
static cmd_status_t ping_client_cmd_processing(void)
{
  cmd_status_t cmd_status = CMD_OK;

  /* Can't wait the end of session Ping to return the command status
     because if so then cmd is blocked until the end of Ping session */
  if (ping_process_flag == false)
  {
    /* No Ping session in progress - a new Ping session is requested */
    if (ping_next_process_flag == false)
    {
      ping_next_process_flag = true;
      PRINT_APP("Ping Start requested...\n\r")
    }
    else
    {
      /* Ping start already in progress */
      cmd_status = CMD_PROCESS_ERROR;
      PRINT_APP("Ping Start already in progress...\n\r")
    }
  }
  else
  {
    /* A Ping is in progress */
    if (ping_next_process_flag == false)
    {
      /* Ping stop already in progress */
      cmd_status = CMD_PROCESS_ERROR;
      PRINT_APP("Ping Stop already in progress...\n\r")
    }
    else
    {
      /* Ping stop requested before the end of the ping session */
      ping_next_process_flag = false;
      PRINT_APP("Ping Stop requested...\n\r")

      /* Increment ping_remote_hostip_index */
      ping_remote_hostip_index++;
      /* Check validity of ping_remote_hostip_index */
      if (ping_remote_hostip_index > (PING_REMOTE_HOSTIP_NB - 1U))
      {
        /* Loop at the beginning of the remote hostip array */
        ping_remote_hostip_index = 0U;
      }
      /* If Dynamic IP is undefined and index is on it */
      if ((ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][0] == 0U)
          && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][1] == 0U)
          && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][2] == 0U)
          && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][3] == 0U)
          && (ping_remote_hostip_index == (PING_REMOTE_HOSTIP_NB - 1U)))
      {
        /* Loop at the beginning of the remote hostip array */
        ping_remote_hostip_index = 0U;
      }
      /* Display the status about Ping index to know which remote ip will be ping if cmd 'ping' is entered */
      if (ping_remote_hostip_index < (PING_REMOTE_HOSTIP_NB - 1U))
      {
        PRINT_APP("Ping index is now on: IP%d\n\r", (ping_remote_hostip_index + 1U))
      }
      else
      {
        PRINT_APP("Ping index is now on: dynamic IP\n\r")
      }
    }
  }
  return cmd_status;
}


/**
  * @brief  cmd management
  * @param  cmd_line_p - command parameters
  * @retval cmd_status_t - status of cmd management CMD_OK or CMD_SYNTAX_ERROR
  */
static cmd_status_t ping_client_cmd(uint8_t *cmd_line_p)
{
  uint32_t argc;
  uint8_t ip_addr[4];
  uint8_t  *argv_p[10];
  uint8_t  *cmd_p;
  cmd_status_t cmd_status = CMD_OK;

  PRINT_APP("\n\r")
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  /* Is it a command ping ? */
  if (memcmp((CRC_CHAR_t *)cmd_p, "ping", crs_strlen(cmd_p)) == 0)
  {
    /* Parameters parsing */
    for (argc = 0U; argc < 10U; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if (argc == 0U)
    {
      /* cmd 'ping' with no parameter: start a ping session on current ping index or stop it */
      cmd_status = ping_client_cmd_processing();
    }
    /*  At least one parameter provided */
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
    {
      /* cmd 'ping help': display help */
      ping_client_cmd_help();
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "ip1", crs_strlen(argv_p[0])) == 0)
    {
      /* cmd 'ping ip1': cmd allowed only if no ping in progress */
      if ((ping_process_flag == false) && (ping_next_process_flag == false))
      {
        /* Start ping session on ip1 */
        ping_remote_hostip_index = 0U;
        cmd_status = ping_client_cmd_processing();
      }
      else
      {
        PRINT_APP("Stop or Wait end of current Ping session before to call this cmd\n\r")
        cmd_status = CMD_PROCESS_ERROR;
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "ip2", crs_strlen(argv_p[0])) == 0)
    {
      /* cmd 'ping ip2': cmd allowed only if no ping in progress */
      if ((ping_process_flag == false) && (ping_next_process_flag == false))
      {
        /* Start ping session on ip2 */
        ping_remote_hostip_index = 1U;
        cmd_status = ping_client_cmd_processing();
      }
      else
      {
        PRINT_APP("Stop or Wait end of current Ping session before to call this cmd\n\r")
        cmd_status = CMD_PROCESS_ERROR;
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "status", crs_strlen(argv_p[0])) == 0)
    {
      /* Display Ping status:
       * IP1 value
       * IP2 value
       * Dynamic IP value - if defined
       * Ping index
       * Ping session status
       */
      PRINT_APP("<<< Ping status >>>\n\r")
      for (uint8_t i = 0; i < PING_REMOTE_HOSTIP_NB; i++)
      {
        /* Check the validity of the remote host ip */
        if ((ping_remote_hostip[i][0] != 0U)
            && (ping_remote_hostip[i][1] != 0U)
            && (ping_remote_hostip[i][2] != 0U)
            && (ping_remote_hostip[i][3] != 0U))
        {
          /* IP is valid - is it the Dynamic IP ? */
          if (i != (PING_REMOTE_HOSTIP_NB - 1U))
          {
            /* it is IP1 or IP2 */
            PRINT_APP("IP%d: %ld.%ld.%ld.%ld\n\r",
                      i + 1U,
                      ping_remote_hostip[i][0],
                      ping_remote_hostip[i][1],
                      ping_remote_hostip[i][2],
                      ping_remote_hostip[i][3])
          }
          else
          {
            /* it is Dynamic IP */
            PRINT_APP("Dynamic IP: %ld.%ld.%ld.%ld\n\r",
                      ping_remote_hostip[i][0],
                      ping_remote_hostip[i][1],
                      ping_remote_hostip[i][2],
                      ping_remote_hostip[i][3])
          }
        }
        else /* IP1 or IP2 is invalid or Dynamic IP not yet defined */
        {
          if (i != (PING_REMOTE_HOSTIP_NB - 1U))
          {
            /* IPx invalid */
            PRINT_APP("IP%d: NOT valid, check Ping IP parameters set in the link or through menu configuration\n\r",
                      i + 1U)
          }
          else
          {
            /* Dynamic IP undefined */
            PRINT_APP("Dynamic IP: UNDEFINED (use command 'ping ddd.ddd.ddd.ddd' to define/start it)\n\r")
          }
        }
      }
      /* Ping index status */
      if (ping_remote_hostip_index < (PING_REMOTE_HOSTIP_NB - 1U))
      {
        PRINT_APP("Ping index on: IP%d\n\r", (ping_remote_hostip_index + 1U))
      }
      else
      {
        PRINT_APP("Ping index on: dynamic IP\n\r")
      }
      /* Ping process status */
      if (ping_process_flag == true)
      {
        if (ping_next_process_flag == false)
        {
          PRINT_APP("A Ping session is in progress and stop has been requested\n\r")
        }
        else
        {
          PRINT_APP("A Ping session is in progress\n\r")
        }
      }
      else
      {
        if (ping_next_process_flag == true)
        {
          PRINT_APP("A Ping session start has been requested\n\r")
        }
        else
        {
          PRINT_APP("Ping is stopped\n\r")
        }
      }
    }
    else /* cmd 'ping xxx' enter - check if it is a valid IP */
    {
      if ((crc_get_ip_addr(argv_p[0], ip_addr, NULL) == 0U)
          && (ip_addr[0] != 0U)
          && (ip_addr[1] != 0U)
          && (ip_addr[2] != 0U)
          && (ip_addr[3] != 0U))
      {
        /* cmd 'ping xxx.xxx.xxx.xxx': cmd allowed only if no ping in progress */
        if ((ping_process_flag == false) && (ping_next_process_flag == false))
        {
          /* Start ping session on Dynamic IP */
          ping_remote_hostip_index = PING_INDEX_CMD;
          /* Update Dynamic IP with the IP provided */
          for (uint8_t i = 0U; i < 4U; i++)
          {
            ping_remote_hostip[ping_remote_hostip_index][i] = ip_addr[i];
          }
          cmd_status = ping_client_cmd_processing();
        }
        else
        {
          PRINT_APP("Stop or Wait end of current Ping session before to call this cmd\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else /* cmd 'ping xxx' not supported */
      {
        PRINT_APP("%s bad parameter or invalid IP %s !\n\r", cmd_p, argv_p[0])
        cmd_status = CMD_SYNTAX_ERROR;
        /* Display help to remind command supported */
        ping_client_cmd_help();
      }
    }
  }
  return cmd_status;
}
#endif /* USE_CMD_CONSOLE */

/**
  * @brief  Configuration handler
  * @note   At initialization update ping client parameters by menu or default value
  * @param  -
  * @retval -
  */
static void ping_client_configuration(void)
{
  /* Possible configuration: IP1 and IP2 only */
  /* Dynamic IP only configurable through cmd module */
  static uint8_t *ping_default_setup_table[PING_DEFAULT_PARAM_NB] =
  {
    PING_DEFAULT_REMOTE_HOSTIP1,
    PING_DEFAULT_REMOTE_HOSTIP2
  };

#if (USE_DEFAULT_SETUP == 0)
  /* Configuration done using the module menu */
  (void)setup_record(SETUP_APPLI_PING_CLIENT,
                     PING_SETUP_VERSION,
                     PING_LABEL,
                     ping_client_setup_handler,
                     ping_client_setup_dump,
                     ping_client_setup_help,
                     ping_default_setup_table,
                     PING_DEFAULT_PARAM_NB);

#else /* (USE_DEFAULT_SETUP == 1) */
  /* Configuration done using the default values of ping_default_setup_table */
  bool ip_valid;
  uint8_t table_indice;
  uint32_t res;
  uint8_t ip_addr[4];

  table_indice = 0U;

  for (uint8_t i = 0U; i < (PING_REMOTE_HOSTIP_NB - 1U); i++)
  {
    ip_valid = false;

    /* Check validity IP address - Only IPv4 is managed */
    res = crc_get_ip_addr(ping_default_setup_table[table_indice], ip_addr, NULL);

    if (res == 0U)
    {
      if ((ip_addr[0] != 0U)
          && (ip_addr[1] != 0U)
          && (ip_addr[2] != 0U)
          && (ip_addr[3] != 0U))
      {
        ip_valid = true;
      }
    }
    /* Is IP valid ? */
    if (ip_valid == false)
    {
      /* IP is not valid: set it to 0.0.0.0 to know that IP is invalid */
      ip_addr[0] = 0U;
      ip_addr[1] = 0U;
      ip_addr[2] = 0U;
      ip_addr[3] = 0U;
    }

    /* Update ping_remote_hostip table */
    for (uint8_t j = 0U; j < 4U; j++)
    {
      ping_remote_hostip[i][j] = ip_addr[j];
    }
    table_indice++;
  }
#endif /* (USE_DEFAULT_SETUP == 0) */

  /* Dynamic IP only set using cmd module => default value is invalid / not defined */
  ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][0] = 0U;
  ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][1] = 0U;
  ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][2] = 0U;
  ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][3] = 0U;
}

#if (USE_DEFAULT_SETUP == 0)
/**
  * @brief  Setup handler
  * @note   At initialization update ping client parameters by menu
  * @param  -
  * @retval uint32_t - always 0
  */
static uint32_t ping_client_setup_handler(void)
{
  bool ip_valid; /* false: ip is invalid - true: ip is valid */
  uint8_t i;
  uint8_t display_string[80]; /* string to request the input of a parameter */
  uint32_t res;
  uint8_t ip_addr[4];

  /* array to contain the user input */
  static uint8_t ping_remote_ip_string[PING_REMOTE_IP_SIZE_MAX];

  /* Request IP1 and IP2 input */
  for (i = 0U; i < (PING_REMOTE_HOSTIP_NB - 1U); i++)
  {
    /* Request Remote host IPi */
    (void)sprintf((CRC_CHAR_t *)display_string,
                  "Remote host IP%d to ping(xxx.xxx.xxx.xxx)",
                  i + 1U);
    ip_valid = false;
    menu_utils_get_string(display_string,
                          ping_remote_ip_string,
                          PING_REMOTE_IP_SIZE_MAX);

    /* Check validity IP address - Only IPv4 is managed */
    res = crc_get_ip_addr(ping_remote_ip_string, ip_addr, NULL);
    /* IP format is OK: xxx.xxx.xxx.xxx */
    if (res == 0U)
    {
      /* IP 0.0.0.0 is nok */
      if ((ip_addr[0] != 0U)
          && (ip_addr[1] != 0U)
          && (ip_addr[2] != 0U)
          && (ip_addr[3] != 0U))
      {
        /* IP is valid - display the result */
        ip_valid = true;
        (void)sprintf((CRC_CHAR_t *)display_string,
                      "Remote host IP%d to ping: %d.%d.%d.%d\n\r",
                      i + 1U,
                      ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
        PRINT_SETUP("%s", display_string)
      }
    }
    /* If IP is not valid - display an error message */
    if (ip_valid == false)
    {
      (void)sprintf((CRC_CHAR_t *)display_string,
                    "Remote host IP%d to ping: value or syntax NOK - IP set to invalid \"0.0.0.0\"\n\r",
                    i + 1U);
      PRINT_SETUP("%s", display_string)
      /* Set IP to invalid */
      ip_addr[0] = 0U;
      ip_addr[1] = 0U;
      ip_addr[2] = 0U;
      ip_addr[3] = 0U;
    }

    /* Save the IP in the ping_remote_hostip */
    for (uint8_t j = 0U; j < 4U; j++)
    {
      ping_remote_hostip[i][j] = ip_addr[j];
    }
  }
  return 0U;
}

/**
  * @brief  Setup help
  * @param  -
  * @retval -
  */
static void ping_client_setup_help(void)
{
  /* Display Ping setup help */
  PRINT_SETUP("\r\n")
  PRINT_SETUP("===================================\r\n")
  PRINT_SETUP("Ping configuration help\r\n")
  PRINT_SETUP("===================================\r\n")
  setup_version_help();
  PRINT_SETUP("------------------------\n\r")
  PRINT_SETUP(" Remote host IP1 to ping\n\r")
  PRINT_SETUP("------------------------\n\r")
  PRINT_SETUP("Default IP addr of remote host 1\n\r")
  PRINT_SETUP("Default value: %s\n\r", PING_DEFAULT_REMOTE_HOSTIP1)
  PRINT_SETUP("\n\r")
  PRINT_SETUP("------------------------\n\r")
  PRINT_SETUP(" Remote host IP2 to ping\n\r")
  PRINT_SETUP("------------------------\n\r")
  PRINT_SETUP("Default IP addr of remote host 2\n\r")
  PRINT_SETUP("Default value: %s\n\r", PING_DEFAULT_REMOTE_HOSTIP2)
  PRINT_SETUP("\n\r")
}

/**
  * @brief  Setup dump
  * @param  -
  * @retval -
  */
static void ping_client_setup_dump(void)
{
  uint8_t display_string[80];

  /* Display Ping setup configuration */
  for (uint8_t i = 0U; i < (PING_REMOTE_HOSTIP_NB - 1U); i++)
  {
    if ((ping_remote_hostip[i][0] <= 255U)
        && (ping_remote_hostip[i][1] <= 255U)
        && (ping_remote_hostip[i][2] <= 255U)
        && (ping_remote_hostip[i][3] <= 255U))
    {
      /* Is IP valid */
      if ((ping_remote_hostip[i][0] != 0U)
          && (ping_remote_hostip[i][1] != 0U)
          && (ping_remote_hostip[i][2] != 0U)
          && (ping_remote_hostip[i][3] != 0U))
      {
        (void)sprintf((CRC_CHAR_t *)display_string,
                      "Remote host IP%d to ping: %ld.%ld.%ld.%ld\n\r",
                      i + 1U,
                      ping_remote_hostip[i][0],
                      ping_remote_hostip[i][1],
                      ping_remote_hostip[i][2],
                      ping_remote_hostip[i][3]);
        PRINT_APP("%s", display_string)
      }
      else
      {
        PRINT_APP("!!! Remote host IP%d to ping: 0.0.0.0 is NOT valid, please change it",
                  i + 1U)
      }
    }
    else
    {
      (void)sprintf((CRC_CHAR_t *)display_string,
                    "!!! Remote host IP%d to ping: syntax NOK - IP set to invalid \"0.0.0.0\"\n\r",
                    i + 1U);
      PRINT_APP("%s", display_string)
    }
  }
}
#endif /* USE_DEFAULT_SETUP == 0 */

/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  p_private_gui_data - value provided at service subscription
  * @note   Unused parameter
  * @retval -
  */
static void ping_client_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);

  /* Event to know Network status ? */
  if (dc_event_id == DC_CELLULAR_NIFMAN_INFO)
  {
    dc_nifman_info_t  dc_nifman_info;
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO,
                      (void *)&dc_nifman_info,
                      sizeof(dc_nifman_info));
    /* Is Network up or down ? */
    if (dc_nifman_info.rt_state == DC_SERVICE_ON)
    {
      PRINT_APP("Ping: Network is up\n\r")
      ping_network_is_on = true;
      /* Inform Ping thread that network is on */
      (void)rtosalMessageQueuePut(pingclient_queue, (uint32_t)dc_event_id, 0U);
    }
    else
    {
      PRINT_APP("Ping: Network is down\n\r")
      ping_network_is_on = false;
    }
  }
#if (USE_BUTTONS == 1)
  /* Is a button pressed ? */
  /* Press button down same result than cmd 'ping' with no parameter:
     start a ping session on current ping index or stop it */
  else if (dc_event_id == DC_BOARD_BUTTONS_DOWN)
  {
    /* No ping session in progress */
    if (ping_process_flag == false)
    {
      /* Start a ping session */
      ping_next_process_flag = true;
      PRINT_APP("Ping Start requested ... \n\r")
    }
    else
    {
      /* A ping is already in progress */
      if (ping_next_process_flag == false)
      {
        /* Ping stop already in progress */
        PRINT_APP("Ping Stop in progress ...\n\r")
      }
      else
      {
        /* Ping stop requested before the end of the ping session */
        ping_next_process_flag = false;
        PRINT_APP("Ping Stop requested ...\n\r")

        /* Increment ping_remote_hostip_index */
        ping_remote_hostip_index++;
        /* Check validity of ping_remote_hostip_index */
        if (ping_remote_hostip_index > (PING_REMOTE_HOSTIP_NB - 1U))
        {
          /* Loop at the beginning of the remote hostip array */
          ping_remote_hostip_index = 0U;
        }
        /* If Dynamic IP is undefined and index is on it */
        if ((ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][0] == 0U)
            && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][1] == 0U)
            && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][2] == 0U)
            && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][3] == 0U)
            && (ping_remote_hostip_index == (PING_REMOTE_HOSTIP_NB - 1U)))
        {
          /* Loop at the beginning of the remote hostip array */
          ping_remote_hostip_index = 0U;
        }
        /* Display the status about Ping index to know which remote ip will be ping if cmd 'ping' is entered */
        if (ping_remote_hostip_index < (PING_REMOTE_HOSTIP_NB - 1U))
        {
          PRINT_APP("Ping index is now on: IP%d\n\r", (ping_remote_hostip_index + 1U))
        }
        else
        {
          PRINT_APP("Ping index is now on: dynamic IP\n\r")
        }
      }
    }
  }
#endif /* USE_BUTTONS == 1 */
  else
  {
    /* Nothing to do */
    __NOP();
  }
}

/**
  * @brief  Socket thread
  * @note   Infinite loop Ping client body
  * @param  p_argument - parameter osThread
  * @note   Unused parameter
  * @retval -
  */
static void ping_client_socket_thread(void *p_argument)
{
  uint8_t  ping_counter; /* Count the number of ping already done in the session */
  uint8_t  ping_index;   /* Index on the remote hostip currently ping */
  /* Ping Statistic */
  uint8_t  ping_rsp_ok;  /* Count number of ping ok */
  uint32_t ping_rsp_min; /* Save the response min for a ping during current session */
  uint32_t ping_rsp_max; /* Save the response max for a ping during current session */
  uint32_t ping_rsp_tot; /* Save the total of the responses during current session  */
  uint32_t msg_queue = 0U; /* Msg received from the queue */
  int32_t  result;
  com_ip_addr_t     ping_targetip; /* Save the remote host ip ping */
  com_sockaddr_in_t ping_target;
  com_ping_rsp_t    ping_rsp;

  UNUSED(p_argument);

  for (;;)
  {
    /* Wait Network is up indication */
    (void)rtosalMessageQueueGet(pingclient_queue, &msg_queue, RTOSAL_WAIT_FOREVER);

    (void)rtosalDelay(1000U); /* just to let apps to show menu */

    /* Release Ping handle of previous session if close was not ok */
    if (ping_handle != COM_HANDLE_INVALID_ID)
    {
      if (com_closeping(ping_handle) == COM_SOCKETS_ERR_OK)
      {
        /* Close session OK - Re-init ping handle to invalid ID */
        ping_handle = COM_HANDLE_INVALID_ID;
      }
    }

    /* Re-init counter for next session */
    ping_counter = 0U;
    ping_rsp_min = 0xFFFFFFFFU;
    ping_rsp_max = 0U;
    ping_rsp_tot = 0U;
    ping_rsp_ok  = 0U;

    while (ping_network_is_on == true)
    {
      /* Update ping_process_flag value */
      ping_process_flag = ping_next_process_flag;

      /* Ping session requested ? */
      if (ping_process_flag == false)
      {
        /* Release Ping handle of previous session if close was not ok */
        if (ping_handle != COM_HANDLE_INVALID_ID)
        {
          if (com_closeping(ping_handle) == COM_SOCKETS_ERR_OK)
          {
            /* Close session OK - Re-init ping handle to invalid ID */
            ping_handle = COM_HANDLE_INVALID_ID;
          }
        }
        /* Re-init counter for next session */
        ping_counter = 0U;
        ping_rsp_min = 0xFFFFFFFFU;
        ping_rsp_max = 0U;
        ping_rsp_tot = 0U;
        ping_rsp_ok  = 0U;
        (void)rtosalDelay(1000U);
#if (PING_LONG_DURATION_TEST == 1U)
        (void)rtosalDelay(1000U);
        ping_occurrence++;
        /* Restart automatically a Ping Sequence on next Remote HostIP */
        ping_process_flag = true;
        PRINT_APP("<<< Ping Start Auto %lu>>>\n\r",
                  ping_occurrence)
        ping_remote_hostip_index++;
        /* Check validity of ping_remote_hostip_index */
        if (ping_remote_hostip_index > (PING_REMOTE_HOSTIP_NB - 1U))
        {
          /* Loop at the beginning of the remote hostip array */
          ping_remote_hostip_index = 0U;
        }
        /* If Dynamic IP is undefined and index is on it */
        if ((ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][0] == 0U)
            && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][1] == 0U)
            && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][2] == 0U)
            && (ping_remote_hostip[PING_REMOTE_HOSTIP_NB - 1U][3] == 0U)
            && (ping_remote_hostip_index == (PING_REMOTE_HOSTIP_NB - 1U)))
        {
          /* Loop at the beginning of the remote hostip array */
          ping_remote_hostip_index = 0U;
        }
#endif  /*  (PING_LONG_DURATION_TEST == 1U) */
      }
      else /* Ping session requested */
      {
        /* Save ping index on the remote host ip requested */
        ping_index = ping_remote_hostip_index;
        /* Check validity of IP to ping */
        if ((ping_remote_hostip[ping_index][0] != 0U)
            && (ping_remote_hostip[ping_index][1] != 0U)
            && (ping_remote_hostip[ping_index][2] != 0U)
            && (ping_remote_hostip[ping_index][3] != 0U))
        {
          /* Start of the session ? */
          if (ping_counter == 0U)
          {
            PRINT_APP("\n\r<<< Ping Started on %ld.%ld.%ld.%ld>>>\n\r",
                      ping_remote_hostip[ping_index][0],
                      ping_remote_hostip[ping_index][1],
                      ping_remote_hostip[ping_index][2],
                      ping_remote_hostip[ping_index][3])
          }

          /* Reach the end of the session ? */
          if (ping_counter < ping_number)
          {
            /* If ping handle is invalid => request a ping handle */
            if (ping_handle == COM_HANDLE_INVALID_ID)
            {
              ping_handle = com_ping();
            }
            if (ping_handle > COM_HANDLE_INVALID_ID)
            {
              /* Process a ping */
              PRINT_DBG("open OK")
              COM_IP4_ADDR(&ping_targetip,
                           ping_remote_hostip[ping_index][0],
                           ping_remote_hostip[ping_index][1],
                           ping_remote_hostip[ping_index][2],
                           ping_remote_hostip[ping_index][3]);

              ping_target.sin_family      = (uint8_t)COM_AF_INET;
              ping_target.sin_port        = (uint16_t)0U;
              ping_target.sin_addr.s_addr = ping_targetip.addr;
              ping_target.sin_len = (uint8_t)sizeof(com_sockaddr_in_t);

              result = com_ping_process(ping_handle,
                                        (const com_sockaddr_t *)&ping_target,
                                        (int32_t)ping_target.sin_len,
                                        PING_RCV_TIMEO_SEC, &ping_rsp);

              /* Is Ping ok ? */
              if ((result == COM_SOCKETS_ERR_OK)
                  && (ping_rsp.status == COM_SOCKETS_ERR_OK))
              {
                /* Ping ok : display the result */
                PRINT_APP("Ping: %d bytes from %d.%d.%d.%d: seq=%.2d time= %ldms ttl=%d\n\r",
                          ping_rsp.size,
                          COM_IP4_ADDR1(&ping_targetip), COM_IP4_ADDR2(&ping_targetip),
                          COM_IP4_ADDR3(&ping_targetip), COM_IP4_ADDR4(&ping_targetip),
                          ping_counter + 1U,
                          ping_rsp.time,
                          ping_rsp.ttl)

                /* Update ping_rsp_max, ping_rsp_min, ping_rsp_tot, ping_rsp_ok */
                if (ping_rsp.time > ping_rsp_max)
                {
                  ping_rsp_max = ping_rsp.time;
                }
                if (ping_rsp.time < ping_rsp_min)
                {
                  ping_rsp_min = ping_rsp.time;
                }
                if (ping_rsp_tot < (0xFFFFFFFFU - ping_rsp.time))
                {
                  ping_rsp_tot += ping_rsp.time;
                }
                else
                {
                  ping_rsp_tot = 0xFFFFFFFFU;
                }
                ping_rsp_ok++;
              }
              else
              {
                /* Ping is NOK - Display an error message for this ping */
                if (result == COM_SOCKETS_ERR_TIMEOUT)
                {
                  PRINT_APP("Ping: timeout from %d.%d.%d.%d: seq=%.2d\n\r",
                            COM_IP4_ADDR1(&ping_targetip), COM_IP4_ADDR2(&ping_targetip),
                            COM_IP4_ADDR3(&ping_targetip), COM_IP4_ADDR4(&ping_targetip),
                            ping_counter + 1U)
                }
                else
                {
                  PRINT_APP("Ping: error from %d.%d.%d.%d: seq=%.2d\n\r",
                            COM_IP4_ADDR1(&ping_targetip), COM_IP4_ADDR2(&ping_targetip),
                            COM_IP4_ADDR3(&ping_targetip), COM_IP4_ADDR4(&ping_targetip),
                            ping_counter + 1U)
                }
              }
              /* Next ping */
              ping_counter++;
              (void)rtosalDelay(PING_SEND_PERIOD);
            }
            else
            {
              /* Ping handle not received */
              ping_handle = COM_HANDLE_INVALID_ID; /* Set it to invalid */
              PRINT_INFO("Ping: low-level not ready - Wait before to try again\n\r")
              /* Wait to try again */
              (void)rtosalDelay(1000U);
            }
          }

          /* Display the result */
          if ((ping_counter == ping_number)               /* Session completed                       */
              || (((ping_next_process_flag == false)      /* Session stopped by user before its end  */
                   || (ping_network_is_on == false))      /* Session stopped because network is down */
                  && (ping_counter > 0U)))                /* at least one Ping has been send         */
          {
            /* Display the result even if it is partial */
            if ((ping_rsp_ok  != 0U) /* at least one ping ok during the session */
                && (ping_rsp_tot != 0xFFFFFFFFU)) /* and it was possible to count the total response */
            {
              PRINT_APP("--- %d.%d.%d.%d Ping Stats ---\n\rPing: min/avg/max = %ld/%ld/%ld ms ok = %d/%d\n\r",
                        COM_IP4_ADDR1(&ping_targetip), COM_IP4_ADDR2(&ping_targetip),
                        COM_IP4_ADDR3(&ping_targetip), COM_IP4_ADDR4(&ping_targetip),
                        ping_rsp_min, ping_rsp_tot / ping_rsp_ok, ping_rsp_max,
                        ping_rsp_ok, ping_counter)

              TRACE_VALID("@valid@:ping:state:%d/%d\n\r", ping_rsp_ok, ping_counter)
            }
            else
            {
              if (ping_rsp_ok == 0U) /* all pings of the session nok */
              {
                PRINT_APP("--- %d.%d.%d.%d Ping Stats ---\n\rPing: min/avg/max = 0/0/0 ms ok = 0/%d\n\r",
                          COM_IP4_ADDR1(&ping_targetip), COM_IP4_ADDR2(&ping_targetip),
                          COM_IP4_ADDR3(&ping_targetip), COM_IP4_ADDR4(&ping_targetip),
                          ping_counter)

                TRACE_VALID("@valid@:ping:state:0/%d\n\r", ping_counter)
              }
              else /* some/all pings ok but total response maximum reached */
              {
                PRINT_APP("--- %d.%d.%d.%d Ping Stats ---\n\rPing: min/avg/max = %ld/Overrun/%ld ms ok = %d/%d\n\r",
                          COM_IP4_ADDR1(&ping_targetip), COM_IP4_ADDR2(&ping_targetip),
                          COM_IP4_ADDR3(&ping_targetip), COM_IP4_ADDR4(&ping_targetip),
                          ping_rsp_min, ping_rsp_max,
                          ping_rsp_ok, ping_counter)

                TRACE_VALID("@valid@:ping:state:%d/%d\n\r", ping_rsp_ok, ping_counter)
              }
            }

            /* Session goes until its end with no issue ? */
            if ((ping_counter == ping_number)
                && (ping_next_process_flag == true))
            {
              PRINT_APP("\n\r<<< Ping Completed >>>\n\r")
            }
            else
            {
              PRINT_APP("\n\r<<< Ping Stopped before the end >>>\n\r")
            }

            /* Release Ping handle */
            if (ping_handle != COM_HANDLE_INVALID_ID)
            {
              if (com_closeping(ping_handle)
                  == COM_SOCKETS_ERR_OK)
              {
                ping_handle = COM_HANDLE_INVALID_ID;
              }
            }
            /* Stop the session */
            ping_process_flag = false;
            ping_next_process_flag = false;
          }
        }
        else /* IP is 0.0.0.0 */
        {
          /* Display an error message */
          if (ping_index != (PING_REMOTE_HOSTIP_NB - 1U))
          {
            PRINT_APP("!!! IP%d: NOT valid, check Ping IP parameters set in the link or through menu configuration\n\r",
                      ping_index + 1U)
          }
          else
          {
            PRINT_APP("!!! Dynamic IP: UNDEFINED (use command 'ping ddd.ddd.ddd.ddd' to define/start it)\n\r")
          }

          TRACE_VALID("@valid@:ping:state:0/%d\n\r", ping_number)
          PRINT_APP("\n\r<<< Ping Stopped before the end due to error in IP settings >>>\n\r")
          ping_process_flag = false;
          ping_next_process_flag = false;
        }
      }
    }
  }
}

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialization
  * @note   Ping client initialization
  * @param  -
  * @retval -
  */
void pingclient_init(void)
{
  /* Ping configuration by menu or default value */
  ping_client_configuration();

  /* Ping initialization */
  ping_handle = COM_HANDLE_INVALID_ID;
  ping_remote_hostip_index  = 0U;
  ping_number = PING_NUMBER_MAX;

  /* Ping deactivated by default */
  ping_process_flag = false;
  ping_next_process_flag = ping_process_flag;

#if (PING_LONG_DURATION_TEST == 1U)
  /* Init Ping occurrence number */
  ping_occurrence = 0U;
#endif  /*  (PING_LONG_DURATION_TEST == 1U) */

  /* Create message queue used to exchange information between callback and pingclient thread */
  pingclient_queue = rtosalMessageQueueNew(NULL, 1U);
  if (pingclient_queue == NULL)
  {
    ERROR_Handler(DBG_CHAN_PING, 2, ERROR_FATAL);
  }
}

/**
  * @brief  Start
  * @note   Ping client start
  * @param  -
  * @retval -
  */
void pingclient_start(void)
{
  static osThreadId pingClientTaskHandle;

  /* Registration to datacache - for Network On/Off and Button notification */
  (void)dc_com_register_gen_event_cb(&dc_com_db, ping_client_notif_cb, (void *) NULL);

#if (USE_CMD_CONSOLE == 1)
  /* Registration to cmd module to support cmd 'ping param' */
  CMD_Declare((uint8_t *)"ping", ping_client_cmd, (uint8_t *)"ping commands");
#endif  /*  (USE_CMD_CONSOLE == 1) */

  /* Create Ping thread  */
  pingClientTaskHandle = rtosalThreadNew((const rtosal_char_t *)"PingCltThread", (os_pthread)ping_client_socket_thread,
                                         PINGCLIENT_THREAD_PRIO, USED_PINGCLIENT_THREAD_STACK_SIZE, NULL);

  if (pingClientTaskHandle == NULL)
  {
    ERROR_Handler(DBG_CHAN_PING, 3, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    /* Update Stack analysis with ping thread data */
    (void)stackAnalysis_addStackSizeByHandle(pingClientTaskHandle, USED_PINGCLIENT_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

#endif /* USE_PING_CLIENT == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
