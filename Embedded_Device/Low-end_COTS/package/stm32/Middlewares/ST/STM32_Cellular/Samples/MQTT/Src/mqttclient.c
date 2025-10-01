/**
  ******************************************************************************
  * @file    mqttclient.c
  * @author  MCD Application Team
  * @brief   Example of MQTT client application
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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

#include "mqttclient.h"

#if (USE_MQTT_CLIENT == 1)

#include <stdbool.h>

/*cstat -MISRAC2012-* */
#include "mqtt.h"
/*cstat +MISRAC2012-* */

#include "mqttclient_conf.h"

#include "rtosal.h"
#include "error_handler.h"

#include "dc_common.h"
#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
#include "dc_mems.h"
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if (USE_NETWORK_LIBRARY == 1)
#include "net_connect.h" /* includes all other includes */
#if (USE_MBEDTLS == 1)
#include "mbedtls_timedate.h"
#include "mbedtls_credentials.h"
#endif /* USE_MBEDTLS == 1 */
#else /* USE_NETWORK_LIBRARY == 0 */
#include "com_sockets.h" /* includes all other includes */
#endif /* USE_NETWORK_LIBRARY == 1 */

#include "cellular_mngt.h"
#include "cellular_runtime_custom.h"

#include "setup.h"
#include "app_select.h"
#include "menu_utils.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* USE_CMD_CONSOLE == 1 */

#if ((MQTTCLIENT_LED_MNGT == 1) && (USE_LEDS == 1))
#include "board_leds.h"
#endif /* (MQTTCLIENT_LED_MNGT == 1) && (USE_LEDS == 1) */

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define MQTTCLIENT_GET_DISTANTIP(a) ((a).addr)
#define MQTTCLIENT_SET_DISTANTIP(a, b) ((a)->addr = (b))
#define MQTTCLIENT_SET_DISTANTIP_NULL(a) ((a)->addr = (uint32_t)0U)

#if (USE_TRACE_MQTT_CLIENT == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_MQTTCLIENT, DBL_LVL_P0, format "\n\r", ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_MQTTCLIENT, DBL_LVL_P0, "Mqttclt: " format "\n\r", ## args)
#define PRINT_DBG(format, args...) \
  TRACE_PRINT(DBG_CHAN_MQTTCLIENT, DBL_LVL_P1, "Mqttclt: " format "\n\r", ## args)
#define PRINT_ERR(format, args...) \
  TRACE_PRINT(DBG_CHAN_MQTTCLIENT, DBL_LVL_ERR, "Mqttclt ERROR: " format "\n\r", ## args)
#else /* USE_PRINTF == 1U */
#include <stdio.h>
#define PRINT_FORCE(format, args...)     (void)printf(format "\n\r", ## args);
#define PRINT_INFO(format, args...)      (void)printf("Mqttclt: " format "\n\r", ## args);
/*#define PRINT_DBG(format, args...)       (void)printf("Mqttclt: " format "\n\r", ## args);*/
/* To reduce trace PRINT_DBG is deactivated when using printf */
#define PRINT_DBG(...)                   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)       (void)printf("Mqttclt ERROR: " format "\n\r", ## args);
#endif /* USE_PRINTF == 0U */

#else /* USE_TRACE_MQTT_CLIENT == 0U */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_MQTTCLIENT, DBL_LVL_P0, format "\n\r", ## args)
#else /* USE_PRINTF == 1U */
#include <stdio.h>
#define PRINT_FORCE(format, args...)     (void)printf(format "\n\r", ## args);
#endif /* USE_PRINTF == 0U */
#define PRINT_INFO(...)      __NOP(); /* Nothing to do */
#define PRINT_DBG(...)       __NOP(); /* Nothing to do */
#define PRINT_ERR(...)       __NOP(); /* Nothing to do */
#endif /* USE_TRACE_MQTT_CLIENT == 1U */

#if (USE_MBEDTLS == 1)
#define MQTTCLIENT_DEFAULT_SERVER_PORT                ((uint8_t *)"8883")
#else
#define MQTTCLIENT_DEFAULT_SERVER_PORT                ((uint8_t *)"1883")
#endif /* USE_MBEDTLS == 1 */

#if (USE_DEFAULT_SETUP == 0)
#define MQTTCLIENT_SETUP_VERSION               (1U)    /* mqttclient setup format */
#endif /* USE_DEFAULT_SETUP == 0 */

/* setup menu label */
#define MQTTCLIENT_LABEL  ((uint8_t*)"Mqttclient")


/*------------------------------------------------------*/
/* Default setup parameters  BEGIN                      */
/* (Following parameters can also be set by setup menu) */
/*------------------------------------------------------*/

/* string sizes */
#define MQTTCLIENT_SENSOR_SIZE_MAX            (16U)    /* size max of mqtt sensor data to send  */
#define MQTTCLIENT_DATA_SIZE_MAX              (32U)    /* size max of mqtt data to send         */
#define MQTTCLIENT_CONFIG_STRING_SIZE_MAX     (10U)    /* size max of config string             */
#define MQTTCLIENT_PASSWORD_SIZE_MAX          (64U)    /* size max of mqtt user                 */
#define MQTTCLIENT_USERNAME_SIZE_MAX          (64U)    /* size max of mqtt password             */
#define MQTTCLIENT_CLIENTID_SIZE_MAX          (16U)    /* size max of mqtt client id            */
#define MQTTCLIENT_ENABLED_SIZE_MAX            (2U)    /* size of enable string for setup menu  */
#define MQTTCLIENT_NAME_SIZE_MAX              (64U)    /* MAX_SIZE_IPADDR of cellular_service.h */
#if (MQTTCLIENT_LED_MNGT == 1)
#define MQTTCLIENT_LED_DATA_SIZE_MAX           (2U)    /* size of MQTT LED frame                */
#endif /* MQTTCLIENT_LED_MNGT == 1 */

#define MQTTCLIENT_ARG_MAX                     (5U)    /* nb max of command arguments */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static osMessageQId mqttclient_queue;                   /* notification queue */
static int32_t      mqttclient_socket;
#if (USE_NETWORK_LIBRARY == 1)
static net_ip_addr_t mqttclient_distantip;
#else /* USE_NETWORK_LIBRARY == 0 */
static com_ip_addr_t mqttclient_distantip;
#endif /* USE_NETWORK_LIBRARY == 1 */
/* Period of processing */
static uint32_t     mqttclient_period;

/* mqtt frame to publish */
#if (MQTTCLIENT_LED_MNGT == 1)
static uint8_t mqttclient_led[MQTTCLIENT_LED_DATA_SIZE_MAX];                 /*  led frame         */
#endif /* MQTTCLIENT_LED_MNGT == 1*/
static uint8_t mqttclient_device_opname[MQTTCLIENT_DATA_SIZE_MAX];           /*  opname frame      */
static uint8_t mqttclient_device_imsi[MQTTCLIENT_DATA_SIZE_MAX];             /*  imsi frame        */
static uint8_t mqttclient_device_imei[MQTTCLIENT_DATA_SIZE_MAX];             /*  imei frame        */

/* MQTT pubish topics */
static uint8_t *mqttclient_opname_snd_topic      = ((uint8_t *)"MNOName");
static uint8_t *mqttclient_imsi_snd_topic        = ((uint8_t *)"IMSI");
static uint8_t *mqttclient_imei_snd_topic        = ((uint8_t *)"IMEI");
static uint8_t *mqttclient_cmd_rcv_topic         = ((uint8_t *)"Command");

#if (MQTTCLIENT_LED_MNGT == 1)
/* mqtt led topics */
/* led publish topics */
static uint8_t *mqttclient_led1_snd_topic        = ((uint8_t *)"LED1");
static uint8_t *mqttclient_led2_snd_topic        = ((uint8_t *)"LED2");
static uint8_t *mqttclient_led3_snd_topic        = ((uint8_t *)"LED3");
/* led subscribe topics */
static uint8_t *mqttclient_led1_rcv_topic        = ((uint8_t *)"LED1Out");
static uint8_t *mqttclient_led2_rcv_topic        = ((uint8_t *)"LED2Out");
static uint8_t *mqttclient_led3_rcv_topic        = ((uint8_t *)"LED3Out");
#endif /* MQTTCLIENT_LED_MNGT == 1 */

/* set if a reboot command has been received from the dashboard */
static bool mqttclient_reboot_flag;

static uint16_t      mqttclient_distantport;
static uint8_t       mqttclient_distantname[MQTTCLIENT_NAME_SIZE_MAX];
static uint8_t       mqttclient_username_string[MQTTCLIENT_USERNAME_SIZE_MAX];
static uint8_t       mqttclient_password_string[MQTTCLIENT_PASSWORD_SIZE_MAX];
static uint8_t       mqttclient_client_id[MQTTCLIENT_CLIENTID_SIZE_MAX];

/* State of MQTTCLIENT process */
static bool mqttclient_process_flag; /* false: inactive, true:  active */

/* State of Network connection */
static bool mqttclient_mqtt_service_is_on;  /* false: mqtt service is down, true: mqtt service is up */
static bool mqttclient_network_initialized; /* false: network not initialized, true: network initialized  */
static bool mqttclient_network_is_on;       /* false: network is down, true: network is up  */

static struct mqtt_client mqttclient_client;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Callback */
#if (USE_NETWORK_LIBRARY == 1)
static void mqttclient_net_notify(void *p_context, uint32_t event_class, uint32_t event_id, void  *p_event_data);
#if (USE_MBEDTLS == 1)
static void mqttclient_tlsinit(void);
#endif /* USE_MBEDTLS == 1 */
#else  /* USE_NETWORK_LIBRARY == 0 */
static void mqttclient_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);
#endif /* USE_NETWORK_LIBRARY == 1 */

static void mqttclient_close_socket(void);
static void mqttclient_socket_thread(void *p_argument);
static void mqttclient_publish_callback(void **p_unused, struct mqtt_response_publish *p_published);
static void mqttclient_check_distantname(void);
static bool mqttclient_create_socket(void);
static bool mqttclient_connect_socket(void);
static bool mqttclient_init_socket(void);
static bool mqttclient_reinit_socket(void);

#if (USE_CMD_CONSOLE == 1)
static cmd_status_t mqttclient_cmd(uint8_t *p_cmd_line);
static void mqttclient_cmd_help(void);
#endif /* USE_CMD_CONSOLE == 1 */

/* Private functions ---------------------------------------------------------*/

#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  help cmd management
  * @param  -
  * @retval -
  */
static void mqttclient_cmd_help(void)
{
  CMD_print_help((uint8_t *)"mqttclient");
  PRINT_FORCE("mqttclient help")
  PRINT_FORCE("mqttclient on         : enable mqtt client process")
  PRINT_FORCE("mqttclient off        : disable mqtt client process")
  PRINT_FORCE("mqttclient led1 <0|1> : led 1 off/on")
  PRINT_FORCE("mqttclient led2 <0|1> : led 2 off/on")
  PRINT_FORCE("mqttclient led3 <0|1> : led 3 off/on")
  PRINT_FORCE("mqttclient period <n> : set the processing period to n (in ms)")
  PRINT_FORCE("mqttclient publish <topic name> <topic value>: publish a topic")
}

/**
  * @brief  cmd management
  * @param  p_cmd_line - command parameters
  * @retval cmd_status_t - status of cmd management
  */
static cmd_status_t mqttclient_cmd(uint8_t *p_cmd_line)
{
  uint8_t len; /* length parameter */
  uint32_t argc;   /* arguments */
  uint8_t  *p_argv[MQTTCLIENT_ARG_MAX];  /* counter of arguments */
  const uint8_t *p_cmd;
  int32_t  atoi_res;

  PRINT_FORCE("")
  p_cmd = (uint8_t *)strtok((CRC_CHAR_t *)p_cmd_line, " \t");
  if (p_cmd != NULL)
  {
    len = (uint8_t)crs_strlen(p_cmd);
    if (memcmp((const CRC_CHAR_t *)p_cmd, "mqttclient", len) == 0)
    {
      /* parameters parsing */
      for (argc = 0U; argc < MQTTCLIENT_ARG_MAX; argc++)
      {
        p_argv[argc] = (uint8_t *)strtok(NULL, " \t");
        if (p_argv[argc] == NULL)
        {
          break;
        }
      }

      if (argc == 0U)
      {
        mqttclient_cmd_help();
      }
      else
      {
        /*  1st parameter analysis */
        len = (uint8_t)crs_strlen(p_argv[0]);
        if (memcmp((CRC_CHAR_t *)p_argv[0], "help", len) == 0)
        {
          /* help command */
          mqttclient_cmd_help();
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "on", len) == 0)
        {
          /* mqtt process activation */
          mqttclient_process_flag = true;
          PRINT_FORCE("\n\r<<< Mqttclient on  ...>>>")
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "off", len) == 0)
        {
          /* mqtt process de activation */
          mqttclient_process_flag = false;
          PRINT_FORCE("\n\r<<< Mqttclient off  ...>>>")
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "publish", len) == 0)
        {
          if (argc == 3U)
          {
            /* publish topic */
            (void)mqtt_publish(&mqttclient_client,
                               (CRC_CHAR_t *)p_argv[1],
                               p_argv[2],
                               crs_strlen(p_argv[2]) + 1U,
                               (uint8_t)MQTT_PUBLISH_QOS_0);
            (void)mqtt_sync(&mqttclient_client);
            PRINT_FORCE("mqtt published %s: %s",  p_argv[1], p_argv[2]);
          }
          else
          {
            PRINT_FORCE("\n\rMqttclt: Wrong parameters of publish command. Usage:")
            mqttclient_cmd_help();
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "sync", len) == 0)
        {
          /* mqtt synchronize force */
          (void)mqtt_sync(&mqttclient_client);
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "period", len) == 0)
        {
          if (argc == 2U)
          {
            atoi_res = crs_atoi(p_argv[1]);
            /* res must be a number >= minimal size */
            if (atoi_res > 0)
            {
              mqttclient_period = (uint32_t)atoi_res;
              PRINT_FORCE("\n\rMqttclt: new period : %ld", mqttclient_period)
            }
            else
            {
              PRINT_FORCE("\n\rMqttclt: parameter 'period' must be >0")
            }
          }
          PRINT_FORCE("\n\rMqttclt: Period processing %ld", mqttclient_period)
        }

#if (MQTTCLIENT_LED_MNGT == 1)
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "led1", len) == 0)
        {
          uint8_t   led_value;
          /* 'led1' command */
          if (argc == 2U)
          {
            /* byte value to set */
            led_value = (uint8_t)crs_atoi(p_argv[1]);
            if (led_value == 1U)
            {
              /* LED 1 on */
              PRINT_FORCE("\n\rMqttclt: Led 1 on")
              mqttclient_led[0] = (uint8_t)'1';
            }
            else
            {
              /* LED 1 off */
              PRINT_FORCE("\n\rMqttclt: Led 1 off")
              mqttclient_led[0] = (uint8_t)'0';
            }

            mqttclient_led[1] = 0;
            /*  publish led1 state */
            (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led1_snd_topic, mqttclient_led, 1,
                               (uint8_t)MQTT_PUBLISH_QOS_0);
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "led2", len) == 0)
        {
          uint8_t   led_value;
          /* 'led2' command */
          if (argc == 2U)
          {
            /* byte value to set */
            led_value = (uint8_t)crs_atoi(p_argv[1]);
            if (led_value == 1U)
            {
              /* LED 2 on */
              PRINT_FORCE("\n\rMqttclt: Led 2 on")
              mqttclient_led[0] = (uint8_t)'1';
            }
            else
            {
              /* LED 2 off */
              PRINT_FORCE("\n\rMqttclt: Led 2 off")
              mqttclient_led[0] = (uint8_t)'0';
            }

            mqttclient_led[1] = 0;
            /*  publish led1 state */
            (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led2_snd_topic, mqttclient_led, 1,
                               (uint8_t)MQTT_PUBLISH_QOS_0);
          }
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "led3", len) == 0)
        {
          uint8_t   led_value;
          /* 'led3' command */
          if (argc == 2U)
          {
            /* byte value to set */
            led_value = (uint8_t)crs_atoi(p_argv[1]);
            if (led_value == 1U)
            {
              /* LED 3 on */
              PRINT_FORCE("\n\rMqttclt: Led 3 on")
              mqttclient_led[0] = (uint8_t)'1';
            }
            else
            {
              /* LED 3 off */
              PRINT_FORCE("\n\rMqttclt: Led 3 off")
              mqttclient_led[0] = (uint8_t)'0';
            }

            mqttclient_led[1] = 0;
            /*  publish led1 state */
            (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led3_snd_topic, mqttclient_led, 1,
                               (uint8_t)MQTT_PUBLISH_QOS_0);
          }
        }
#endif /*  (MQTTCLIENT_LED_MNGT == 1) */
        else
        {
          PRINT_FORCE("\n\rMqttclt: Unrecognised command. Usage:")
          mqttclient_cmd_help();
        }
      }
    }
    else
    {
      PRINT_FORCE("\n\rMqttclt: Unrecognised command. Usage:")
      mqttclient_cmd_help();
    }
  }
  else
  {
    PRINT_FORCE("\n\rMqttclt: Unrecognised command. Usage:")
    mqttclient_cmd_help();
  }
  return CMD_OK;
}
#endif /* USE_CMD_CONSOLE == 1 */

#if (USE_NETWORK_LIBRARY == 1)
/**
  * @brief  Callback called when a value in connect library is modified
  * @note   Used to know network status
  * @param  context - net_if_handle_t
  * @param  event_class - event class
  * @param  event_id - event id
  * @param  event_data - event data
  * @retval -
  */
static void mqttclient_net_notify(void *p_context, uint32_t event_class, uint32_t event_id, void *p_event_data)
{
  UNUSED(p_context);
  UNUSED(p_event_data);

  /* network library event management */
  if (event_class == (uint32_t)NET_EVENT_STATE_CHANGE)
  {
    if (event_id == (uint32_t)NET_STATE_CONNECTED)
    {
      /* Network On event */
      PRINT_INFO("Network is up\n\r")
      mqttclient_network_is_on = true;
      if (mqttclient_network_initialized == false)
      {
        /* 1st network initialization  */
        mqttclient_network_initialized = true;
        (void)rtosalMessageQueuePut(mqttclient_queue, event_id, 0U);
      }
    }
    else
    {
      /* Network Down event */
      mqttclient_network_is_on = false;
      PRINT_INFO("Network is down\n\r")
    }
  }
}
#else /* USE_NETWORK_LIBRARY == 0 */
/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  private_gui_data - value provided at service subscription
  * @note   Unused parameter
  * @retval -
  */
static void mqttclient_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);

  if (dc_event_id == DC_CELLULAR_NIFMAN_INFO)
  {
    /* Nifman event received */
    dc_nifman_info_t  dc_nifman_info;
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&dc_nifman_info, sizeof(dc_nifman_info));
    if (dc_nifman_info.rt_state == DC_SERVICE_ON)
    {
      /* Data transfer service ON */
      PRINT_INFO("Network is up\n\r")
      mqttclient_network_is_on = true;
      if (mqttclient_network_initialized == false)
      {
        /* 1st network initialization  */
        mqttclient_network_initialized = true;
        (void)rtosalMessageQueuePut(mqttclient_queue, (uint32_t)dc_event_id, 0U);
      }
    }
    else
    {
      /* Data transfer service OFF */
      mqttclient_network_is_on = false;
      PRINT_INFO("Network is down\n\r")
    }
  }
}
#endif /* USE_NETWORK_LIBRARY == 1 */


/**
  * @brief  check and convert mqtt server url to IP address
  * @param  -
  * @retval -
  */
static void mqttclient_check_distantname(void)
{
  /* If distantname is provided and distantip is unknown
     call DNS network resolution service */
  if ((crs_strlen(mqttclient_distantname) > 0U)
      && (MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip) == 0U))
  {
    /* DNS network resolution request */
    PRINT_INFO("Distant Name provided %s. DNS resolution started", mqttclient_distantname)
#if (USE_NETWORK_LIBRARY == 1)
    net_sockaddr_t distantaddr;
    distantaddr.sa_len = (uint8_t)sizeof(net_sockaddr_t);
    if (net_if_gethostbyname(NULL, &distantaddr, (char_t *)mqttclient_distantname) == NET_OK)
    {
      MQTTCLIENT_SET_DISTANTIP(&mqttclient_distantip, ((net_sockaddr_in_t *)&distantaddr)->sin_addr.s_addr);
      PRINT_INFO("DNS resolution OK - Mqtt Remote IP: %ld.%ld.%ld.%ld",
                 (((MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip)) & 0x000000FFU) >> 0),
                 (((MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip)) & 0x0000FF00U) >> 8),
                 (((MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip)) & 0x00FF0000U) >> 16),
                 (((MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip)) & 0xFF000000U) >> 24))
      /* No reset of nb_error_short wait to see if distant can be reached */
    }
#else /* USE_NETWORK_LIBRARY == 0 */
    com_sockaddr_t distantaddr;

    if (com_gethostbyname(mqttclient_distantname, &distantaddr) == COM_SOCKETS_ERR_OK)
    {
      /* Even next test doesn't suppress MISRA Warnings */
      /*
            if ((distantaddr.sa_len == (uint8_t)sizeof(com_sockaddr_in_t))
                && ((distantaddr.sa_family == (uint8_t)COM_AF_UNSPEC)
                    || (distantaddr.sa_family == (uint8_t)COM_AF_INET)))
      */
      {
        MQTTCLIENT_SET_DISTANTIP(&mqttclient_distantip, ((com_sockaddr_in_t *)&distantaddr)->sin_addr.s_addr);
        PRINT_INFO("DNS resolution OK - Mqtt Remote IP: %d.%d.%d.%d",
                   COM_IP4_ADDR1(&mqttclient_distantip), COM_IP4_ADDR2(&mqttclient_distantip),
                   COM_IP4_ADDR3(&mqttclient_distantip), COM_IP4_ADDR4(&mqttclient_distantip))
        /* No reset of nb_error_short wait to see if distant can be reached */
      }
    }
#endif /* USE_NETWORK_LIBRARY == 1 */
    else
    {
      PRINT_ERR("DNS resolution NOK. Will retry later")
    }
  }
}

/**
  * @brief  Create mqtt socket
  * @param  -
  * @retval - true if OK, false if failed
  */

static bool mqttclient_create_socket(void)
{
  bool result;

  /* Create a TCP socket */
#if (USE_NETWORK_LIBRARY == 1)
  /* use network library interface */
  mqttclient_socket = net_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
#else /* USE_NETWORK_LIBRARY == 0 */
  /* use com interface */
  mqttclient_socket = com_socket(COM_AF_INET, COM_SOCK_STREAM, COM_IPPROTO_TCP);
#endif /* USE_NETWORK_LIBRARY == 1 */

  if (mqttclient_socket >= 0)
  {
    result = true;
    PRINT_INFO("socket create OK")
  }
  else
  {
    result = false;
    PRINT_INFO("socket create KO")
  }

  return result;
}

/**
  * @brief  Create and connect mqtt socket
  * @note   If connect failed, close the socket
  * @param  -
  * @retval - return true if OK, false if failed
  */

static bool mqttclient_connect_socket(void)
{
  bool result;
  int32_t ret;
#if (USE_NETWORK_LIBRARY == 1)
#if (USE_MBEDTLS == 1)
  /* Root CA of MQTT sever */
  static uint8_t mqttclient_root_ca[] = MQTTCLIENT_ROOT_CA;
  bool false_val = false;
#endif /* USE_MBEDTLS == 1 */
  net_sockaddr_in_t address;
#else  /* USE_NETWORK_LIBRARY == 0 */
  com_sockaddr_in_t address;
#endif /* USE_NETWORK_LIBRARY == 1 */

  /* Create a TCP socket */
#if (USE_NETWORK_LIBRARY == 1)

#if (USE_MBEDTLS == 1)
  uint32_t timeout = 0U;

  /* set socket parameters */
  ret = net_setsockopt(mqttclient_socket, NET_SOL_SOCKET, NET_SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout));
  if (ret == NET_OK)
  {
    ret = net_setsockopt(mqttclient_socket, NET_SOL_SOCKET, NET_SO_SECURE, (const void *)NULL, 0U);
  }

  /* set CA root for mbedTLS authentication  */
  if (ret == NET_OK)
  {
    ret = net_setsockopt(mqttclient_socket, NET_SOL_SOCKET, NET_SO_TLS_CA_CERT, (void *)mqttclient_root_ca,
                         (crs_strlen((uint8_t *)mqttclient_root_ca) + 1U));
  }
  if (ret == NET_OK)
  {
    ret = net_setsockopt(mqttclient_socket, NET_SOL_SOCKET, NET_SO_TLS_SERVER_VERIFICATION, &false_val, sizeof(bool));
  }
#else  /* USE_MBEDTLS == 0 */
  ret = NET_OK;
#endif /* USE_MBEDTLS == 1 */

  if (ret == NET_OK)
  {
    address.sin_family      = (uint8_t)NET_AF_INET;
    address.sin_port        = NET_HTONS(mqttclient_distantport);
    address.sin_addr.s_addr = MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip);

    /* connection to mqtt server */
    ret = net_connect(mqttclient_socket, (net_sockaddr_t *)&address, (int32_t)sizeof(net_sockaddr_in_t));
  }
#else /* USE_NETWORK_LIBRARY == 0 */
  /* mbedTls not used => com interface used */
  address.sin_family      = (uint8_t)COM_AF_INET;
  address.sin_port        = COM_HTONS(mqttclient_distantport);
  address.sin_addr.s_addr = mqttclient_distantip.addr;
  /* connection to mqtt server */
  ret = com_connect(mqttclient_socket, (com_sockaddr_t const *)&address, (int32_t)sizeof(com_sockaddr_in_t));
#endif /* USE_NETWORK_LIBRARY == 1 */

  if (ret == 0)
  {
    result = true;
    PRINT_INFO("socket connect OK")
  }
  else
  {
    result = false;
    PRINT_INFO("socket connect KO")
    /* connection fail */
    mqttclient_close_socket();
  }

  return result;
}

/**
  * @brief  Close mqtt socket
  * @param  -
  * @retval -
  */
static void mqttclient_close_socket(void)
{
#if (USE_NETWORK_LIBRARY == 1)
  if (net_closesocket(mqttclient_socket) != NET_OK)
#else /* USE_NETWORK_LIBRARY == 0 */
  if (com_closesocket(mqttclient_socket) != COM_SOCKETS_ERR_OK)
#endif /* USE_NETWORK_LIBRARY == 1 */
  {
    PRINT_ERR("socket close NOK")
  }
  else
  {
    mqttclient_socket = -1;
    PRINT_INFO("socket close OK")
  }
}

/**
  * @brief  mqtt socket creation, connection and initialization
  * @param  -
  * @retval - true if OK, false if failed
  */
static bool mqttclient_init_socket(void)
{
  static uint8_t mqttclient_sendbuf[2048];
  static uint8_t mqttclient_recvbuf[1024];
  bool result = false;
  enum MQTTErrors mqtt_err;

  /* If distantip to contact is unknown, call DNS resolver service */
  if (MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip) == (uint32_t)0U)
  {
    /* convert mqtt server url to IP addr */
    mqttclient_check_distantname();
  }

  /* If distantip to contact is known, execute rest of the process */
  if (MQTTCLIENT_GET_DISTANTIP(mqttclient_distantip) != (uint32_t)0U)
  {
    /* mqtt socket creation */
    if (mqttclient_create_socket() == true)
    {
      /* mqtt socket connection */
      if (mqttclient_connect_socket() == true)
      {
        /* mqtt server initialization */
        mqtt_err = mqtt_init(&mqttclient_client, mqttclient_socket, mqttclient_sendbuf, sizeof(mqttclient_sendbuf),
                             mqttclient_recvbuf, sizeof(mqttclient_recvbuf), mqttclient_publish_callback);
        if (mqtt_err == MQTT_OK)
        {
          /* mqtt server connection */
          uint8_t connect_flags = (uint8_t)MQTT_CONNECT_USER_NAME | (uint8_t)MQTT_CONNECT_PASSWORD;
          mqtt_err = mqtt_connect(&mqttclient_client, (CRC_CHAR_t *)mqttclient_client_id, NULL, NULL, 0,
                                  (CRC_CHAR_t *)mqttclient_username_string, (CRC_CHAR_t *)mqttclient_password_string,
                                  connect_flags, MQTTCLIENT_KEEP_ALIVE);
          if (mqtt_err == MQTT_OK)
          {
            /* mqtt server connection OK*/
            result = true;
            mqttclient_mqtt_service_is_on = true;
          }
        }
      }
      if (result == false)
      {
        /* mqtt socket connection KO */
        mqttclient_close_socket();
        mqttclient_mqtt_service_is_on = false;
      }
    }
  }
  return result;
}

#if (USE_DEFAULT_SETUP == 0)

/**
  * @brief  Setup handler
  * @note   At initialization update mqtt app parameters by menu
  * @param  -
  * @retval - 0: Ok
  */
static uint32_t mqttclient_setup_handler(void)
{
  static uint8_t mqttclient_config_string[MQTTCLIENT_CONFIG_STRING_SIZE_MAX];
  static uint8_t mqttclient_enabled[2];

  /* gets Appli enabled at boot or not */
  menu_utils_get_string((uint8_t *)"Mqttclient client enabled at boot (0: not enabled - 1: enabled)",
                        mqttclient_enabled,
                        MQTTCLIENT_ENABLED_SIZE_MAX);

  if (mqttclient_enabled[0] == (uint8_t)'1')
  {
    mqttclient_process_flag = true;
  }
  else
  {
    mqttclient_process_flag = false;
  }

  /* gets mqtt server name */
  menu_utils_get_string((uint8_t *)"Mqtt server URL", mqttclient_distantname, MQTTCLIENT_NAME_SIZE_MAX);

  /* gets mqtt server port        */
  menu_utils_get_string((uint8_t *)"Mqtt server port", mqttclient_config_string, MQTTCLIENT_CONFIG_STRING_SIZE_MAX);

  mqttclient_distantport = (uint16_t)crs_atoi(mqttclient_config_string);

  /* gets mqtt user name          */
  menu_utils_get_string((uint8_t *)"Mqtt username", mqttclient_username_string, MQTTCLIENT_USERNAME_SIZE_MAX);

  /* gets mqtt password           */
  menu_utils_get_string((uint8_t *)"Mqtt password", mqttclient_password_string, MQTTCLIENT_PASSWORD_SIZE_MAX);

  /* sets mqtt client id          */
  menu_utils_get_string((uint8_t *)"Mqtt client Id", mqttclient_client_id, MQTTCLIENT_PASSWORD_SIZE_MAX);

  /* qets mqtt process period     */
  menu_utils_get_string((uint8_t *)"Mqtt synchro period", mqttclient_config_string, MQTTCLIENT_CONFIG_STRING_SIZE_MAX);

  mqttclient_period      = (uint32_t)crs_atoi(mqttclient_config_string);

  return 0U;
}

/**
  * @brief  Setup dump
  * @note   At initialization dump the value
  * @param  -
  * @retval -
  */
static void mqttclient_setup_dump(void)
{
  /* display mqtt parameters */
  if (mqttclient_process_flag == true)
  {
    PRINT_SETUP("\r\n%s enabled after boot\r\n", MQTTCLIENT_LABEL)
  }
  else
  {
    PRINT_SETUP("\r\n%s disabled after boot\r\n", MQTTCLIENT_LABEL)
  }

  PRINT_SETUP("Mqtt server URL    : %s\r\n", mqttclient_distantname)
  PRINT_SETUP("Mqtt server port   : %d\r\n", mqttclient_distantport)
  PRINT_SETUP("Mqtt username      : %s\r\n", mqttclient_username_string)
  PRINT_SETUP("Mqtt password      : %s\r\n", mqttclient_password_string)
  PRINT_SETUP("Mqtt client id     : %s\r\n", mqttclient_client_id)
  PRINT_SETUP("Mqtt synchro period: %ld\r\n", mqttclient_period)
}

/**
  * @brief  Setup help
  * @param  -
  * @retval -
  */
static void mqttclient_setup_help(void)
{
  PRINT_SETUP("\r\n")
  PRINT_SETUP("===================================\r\n")
  PRINT_SETUP("Mqttclient configuration help\r\n")
  PRINT_SETUP("===================================\r\n")
  /* setup version display */
  setup_version_help();
  PRINT_SETUP("enabled after boot: allow to enable mqttclient application after boot\n\r")
}
#endif /* USE_DEFAULT_SETUP == 1 */

/**
  * @brief  set default configuration
  * @param  -
  * @retval -
  */
static void mqttclient_configuration(void)
{
  /* setup default params*/
  static uint8_t *mqttclient_default_setup_table[MQTTCLIENT_DEFAULT_PARAM_NB] =
  {
    MQTTCLIENT_DEFAULT_ENABLED,           /* Appli enabled at boot   */
    MQTTCLIENT_DEFAULT_SERVER_NAME,       /* mqtt server name        */
    MQTTCLIENT_DEFAULT_SERVER_PORT,       /* mqtt server port        */
    MQTTCLIENT_DEFAULT_USERNAME,          /* mqtt user name          */
    MQTTCLIENT_DEFAULT_PASSWORD,          /* mqtt password           */
    MQTTCLIENT_DEFAULT_CLIENTID,          /* mqtt client id          */
    MQTTCLIENT_DEFAULT_SYNCHRO_PERIOD     /* mqtt process period     */
  };

#if (USE_DEFAULT_SETUP == 0)
  /* Use of setup Menu */
  /* recording to setup service */
  (void)setup_record(SETUP_APPLI_MQTTCLIENT,
                     MQTTCLIENT_SETUP_VERSION,
                     MQTTCLIENT_LABEL,
                     mqttclient_setup_handler,
                     mqttclient_setup_dump,
                     mqttclient_setup_help,
                     mqttclient_default_setup_table,
                     MQTTCLIENT_DEFAULT_PARAM_NB);

#else /* (USE_DEFAULT_SETUP == 1) */
  uint8_t *tmp_string;
  uint8_t tab_ind;
  tab_ind = 0U;

  /* setup menu not used */

  /* sets Appli enabled at boot or not */
  tmp_string = mqttclient_default_setup_table[tab_ind];
  if (tmp_string[0] == (uint8_t)'1')
  {
    mqttclient_process_flag = true;
  }
  else
  {
    mqttclient_process_flag = false;
  }

  tab_ind++;
  /* sets mqtt server name */
  (void)memcpy((CRC_CHAR_t *)mqttclient_distantname, (const CRC_CHAR_t *)mqttclient_default_setup_table[tab_ind],
               crs_strlen(mqttclient_default_setup_table[tab_ind]) + 1U);
  tab_ind++;

  /* sets mqtt server port        */
  mqttclient_distantport = (uint16_t)crs_atoi(mqttclient_default_setup_table[tab_ind]);
  tab_ind++;

  /* sets mqtt user name          */
  (void)memcpy((CRC_CHAR_t *)mqttclient_username_string, (const CRC_CHAR_t *)mqttclient_default_setup_table[tab_ind],
               crs_strlen(mqttclient_default_setup_table[tab_ind]) + 1U);
  tab_ind++;

  /* sets mqtt password           */
  (void)memcpy((CRC_CHAR_t *)mqttclient_password_string, (const CRC_CHAR_t *)mqttclient_default_setup_table[tab_ind],
               crs_strlen(mqttclient_default_setup_table[tab_ind]) + 1U);
  tab_ind++;

  /* sets mqtt client id          */
  (void)memcpy((CRC_CHAR_t *)mqttclient_client_id, (const CRC_CHAR_t *)mqttclient_default_setup_table[tab_ind],
               crs_strlen(mqttclient_default_setup_table[tab_ind]) + 1U);
  tab_ind++;
  /* sets mqtt process period     */
  mqttclient_period = (uint32_t)crs_atoi(mqttclient_default_setup_table[tab_ind]);

#endif /* USE_DEFAULT_SETUP == 1 */
}

/**
  * @brief  mqtt receiving calllback
  * @param  published   (IN) received topics
  * @retval -
  */
static void mqttclient_publish_callback(void **p_unused, struct mqtt_response_publish *p_published)
{
  UNUSED(p_unused);

  if (memcmp(p_published->topic_name, mqttclient_cmd_rcv_topic, crs_strlen(mqttclient_cmd_rcv_topic)) == 0)
  {
    /* system command management*/
    if (memcmp(p_published->application_message, "reboot", crs_strlen("reboot")) == 0)
    {
      /* Reboot command*/
      PRINT_INFO("=================== Reboot\n");
      mqttclient_reboot_flag = true;
    }
    else if (memcmp(p_published->application_message, "switchoff", crs_strlen("switchoff")) == 0)
    {
      /* switch off command*/
      PRINT_INFO("=================== Switch Off\n");
    }
    else
    {
      /* nothing to do */
      __NOP();
    }
  }
#if (MQTTCLIENT_LED_MNGT == 1)
  /* Get LED commands */
  else if (memcmp(p_published->topic_name, mqttclient_led1_rcv_topic, crs_strlen(mqttclient_led1_rcv_topic)) == 0)
  {
    /* LED 1 command */
    if (((const uint8_t *)p_published->application_message)[0] == (uint8_t)'1')
    {
      /* Command: LED Green on */
      PRINT_INFO("=================== Received publish LED Green ON\n");
#if (USE_LEDS == 1)
      board_leds_on(GREEN_LED);
#endif /* USE_LEDS == 1 */
    }
    else
    {
      /* Command: LED Green off */
      PRINT_INFO("=================== Received publish LED Green OFF\n");
#if (USE_LEDS == 1)
      board_leds_off(GREEN_LED);
#endif /* USE_LEDS == 1 */
    }
  }
  else if (memcmp(p_published->topic_name, mqttclient_led2_rcv_topic, crs_strlen(mqttclient_led1_rcv_topic)) == 0)
  {
    /* LED 2 command */
    if (((const uint8_t *)p_published->application_message)[0] == (uint8_t)'1')
    {
      /* Command: LED Red/Orange on */
      PRINT_INFO("=================== Received publish LED Red/Orange ON\n");
#if (USE_LEDS == 1)
      board_leds_on(RED_LED);
#endif /* USE_LEDS == 1 */
    }
    else
    {
      /* Command: LED 2 off */
      PRINT_INFO("=================== Received publish LED Red/Orange OFF\n");
#if (USE_LEDS == 1)
      board_leds_off(RED_LED);
#endif /* USE_LEDS == 1 */
    }
  }
  else if (memcmp(p_published->topic_name, mqttclient_led3_rcv_topic, crs_strlen(mqttclient_led1_rcv_topic)) == 0)
  {
    /* LED 3 command */
    if (((const uint8_t *)p_published->application_message)[0] == (uint8_t)'1')
    {
      /* Command: LED Blue/Other on */
      PRINT_INFO("=================== Received publish LED Blue/Other ON\n");
#if (USE_LEDS == 1)
      board_leds_on(BLUE_LED);
#endif /* USE_LEDS == 1 */
    }
    else
    {
      /* Command: LED 3 off */
      PRINT_INFO("=================== Received publish LED Blue/Other OFF\n");
#if (USE_LEDS == 1)
      board_leds_off(BLUE_LED);
#endif /* USE_LEDS == 1 */
    }
  }
  else
  {
    /* Nothing to do */
    __NOP();
  }
#endif /* MQTTCLIENT_LED_MNGT == 1 */
}

/**
  * @brief  mqtt periodical processing
  * @param  -
  * @retval -
  */
static void mqttclient_process(void)
{
#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
  static uint8_t mqttclient_device_humidity[MQTTCLIENT_SENSOR_SIZE_MAX];         /*  Humidity frame    */
  static uint8_t mqttclient_device_pressure[MQTTCLIENT_SENSOR_SIZE_MAX];         /*  pressure frame    */
  static uint8_t mqttclient_device_temperature[MQTTCLIENT_SENSOR_SIZE_MAX];      /*  temperature frame */
  static const uint8_t *mqttclient_humidity_snd_topic    = ((uint8_t *)"Humidity");
  static const uint8_t *mqttclient_pressure_snd_topic    = ((uint8_t *)"Pressure");
  static const uint8_t *mqttclient_temperature_snd_topic = ((uint8_t *)"Temperature");
  static dc_pressure_info_t      pressure_info;
  static dc_humidity_info_t      humidity_info;
  static dc_temperature_info_t   temperature_info;

  if (mqttclient_mqtt_service_is_on == true)
  {
    /* read Humidity value from sensor */
    (void)dc_com_read(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));

    if (humidity_info.rt_state == DC_SERVICE_ON)
    {
      /* publish Humidity value */
      (void)sprintf((CRC_CHAR_t *)mqttclient_device_humidity, "%f", humidity_info.humidity);
      (void)mqtt_publish(&mqttclient_client, (const CRC_CHAR_t *)mqttclient_humidity_snd_topic,
                         mqttclient_device_humidity, crs_strlen(mqttclient_device_humidity),
                         (uint8_t)MQTT_PUBLISH_QOS_0);
    }

    /* read Temperature value from sensor */
    (void)dc_com_read(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));

    if (temperature_info.rt_state == DC_SERVICE_ON)
    {
      /* publish Temperature value */
      (void)sprintf((CRC_CHAR_t *)mqttclient_device_temperature, "%f", temperature_info.temperature);
      (void)mqtt_publish(&mqttclient_client, (const CRC_CHAR_t *)mqttclient_temperature_snd_topic,
                         mqttclient_device_temperature, crs_strlen(mqttclient_device_temperature),
                         (uint8_t)MQTT_PUBLISH_QOS_0);
    }

    /* read Pressure value from sensor */
    (void)dc_com_read(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));

    if (pressure_info.rt_state == DC_SERVICE_ON)
    {
      /* publish Pressure value */
      (void)sprintf((CRC_CHAR_t *)mqttclient_device_pressure, "%f", pressure_info.pressure);
      (void)mqtt_publish(&mqttclient_client, (const CRC_CHAR_t *)mqttclient_pressure_snd_topic,
                         mqttclient_device_pressure, crs_strlen(mqttclient_device_pressure),
                         (uint8_t)MQTT_PUBLISH_QOS_0);
    }
  }
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */
}

/**
  * @brief  clear cellular info off dashboard
  * @param  -
  * @retval -
  */
static void mqttclient_clear_cellular_info(void)
{
  /* clear  OP name*/
  if (mqttclient_mqtt_service_is_on == true)
  {
    (void)memcpy((CRC_CHAR_t *)mqttclient_device_opname,  "Reboot in progress...", crs_strlen("Reboot in progress..."));
    (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_opname_snd_topic, mqttclient_device_opname,
                       crs_strlen(mqttclient_device_opname) + 1U, (uint8_t)MQTT_PUBLISH_QOS_0);

    /* clear  IMSI */
    mqttclient_device_imsi[0] = (uint8_t)' ';
    mqttclient_device_imsi[1] = (uint8_t)0;
    (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_imsi_snd_topic,
                       mqttclient_device_imsi, 1, (uint8_t)MQTT_PUBLISH_QOS_0);
    mqttclient_device_imei[0] = (uint8_t)' ';
    mqttclient_device_imei[1] = 0U;

    /* clear  IMEI */
    (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_imei_snd_topic,
                       mqttclient_device_imei, 1, (uint8_t)MQTT_PUBLISH_QOS_0);

    /* force a mqtt synchronize */
    (void)mqtt_sync(&mqttclient_client);
  }
}

/**
  * @brief  mqtt socket reinitialization (disconnect, closing socket and then initialization)
  * @param  -
  * @retval true if OK, false if failed
  */
static bool mqttclient_reinit_socket(void)
{
  bool result;

  mqttclient_mqtt_service_is_on = false;
  (void)mqtt_disconnect(&mqttclient_client);
  mqttclient_close_socket();
  result = mqttclient_init_socket();

  return result;
}

#if (USE_NETWORK_LIBRARY == 1)
#if (USE_MBEDTLS == 1)
/**
  * @brief  initialize tls credentials
  * @param  -
  * @retval -
  */
static void mqttclient_tlsinit(void)
{
  /* credential init */
  mbedtls_credentials_init();

  /* get time and date from network and set in to RTC */
  (void)mbedtls_timedate_set_from_network();
}
#endif /* USE_MBEDTLS == 1 */
#endif /* USE_NETWORK_LIBRARY == 1 */


/**
  * @brief  mqttclient thread
  * @note   Infinite loop Mqttclient client body
  * @param  p_argument - parameter osThread (unused)
  * @retval -
  */
static void mqttclient_socket_thread(void *p_argument)
{
  UNUSED(p_argument);
  static dc_cellular_info_t  cellular_info;
  static dc_sim_info_t       cellular_sim_info;

  bool ret;
  enum MQTTErrors mqtt_err;
  uint32_t msg_queue = 0U;    /* Msg received from the queue */

  /* waiting for initialized network */
  (void)rtosalMessageQueueGet(mqttclient_queue, &msg_queue, RTOSAL_WAIT_FOREVER);

#if (USE_NETWORK_LIBRARY == 1)
#if (USE_MBEDTLS == 1)
  mqttclient_tlsinit();
#endif /* USE_MBEDTLS == 1 */
#endif /* USE_NETWORK_LIBRARY == 1 */

  do
  {
    /* mqtt socket creation */
    ret = mqttclient_init_socket();
    /* Waiting for 1s */
    rtosalDelay(1000);
  } while (ret != true);

  /* read cellular infos in data cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));

  if (cellular_info.rt_state ==  DC_SERVICE_ON)
  {
    /* OP NAME to publish */
    (void)memcpy((CRC_CHAR_t *)mqttclient_device_opname,  cellular_info.mno_name, crs_strlen(cellular_info.mno_name));
    (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_opname_snd_topic,
                       mqttclient_device_opname, crs_strlen(mqttclient_device_opname) + 1U,
                       (uint8_t)MQTT_PUBLISH_QOS_0);

    /* IMEI NAME to publish */
    (void)memcpy((CRC_CHAR_t *)mqttclient_device_imei,  cellular_info.imei, crs_strlen(cellular_info.imei));
    (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_imei_snd_topic,
                       mqttclient_device_imei, crs_strlen(mqttclient_device_imei) + 1U, (uint8_t)MQTT_PUBLISH_QOS_0);
  }

  /* read SIM infos in data cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cellular_sim_info, sizeof(cellular_sim_info));

  if (cellular_info.rt_state ==  DC_SERVICE_ON)
  {
    /* IMSI to publish */
    (void)memcpy((CRC_CHAR_t *)mqttclient_device_imsi, cellular_sim_info.imsi,
                 crs_strlen((uint8_t *)cellular_sim_info.imsi));
    (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_imsi_snd_topic, mqttclient_device_imsi,
                       crs_strlen(mqttclient_device_imsi) + 1U, (uint8_t)MQTT_PUBLISH_QOS_0);
  }


  /* initial led state (off) */
#if (MQTTCLIENT_LED_MNGT == 1)
  mqttclient_led[0] = (uint8_t)'0';
  mqttclient_led[1] = 0U;

  /* led state  to publish */
  (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led1_snd_topic, mqttclient_led, 1,
                     (uint8_t)MQTT_PUBLISH_QOS_0);
  (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led2_snd_topic, mqttclient_led, 1,
                     (uint8_t)MQTT_PUBLISH_QOS_0);
  (void)mqtt_publish(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led3_snd_topic, mqttclient_led, 1,
                     (uint8_t)MQTT_PUBLISH_QOS_0);

  /* led command to subscribe */
  (void)mqtt_subscribe(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led1_rcv_topic, (int32_t)MQTT_PUBLISH_QOS_0);
  (void)mqtt_subscribe(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led2_rcv_topic, (int32_t)MQTT_PUBLISH_QOS_0);
  (void)mqtt_subscribe(&mqttclient_client, (CRC_CHAR_t *)mqttclient_led3_rcv_topic, (int32_t)MQTT_PUBLISH_QOS_0);
  (void)mqtt_subscribe(&mqttclient_client, (CRC_CHAR_t *)mqttclient_cmd_rcv_topic, (int32_t)MQTT_PUBLISH_QOS_0);
#endif /* MQTTCLIENT_LED_MNGT == 1 */

  while (true)
  {
    if (mqttclient_network_is_on == true)
    {
      if (mqttclient_reboot_flag == true)
      {
        /* a reboot command has occurred from dashboard => reboot processing */
        mqttclient_reboot_flag = false;
        /* publish clearing of cellular infos */
        mqttclient_clear_cellular_info();

        /* waiting 1s for publish achievement  */
        (void)rtosalDelay(1000);

        /* reboot */
        NVIC_SystemReset();
        /* NVIC_SystemReset never return  */
      }

      if (mqttclient_process_flag == true)
      {
        /* print a message */
        mqttclient_process();

        /* mqtt server synchronize  */
        mqtt_err = mqtt_sync(&mqttclient_client);
        PRINT_INFO("mqtt_sync ");
        if (mqtt_err == MQTT_ERROR_MALFORMED_RESPONSE)
        {
          PRINT_INFO("MQTT_ERROR_MALFORMED_RESPONSE *******************************");
        }
        else if ((mqtt_err != MQTT_OK)
                 || (mqttclient_client.error != MQTT_OK))
        {
          /* an error has occurred => socket to reinit */
          (void)mqttclient_reinit_socket();
        }
        else
        {
          /* Nothing to do */
          __NOP();
        }

        /* check for errors */
        if (mqttclient_client.error != MQTT_OK)
        {
          PRINT_INFO("error: %s\n", mqtt_error_str(mqttclient_client.error));
        }
      }
    }

    /* Waiting for nextmqtt period  */
    (void)rtosalDelay(mqttclient_period);
  }
}


/* Public Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialization
  * @note   Mqttclient initialization
  * @param  -
  * @retval -
  */
void mqttclient_init(void)
{
  /* Socket initialization */
  mqttclient_socket         = COM_SOCKET_INVALID_ID;
  mqttclient_period         = (uint32_t)crs_atoi(MQTTCLIENT_DEFAULT_SYNCHRO_PERIOD);
  /* Configuration by menu or default value */
  mqttclient_configuration();

  /* variables initialization */

  /* Network state */
  mqttclient_mqtt_service_is_on  = false;
  mqttclient_network_is_on       = false;
  mqttclient_network_initialized = false;
  mqttclient_reboot_flag = false;
  MQTTCLIENT_SET_DISTANTIP_NULL((&mqttclient_distantip));

  /* Initialize mqtt_client structure */
  (void)memset(&mqttclient_client, 0, sizeof(mqttclient_client));

  /* Event queue creation */
  mqttclient_queue = rtosalMessageQueueNew(NULL, 1U);
  if (mqttclient_queue == NULL)
  {
    ERROR_Handler(DBG_CHAN_MQTTCLIENT, 1, ERROR_FATAL);
  }
}

/**
  * @brief  Start
  * @note   Mqttclient start
  * @param  -
  * @retval -
  */
void mqttclient_start(void)
{
  static osThreadId mqttCltTaskHandle;

#if (USE_NETWORK_LIBRARY == 1)
  /* Registration to network library - for Network On/Off */
  (void)cellular_net_register(mqttclient_net_notify);
#else /* USE_NETWORK_LIBRARY == 0 */
  /* Registration to datacache */
  (void)dc_com_register_gen_event_cb(&dc_com_db, mqttclient_notif_cb, (void *) NULL);
#endif /* USE_NETWORK_LIBRARY == 1 */

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare((uint8_t *)"mqttclient", mqttclient_cmd, (uint8_t *)"mqttclient commands");
#endif /* USE_CMD_CONSOLE == 1 */

  /* Create Mqttclient Client thread  */
  mqttCltTaskHandle = rtosalThreadNew((const rtosal_char_t *)"MqttCltThread", (os_pthread)mqttclient_socket_thread,
                                      MQTTCLIENT_THREAD_PRIO, USED_MQTTCLIENT_THREAD_STACK_SIZE, NULL);
  if (mqttCltTaskHandle == NULL)
  {
    ERROR_Handler(DBG_CHAN_MQTTCLIENT, 2, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    /* stack analysis registration */
    (void)stackAnalysis_addStackSizeByHandle(mqttCltTaskHandle, USED_MQTTCLIENT_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

#endif /* USE_MQTT_CLIENT== 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
