/**
  ******************************************************************************
  * @file    dc_generic.c
  * @author  MCD Application Team
  * @brief   This file contains data cache test
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

#if (USE_DC_GENERIC == 1)

#include <stdbool.h>
#include <string.h>

#include "rtosal.h"
#include "dc_generic.h"

#include "dc_common.h"
#include "cellular_runtime_standard.h"
#include "error_handler.h"

#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#else
#include <stdio.h>
#endif  /* (USE_PRINTF == 0U) */

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

#if (USE_MQTT_CLIENT == 1)
#include "mqttclient.h"
#endif /* (USE_MQTT_CLIENT == 1) */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define DC_GEN_OUTPUT_FRAME_SIZE 10

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

#if (USE_CMD_CONSOLE == 1)
/* generic command label definition */
static uint8_t *dc_generic_cmd_label = (uint8_t *)"dcgen";
#if (USE_LINK_UART == 1)
static uint8_t *dc_generic_wb55_cmd_label = (uint8_t *)"-";
static dc_generic_byte_table_t   generic_byte_table_cmd;

static uint8_t dc_generic_device_table[DC_BYTE_TABLE_SIZE];
#endif  /* USE_LINK_UART */
#endif  /* USE_CMD_CONSOLE */

/* Global variables ----------------------------------------------------------*/
/* generic DC entries definition */
dc_com_res_id_t  DC_GENERIC_BOOL_1     = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_BOOL_2     = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_BOOL_3     = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_UINT8_1    = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_UINT8_2    = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_UINT32_1   = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_UINT32_2   = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_FLOAT_1    = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_FLOAT_2    = DC_COM_INVALID_ENTRY ;
dc_com_res_id_t  DC_GENERIC_BYTE_TABLE = DC_COM_INVALID_ENTRY ;

/* Private function prototypes -----------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
static cmd_status_t dc_generic_cmd(uint8_t *cmd_line_p);
static void dc_generic_help_cmd(void);
#if (USE_LINK_UART == 1)
static cmd_status_t dc_generic_wb55_cmd(uint8_t *cmd_line_p);
static cmd_status_t dc_generic_wb55_rec_cmd(uint8_t *cmd_line_p);



static uint8_t *dc_generic_device_cmd_label = (uint8_t *)"device";
static void dc_generic_device_state_send(uint8_t *byte_table, uint16_t device_number);
#endif /* USE_LINK_UART */
#endif /* USE_CMD_CONSOLE */

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/* trace managment macro definition */
#if (USE_PRINTF == 0U)
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_MAIN, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#define PRINT_FORCE(format, args...)   (void)printf("" format "\n\r", ## args);
#endif /* (USE_PRINTF == 0U) */

#if (USE_TRACE_DCMEMS == 1U)
#if (USE_PRINTF == 0)
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "DCTEST:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, "DCTEST:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "DCTEST ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...) (void)printf("DCTEST:" format "\n\r", ## args);
#define PRINT_DBG(format, args...)  (void)printf("DCTEST:" format "\n\r", ## args);
#define PRINT_ERR(format, args...)  (void)printf("DCTEST ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_DCMEMS */

/* Global variables ----------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

#if (USE_CMD_CONSOLE == 1)

/**
  * @brief  command help
  * @param  none
  * @retval none
  */
static void dc_generic_help_cmd(void)
{
  PRINT_FORCE("%s [help]", dc_generic_cmd_label);
  PRINT_FORCE("%s list (display all entry values", dc_generic_cmd_label);
  PRINT_FORCE("%s bool1 [on|off]  (set/get booll entry)", dc_generic_cmd_label);
  PRINT_FORCE("%s bool2 [on|off]  (set/get bool2 entry)", dc_generic_cmd_label);
  PRINT_FORCE("%s bool3 [on|off]  (set/get bool3 entry)", dc_generic_cmd_label);
  PRINT_FORCE("%s byte1 [value (0-255)]  (set/get byte1 entry)", dc_generic_cmd_label);
  PRINT_FORCE("%s byte2 [value (0-255)]  (set/get byte2 entry)", dc_generic_cmd_label);
  PRINT_FORCE("%s long1 [value]  (set/get long1 entry)", dc_generic_cmd_label);
  PRINT_FORCE("%s long2 [value]  (set/get long2 entry)", dc_generic_cmd_label);
}

#if (USE_LINK_UART == 1)
/**
  * @brief  send command to link uart
  * @param  ptr   command to send
  * @param  len   command lenght
  * @retval none
  */
void dc_gen_uartTransmit(uint8_t *ptr, uint16_t len)
{
  (void)HAL_UART_Transmit_IT(&COM_INTERFACE_UART_HANDLE, (uint8_t *)ptr, len);
}

/**
  * @brief  dc callback
  * @param  dc_event_id   dc event
  * @param  private_data   private dc data (unused)
  * @retval none
  */
static void dc_gen_notif_cb(dc_com_event_id_t dc_event_id, const void *private_data)
{
  UNUSED(private_data);
  dc_generic_uint8_info_t   generic_uint8;
  dc_generic_uint8_info_t   generic_bool;
  uint16_t framesize;
  framesize = 9U;
  static uint8_t dc_gen_output_frame[DC_GEN_OUTPUT_FRAME_SIZE];


  if (dc_event_id == DC_GENERIC_UINT8_2)
  {
    /* generic UINT8 event */
    (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_2,
                      (void *)&generic_uint8,
                      sizeof(generic_uint8));
    if (generic_uint8.rt_state == DC_SERVICE_ON)
    {
      /* valid value */
      (void)sprintf((CRC_CHAR_t *)dc_gen_output_frame, "+ E0 %03d\r", (uint16_t)generic_uint8.value);
      PRINT_FORCE("Motor Speed sent:%s", dc_gen_output_frame)
      dc_gen_uartTransmit((uint8_t *)dc_gen_output_frame, framesize);
    }
  }
  else if (dc_event_id == DC_GENERIC_BOOL_2)
  {
    /* generic BOOL event */
    (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_2,
                      (void *)&generic_bool,
                      sizeof(generic_bool));
    if (generic_bool.rt_state == DC_SERVICE_ON)
    {
      /* valid value */
      (void)sprintf((CRC_CHAR_t *)dc_gen_output_frame, "+ E1 %03d\r", (uint16_t)generic_bool.value);
      PRINT_FORCE("Ledlight sent:%s", dc_gen_output_frame)
      dc_gen_uartTransmit((uint8_t *)dc_gen_output_frame, framesize);
    }
  }
  else
  {
    /* Nothing to do */
  }
}
#endif /* (USE_LINK_UART == 1)*/

/**
  * @brief  command handle
  * @param  cmd_line_p   command line event
  * @retval command result
  */
static cmd_status_t dc_generic_cmd(uint8_t *cmd_line_p)
{
  static const uint8_t *dc_ext_activation[] =
  {
    ((uint8_t *)"off"),
    ((uint8_t *)"on")
  };
  uint8_t    *argv_p[10];
  uint32_t    argc;
  const uint8_t    *cmd_p;
  dc_generic_bool_info_t    generic_bool;
  dc_generic_uint8_info_t   generic_uint8;
  dc_generic_uint32_info_t  generic_uint32;
  dc_generic_float_info_t   generic_float;
  uint32_t   byte_value;
  uint32_t   long_value;

  PRINT_FORCE("\n\r");

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if ((cmd_p != NULL) && (strncmp((const CRC_CHAR_t *)cmd_p,
                                  (const CRC_CHAR_t *)dc_generic_cmd_label,
                                  strlen((const CRC_CHAR_t *)cmd_p)) == 0))
  {
    /* parameters parsing                     */
    for (argc = 0U ; argc < 10U ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }
    if ((argc == 0U) || (strncmp((CRC_CHAR_t *)argv_p[0], "help", strlen((CRC_CHAR_t *)argv_p[0])) == 0))
    {
      /* help command */
      PRINT_FORCE("***** %s help *****\n", (CRC_CHAR_t *)dc_generic_cmd_label);
      dc_generic_help_cmd();
    }
    else if ((argc == 0U)
             || (strncmp((CRC_CHAR_t *)argv_p[0], "list", strlen((CRC_CHAR_t *)argv_p[0])) == 0))
    {
      /* list command */
      PRINT_FORCE("***** %s Entry list *****\n", (CRC_CHAR_t *)dc_generic_cmd_label);

      /* read bool1 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_1, (void *)&generic_bool, sizeof(generic_bool));
      if (generic_bool.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("bool1: %s", dc_ext_activation[generic_bool.value]);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("bool1: invalid");
      }
      /* read bool2 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_2, (void *)&generic_bool, sizeof(generic_bool));
      if (generic_bool.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("bool2: %s", dc_ext_activation[generic_bool.value]);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("bool2 invalid");
      }
      /* read bool3 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_3, (void *)&generic_bool, sizeof(generic_bool));
      if (generic_bool.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("bool3: %s", dc_ext_activation[generic_bool.value]);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("bool3 invalid");
      }

      /* read UINT8_1 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("byte1: %d", generic_uint8.value);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("byte1 invalid");
      }

      /* read UINT8_2 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_2, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("byte2: %d", generic_uint8.value);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("byte2 invalid");
      }

      /* read UINT32_1 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("long1: %ld", generic_uint32.value);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("long1 invalid");
      }

      /* read UINT32_2 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("long2: %ld", generic_uint32.value);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("long2 invalid");
      }

      /* read FLOAT_1 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_FLOAT_1, (void *)&generic_float, sizeof(generic_float));
      if (generic_float.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("float1: %f", generic_float.value);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("float1 invalid");
      }

      /* read FLOAT_2 value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_FLOAT_2, (void *)&generic_float, sizeof(generic_float));
      if (generic_float.rt_state ==  DC_SERVICE_ON)
      {
        /* valid value */
        PRINT_FORCE("float2: %f", generic_float.value);
      }
      else
      {
        /* invalid value */
        PRINT_FORCE("float2 invalid");
      }

    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "bool1", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'bool1' command */
      if (argc == 2U)
      {
        /* boolean value to set */
        generic_bool.rt_state  =  DC_SERVICE_ON;
        if (strncmp((const CRC_CHAR_t *)argv_p[1],
                    (const CRC_CHAR_t *)(dc_ext_activation[0]),
                    strlen((const CRC_CHAR_t *)argv_p[1]))
            == 0)
        {
          /* set value in DC */
          generic_bool.value      =  0;
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_1, (void *)&generic_bool, sizeof(generic_bool));
        }
        else if (strncmp((const CRC_CHAR_t *)argv_p[1],
                         (const CRC_CHAR_t *)(dc_ext_activation[1]),
                         strlen((const CRC_CHAR_t *)argv_p[1]))
                 == 0)
        {
          generic_bool.value      =  1;
          /* set value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_1, (void *)&generic_bool, sizeof(generic_bool));
        }
        else
        {
          PRINT_FORCE("bool1: bad value %s", (CRC_CHAR_t *)argv_p[1]);
        }
      }
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_1, (void *)&generic_bool, sizeof(generic_bool));

      if (generic_bool.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("bool1: %s", dc_ext_activation[generic_bool.value]);
      }
      else
      {
        PRINT_FORCE("bool1 invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "bool2", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'bool2' command */
      if (argc == 2U)
      {
        /* boolean value to set */
        generic_bool.rt_state  =  DC_SERVICE_ON;
        if (strncmp((const CRC_CHAR_t *)argv_p[1],
                    (const CRC_CHAR_t *)(dc_ext_activation[0]),
                    strlen((const CRC_CHAR_t *)argv_p[1]))
            == 0)
        {
          generic_bool.value      =  0;
          /* set value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_2, (void *)&generic_bool, sizeof(generic_bool));
        }
        else if (strncmp((const CRC_CHAR_t *)argv_p[1],
                         (const CRC_CHAR_t *)(dc_ext_activation[1]),
                         strlen((const CRC_CHAR_t *)argv_p[1]))
                 == 0)
        {
          generic_bool.value      =  1;
          /* set value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_2, (void *)&generic_bool, sizeof(generic_bool));
        }
        else
        {
          PRINT_FORCE("bool2: bad value %s", (CRC_CHAR_t *)argv_p[1]);
        }
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_2, (void *)&generic_bool, sizeof(generic_bool));

      if (generic_bool.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("bool2: %s", dc_ext_activation[generic_bool.value]);
      }
      else
      {
        PRINT_FORCE("bool2 invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "bool3", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'bool3' command */
      if (argc == 2U)
      {
        /* boolean value to set */
        generic_bool.rt_state  =  DC_SERVICE_ON;
        if (strncmp((const CRC_CHAR_t *)argv_p[1],
                    (const CRC_CHAR_t *)(dc_ext_activation[0]),
                    strlen((const CRC_CHAR_t *)argv_p[1]))
            == 0)
        {
          generic_bool.value      =  0;
          /* set value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_3, (void *)&generic_bool, sizeof(generic_bool));
        }
        else if (strncmp((const CRC_CHAR_t *)argv_p[1],
                         (const CRC_CHAR_t *)(dc_ext_activation[1]),
                         strlen((const CRC_CHAR_t *)argv_p[1]))
                 == 0)
        {
          generic_bool.value      =  1;
          /* set value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_3, (void *)&generic_bool, sizeof(generic_bool));
        }
        else
        {
          PRINT_FORCE("bool3: bad value %s", (CRC_CHAR_t *)argv_p[1]);
        }
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BOOL_3, (void *)&generic_bool, sizeof(generic_bool));

      if (generic_bool.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("bool3: %s", dc_ext_activation[generic_bool.value]);
      }
      else
      {
        PRINT_FORCE("bool3 invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "byte1", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'byte1' command */
      if (argc == 2U)
      {
        /* byte value to set */
        byte_value = (uint32_t)crs_atoi(argv_p[1]);
        generic_uint8.rt_state  =  DC_SERVICE_ON;
        generic_uint8.value     = (uint8_t)byte_value;
        /* set value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_uint8, sizeof(generic_uint8));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("byte1: %d", generic_uint8.value);
      }
      else
      {
        PRINT_FORCE("byte1 invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "byte2", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'byte2' command */
      if (argc == 2U)
      {
        /* byte value to set */
        byte_value = (uint32_t)crs_atoi(argv_p[1]);

        generic_uint8.rt_state  =  DC_SERVICE_ON;
        generic_uint8.value     = (uint8_t)byte_value;
        /* set value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT8_2, (void *)&generic_uint8, sizeof(generic_uint8));
      }
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_2, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("byte2: %d", generic_uint8.value);
      }
      else
      {
        PRINT_FORCE("byte2 invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "long1", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'long1' command */
      if (argc == 2U)
      {
        /* long value to set */
        long_value = (uint32_t)crs_atoi(argv_p[1]);
        generic_uint32.rt_state  =  DC_SERVICE_ON;
        generic_uint32.value     =  long_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_uint32, sizeof(generic_uint32));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("long1: %ld", generic_uint32.value);
      }
      else
      {
        PRINT_FORCE("long1 invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "long2", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* 'long2' command */
      if (argc == 2U)
      {
        /* long value to set */
        long_value = (uint32_t)crs_atoi(argv_p[1]);
        generic_uint32.rt_state  =  DC_SERVICE_ON;
        generic_uint32.value     =  long_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_uint32, sizeof(generic_uint32));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("long2: %ld", generic_uint32.value);
      }
      else
      {
        PRINT_FORCE("long2 invalid");
      }
    }
    else
    {
      /* invalid command */
      PRINT_FORCE("%s %s: invalid command", (CRC_CHAR_t *)dc_generic_cmd_label, argv_p[0]);
      PRINT_FORCE("%s usage:", (CRC_CHAR_t *)dc_generic_cmd_label);
      PRINT_FORCE("------------");

      /* display help */
      dc_generic_help_cmd();
    }

  }
  return CMD_OK;
}

#if (USE_LINK_UART == 1)
/**
  * @brief  command managmenet to set link device
  * @param  none
  * @retval none
  */
static void dc_generic_device_cmd_help(void)
{
  CMD_print_help(dc_generic_device_cmd_label);
  PRINT_FORCE("%s [help]", dc_generic_device_cmd_label);
  PRINT_FORCE("%s set <device id> <action (0-1)>  (send action to a remote device)", dc_generic_device_cmd_label);
  PRINT_FORCE("%s state>  (read device state)", dc_generic_device_cmd_label);
}

static cmd_status_t dc_generic_device_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;

  uint8_t device_id;
  uint8_t device_action;
  uint32_t    i;

  const uint8_t    *cmd_p;


  PRINT_FORCE("\n\r");

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (strncmp((const CRC_CHAR_t *)cmd_p, (const CRC_CHAR_t *)dc_generic_device_cmd_label,
              strlen((const CRC_CHAR_t *)cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < 10U ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }
    if ((argc == 0U) || (strncmp((CRC_CHAR_t *)argv_p[0], "help", strlen((CRC_CHAR_t *)argv_p[0])) == 0))
    {
      /* no argument or help command => display help */
      dc_generic_device_cmd_help();
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "set", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* device set <device> <action> */
      if (argc == 3U)
      {
        device_id = (uint8_t)crs_atoi(argv_p[1]);
        if (device_id < DC_BYTE_TABLE_SIZE)
        {
          device_action = (uint8_t)crs_atoi(argv_p[2]);
          /*          dc_generic_device_set(device_id, device_action); */
          dc_generic_device_table[crs_atoi(argv_p[1])] = device_action;
          dc_generic_device_state_send(dc_generic_device_table, DC_BYTE_TABLE_SIZE);
        }
        else
        {
          PRINT_FORCE("device command ERROR. device id >= %d", DC_BYTE_TABLE_SIZE);
        }
      }
      else
      {
        PRINT_FORCE("device command ERROR. Usage : 'device set <device id> <action (0-1>'");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "state", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      (void)dc_com_read(&dc_com_db, DC_GENERIC_BYTE_TABLE, (void *)&generic_byte_table_cmd,
                        sizeof(dc_generic_byte_table_t));
      if (generic_byte_table_cmd.rt_state  !=  DC_SERVICE_ON)
      {
        PRINT_FORCE("Byte Table Service OFF");
      }
      else
      {
        for (i = 0U ; i < DC_BYTE_TABLE_SIZE ; i++)
        {
          PRINT_FORCE("Id %3d : %d", i, generic_byte_table_cmd.table[i]);
        }
      }
    }
    else
    {
      /* invalid command */
      PRINT_FORCE("%s %s: invalid command", (CRC_CHAR_t *)dc_generic_device_cmd_label, argv_p[0]);
      PRINT_FORCE("%s usage:", (CRC_CHAR_t *)dc_generic_device_cmd_label);
      PRINT_FORCE("------------");

      /* display help */
      dc_generic_device_cmd_help();
    }
  }
  return CMD_OK;
}

/**
  * @brief  help command for ces demo
  * @param  none
  * @retval none
  */
static void dc_generic_w55_help_cmd(void)
{
  CMD_print_help(dc_generic_cmd_label);
  PRINT_FORCE("%s [help]", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s list (display all entry values)", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s F0: ToF Distance", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s F1: ToF Counter", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s F2: SPL", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s 67: Temperature", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s 73: Pressure", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s D0 <DEV ID> <Action>” (<Action> is 1=switch ON or 0=switch OFF)", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s D1 <Nb> <Bitmap>” (Nb is the number of devices)", dc_generic_wb55_cmd_label);
  PRINT_FORCE("%s tbyte\r” (byte table list)", dc_generic_wb55_cmd_label);
}


/**
  * @brief  send device state bitmap on UART
  * @param  byte_table   table of device state
  * @param  device_number   device number
  * @retval none
  */

static void dc_generic_device_state_send(uint8_t *byte_table, uint16_t device_number)
{
  static uint8_t dc_generic_device_cmd_to_send[(DC_BYTE_TABLE_SIZE >> 4) + 20U];
  static uint8_t *dc_generic_device_state_send_cmdid = "D1";
  static uint8_t *dc_generic_device_state_send_cmdheader = "-";
  uint16_t i;
  uint8_t digit;
  uint16_t digit_ind;

  /* setting frame header in command to send ('- D1 <device number> ') */
  (void)sprintf((CRC_CHAR_t *)dc_generic_device_cmd_to_send, "%s %s %d ",
                (CRC_CHAR_t *)dc_generic_device_state_send_cmdheader,
                (CRC_CHAR_t *)dc_generic_device_state_send_cmdid, device_number);

  digit_ind = (uint16_t)crs_strlen(dc_generic_device_cmd_to_send);

  /* parsing device table */
  for (i = 0 ; i < device_number ; i++)
  {
    digit = byte_table[i];

    /* hexa digit to ASCII conversion */
    if (digit <= 9U)
    {
      digit = digit + (uint8_t)'0';
    }
    else
    {
      digit = digit + (uint8_t)'A' - 10U;
    }

    /* Adding new digit in command to send */
    dc_generic_device_cmd_to_send[digit_ind] = digit;
    digit_ind++;
  }

  /* adding end of frame delimiter */
  dc_generic_device_cmd_to_send[digit_ind] = (uint8_t)'\r';
  digit_ind++;
  dc_generic_device_cmd_to_send[digit_ind] = 0U;
  PRINT_FORCE("device set(%d bytes): %s\r\n", digit_ind, dc_generic_device_cmd_to_send);

  /* sending frame on uart */
  dc_gen_uartTransmit(dc_generic_device_cmd_to_send, digit_ind);
}

/**
  * @brief  command managmenet for CES demo
  * @param  none
  * @retval none
  */
static cmd_status_t dc_generic_wb55_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;
  uint8_t    byte_number;
  const uint8_t    *cmd_p;
  dc_generic_uint8_info_t   generic_uint8;
  dc_generic_uint32_info_t  generic_uint32;
  dc_generic_float_info_t   generic_float;
  uint32_t   byte_value;
  uint32_t   long_value;
  float_t    float_value;
  int32_t    int_value;

  PRINT_FORCE("\n\r");

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (strncmp((const CRC_CHAR_t *)cmd_p, (const CRC_CHAR_t *)dc_generic_wb55_cmd_label,
              strlen((const CRC_CHAR_t *)cmd_p)) == 0)
  {
    /* parameters parsing                     */
    for (argc = 0U ; argc < 10U ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }
    if ((argc == 0U) || (strncmp((CRC_CHAR_t *)argv_p[0], "help", strlen((CRC_CHAR_t *)argv_p[0])) == 0))
    {
      /* no argument or help command => display help */
      dc_generic_w55_help_cmd();
    }
    else if ((argc == 0U)
             || (strncmp((CRC_CHAR_t *)argv_p[0], "list", strlen((CRC_CHAR_t *)argv_p[0])) == 0))
    {
      /* list command: display all generic values */
      PRINT_FORCE("***** %s Entry list *****\n", (CRC_CHAR_t *)dc_generic_wb55_cmd_label);

      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("SPL(dB): %d", generic_uint8.value);
      }
      else
      {
        PRINT_FORCE("SPL invalid");
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_2, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("Motor Speed(%%): %ld", (uint32_t)generic_uint8.value);
      }
      else
      {
        PRINT_FORCE("Motor Speed invalid");
      }

      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("ToF distance(cm): %ld", generic_uint32.value);
      }
      else
      {
        PRINT_FORCE("ToF distance invalid");
      }

      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("ToF count: %ld", generic_uint32.value);
      }
      else
      {
        PRINT_FORCE("ToF count invalid");
      }

      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_FLOAT_1, (void *)&generic_float, sizeof(generic_float));
      if (generic_float.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("temperature(°C): %f", generic_float.value);
      }
      else
      {
        PRINT_FORCE("temperature invalid");
      }

      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_FLOAT_2, (void *)&generic_float, sizeof(generic_float));
      if (generic_float.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("Pressure: %f", generic_float.value);
      }
      else
      {
        PRINT_FORCE("Pressure invalid");
      }

    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "F2", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* SPL 0xF2  (dB SPL - 3 ascii bytes) = 55dB SPL  (Suggested range: 0-120) */
      if (argc == 2U)
      {
        byte_value = (uint32_t)crs_atoi(argv_p[1]);
        generic_uint8.rt_state  =  DC_SERVICE_ON;
        generic_uint8.value     = (uint8_t)byte_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_uint8, sizeof(generic_uint8));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_uint8, sizeof(generic_uint8));
      if (generic_uint8.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("SPL(dB): %d", generic_uint8.value);
      }
      else
      {
        PRINT_FORCE("SPL invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "F0", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* ToF Distance 0xF0 (cm - 3 ascii bytes) = 270cm  (Suggested range: 0-1000) */
      if (argc == 2U)
      {
        long_value = (uint32_t)crs_atoi(argv_p[1]);
        generic_uint32.rt_state  =  DC_SERVICE_ON;
        generic_uint32.value     =  long_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_uint32, sizeof(generic_uint32));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("ToF dist(cm): %ld", generic_uint32.value);
      }
      else
      {
        PRINT_FORCE("ToF dist invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "F1", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* ToF Counter 0xF1 (# - 3 ascii bytes - 100's/10's/1's) = 194 people  (Suggested range: 0-1000) */
      if (argc == 2U)
      {
        long_value = (uint32_t)crs_atoi(argv_p[1]);
        generic_uint32.rt_state  =  DC_SERVICE_ON;
        generic_uint32.value     =  long_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_uint32, sizeof(generic_uint32));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_uint32, sizeof(generic_uint32));
      if (generic_uint32.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("ToF count: %ld", generic_uint32.value);
      }
      else
      {
        PRINT_FORCE("ToF count invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "67", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* Temp 0x67 (0.1degC - 3 ascii bytes) = 23.4 degC  (Suggested range: 0-100) */
      if (argc == 2U)
      {
        int_value = crs_atoi(argv_p[1]);
        float_value = (float_t)int_value / (float_t)10 ;
        generic_float.rt_state  =  DC_SERVICE_ON;
        generic_float.value     =  float_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_FLOAT_1, (void *)&generic_float, sizeof(generic_float));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_FLOAT_1, (void *)&generic_float, sizeof(generic_float));
      if (generic_float.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("temperature(°C): %f", generic_float.value);
      }
      else
      {
        PRINT_FORCE("temperature invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "73", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* Pressure 0x73 (kPa's - 3 ascii bytes) = 103 kPa's (about 1.02ATM)  (Suggested range: 50-150) */
      if (argc == 2U)
      {
        int_value = crs_atoi(argv_p[1]);
        float_value = (float_t)int_value / (float_t)10 ;
        generic_float.rt_state  =  DC_SERVICE_ON;
        generic_float.value     =  float_value;
        /* write value in DC */
        (void)dc_com_write(&dc_com_db, DC_GENERIC_FLOAT_2, (void *)&generic_float, sizeof(generic_float));
      }
      /* read value in DC */
      (void)dc_com_read(&dc_com_db, DC_GENERIC_FLOAT_2, (void *)&generic_float, sizeof(generic_float));
      if (generic_float.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("Pression: %f", generic_float.value);
      }
      else
      {
        PRINT_FORCE("Pression invalid");
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "D0", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* “– D0 <LED ID> <Action>\r” (<Action> is 1 = switch ON or 0 = switch OFF) */
      if (argc == 3U)
      {
        byte_number = (uint8_t)crs_atoi(argv_p[1]);
        byte_value = (uint32_t)crs_atoi(argv_p[2]);
        if (byte_number >= DC_BYTE_TABLE_SIZE)
        {
          PRINT_FORCE("ERROR: - D0 ID: %d > %d ", (uint16_t)byte_number, DC_BYTE_TABLE_SIZE - 1U);
        }
        else
        {
          (void)dc_com_read(&dc_com_db, DC_GENERIC_BYTE_TABLE,
                            (void *)&generic_byte_table_cmd, sizeof(dc_generic_byte_table_t));
          generic_byte_table_cmd.rt_state  =  DC_SERVICE_ON;
          generic_byte_table_cmd.table[byte_number] = (uint8_t)byte_value;
          /* write value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BYTE_TABLE,
                             (void *)&generic_byte_table_cmd, sizeof(dc_generic_byte_table_t));
          PRINT_FORCE("Byte %d set: %d", byte_number, generic_byte_table_cmd.table[byte_number]);
        }
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "D0", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* “– D0 <LED ID> <Action>\r” (<Action> is 1 = switch ON or 0 = switch OFF) */
      if (argc == 3U)
      {
        byte_number = (uint8_t)crs_atoi(argv_p[1]);
        byte_value = (uint32_t)crs_atoi(argv_p[2]);
        if (byte_number >= DC_BYTE_TABLE_SIZE)
        {
          PRINT_FORCE("ERROR: - D0 ID: %d > %d ", (uint16_t)byte_number, DC_BYTE_TABLE_SIZE - 1U);
        }
        else
        {
          (void)dc_com_read(&dc_com_db, DC_GENERIC_BYTE_TABLE,
                            (void *)&generic_byte_table_cmd, sizeof(dc_generic_byte_table_t));
          generic_byte_table_cmd.rt_state  =  DC_SERVICE_ON;
          generic_byte_table_cmd.table[byte_number] = (uint8_t)byte_value;
          /* write value in DC */
          (void)dc_com_write(&dc_com_db, DC_GENERIC_BYTE_TABLE,
                             (void *)&generic_byte_table_cmd, sizeof(dc_generic_byte_table_t));
          PRINT_FORCE("Byte %d set: %d", byte_number, generic_byte_table_cmd.table[byte_number]);
        }
      }
    }
    else if (strncmp((CRC_CHAR_t *)argv_p[0], "D1", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
    {
      /* “– D1 <Nb> <Bitmap>\r”  */
      if (argc == 3U)
      {
        byte_number = (uint8_t)crs_atoi(argv_p[1]);
        if (byte_number > DC_BYTE_TABLE_SIZE)
        {
          PRINT_FORCE("ERROR: - D1 Nb: %d > %d ", (uint16_t)byte_number, DC_BYTE_TABLE_SIZE - 1U);
        }
        else
        {
#if (USE_MQTT_CLIENT == 1)
          /*            mqttapp_publish_devices(rcv_byte_table, byte_number); */
          mqttclient_publish_bitmap(argv_p[2]);
#endif /* (USE_MQTT_CLIENT == 1) */
          PRINT_FORCE("UART pattern received: %s ", argv_p[2]);
        }
      }
    }
    else
    {
      /* invalid command */
      PRINT_FORCE("%s %s: invalid command", (CRC_CHAR_t *)dc_generic_cmd_label, argv_p[0]);
      PRINT_FORCE("%s usage:", (CRC_CHAR_t *)dc_generic_cmd_label);
      PRINT_FORCE("------------");

      /* display help */
      dc_generic_w55_help_cmd();
    }
  }
  return CMD_OK;
}

/**
  * @brief  command reveived on link uart
  * @param  none
  * @retval none
  */
static cmd_status_t dc_generic_wb55_rec_cmd(uint8_t *cmd_line_p)
{
  PRINT_FORCE("%s\n\r", cmd_line_p);
  return CMD_OK;
}
#endif /* (USE_LINK_UART == 1) */

#endif /* USE_CMD_CONSOLE */

/**
  * @brief  component init
  * @param  none
  * @retval none
  */
void dc_generic_init(void)
{

  static dc_generic_bool_info_t    generic_bool1;          /* generic boolean value */
  static dc_generic_bool_info_t    generic_bool2;          /* generic boolean value */
  static dc_generic_bool_info_t    generic_bool3;          /* generic boolean value */
  static dc_generic_uint8_info_t   generic_byte1;          /* generic byte value    */
  static dc_generic_uint8_info_t   generic_byte2;          /* generic byte value    */
  static dc_generic_uint32_info_t  generic_long1;          /* generic unit32 value  */
  static dc_generic_uint32_info_t  generic_long2;          /* generic unit32 value  */
  static dc_generic_float_info_t   generic_float1;         /* generic float value   */
  static dc_generic_float_info_t   generic_float2;         /* generic float value   */
  static dc_generic_byte_table_t   generic_byte_table;     /* generic byte table    */

#if (USE_CMD_CONSOLE == 1)
#if (USE_LINK_UART == 1)
  (void)memset(dc_generic_device_table, 0, sizeof(dc_generic_device_table));
#endif /* USE_LINK_UART */
#endif /* USE_CMD_CONSOLE */

  /* initialize all generic DC entries to 0 */
  (void)memset((void *)&generic_bool1,  0, sizeof(dc_generic_bool_info_t));
  (void)memset((void *)&generic_bool2,  0, sizeof(dc_generic_bool_info_t));
  (void)memset((void *)&generic_bool3,  0, sizeof(dc_generic_bool_info_t));
  (void)memset((void *)&generic_byte1,  0, sizeof(dc_generic_uint8_info_t));
  (void)memset((void *)&generic_byte2,  0, sizeof(dc_generic_uint8_info_t));
  (void)memset((void *)&generic_long1,  0, sizeof(dc_generic_uint32_info_t));
  (void)memset((void *)&generic_long2,  0, sizeof(dc_generic_uint32_info_t));
  (void)memset((void *)&generic_float1, 0, sizeof(dc_generic_float_info_t));
  (void)memset((void *)&generic_float2, 0, sizeof(dc_generic_float_info_t));
  (void)memset((void *)&generic_byte_table, 0, sizeof(dc_generic_byte_table_t));

  /* registers generic boo1 entry in DC */
  DC_GENERIC_BOOL_1 = dc_com_register_serv(&dc_com_db,
                                           (void *)&generic_bool1,
                                           (uint16_t)sizeof(dc_generic_bool_info_t));

  /* registers generic boo2 entry in DC */
  DC_GENERIC_BOOL_2 = dc_com_register_serv(&dc_com_db,
                                           (void *)&generic_bool2,
                                           (uint16_t)sizeof(dc_generic_bool_info_t));

  /* registers generic boo2 entry in DC */
  DC_GENERIC_BOOL_3 = dc_com_register_serv(&dc_com_db,
                                           (void *)&generic_bool3,
                                           (uint16_t)sizeof(dc_generic_bool_info_t));

  /* registers generic byte1 entry in DC */
  DC_GENERIC_UINT8_1 = dc_com_register_serv(&dc_com_db,
                                            (void *)&generic_byte1,
                                            (uint16_t)sizeof(dc_generic_uint8_info_t));

  /* registers generic byte2 entry in DC */
  DC_GENERIC_UINT8_2 = dc_com_register_serv(&dc_com_db,
                                            (void *)&generic_byte2,
                                            (uint16_t)sizeof(dc_generic_uint8_info_t));

  /* registers generic long1 entry in DC */
  DC_GENERIC_UINT32_1 = dc_com_register_serv(&dc_com_db,
                                             (void *)&generic_long1,
                                             (uint16_t)sizeof(dc_generic_uint32_info_t));
  /* registers generic long2 entry in DC */
  DC_GENERIC_UINT32_2 = dc_com_register_serv(&dc_com_db,
                                             (void *)&generic_long2,
                                             (uint16_t)sizeof(dc_generic_uint32_info_t));

  /* registers generic float1 entry in DC */
  DC_GENERIC_FLOAT_1 = dc_com_register_serv(&dc_com_db,
                                            (void *)&generic_float1,
                                            (uint16_t)sizeof(dc_generic_float_info_t));

  /* registers generic float2 entry in DC */
  DC_GENERIC_FLOAT_2 = dc_com_register_serv(&dc_com_db,
                                            (void *)&generic_float2,
                                            (uint16_t)sizeof(dc_generic_float_info_t));

  /* registers generic byte tabe entry in DC */
  DC_GENERIC_BYTE_TABLE = dc_com_register_serv(&dc_com_db,
                                               (void *)&generic_byte_table,
                                               (uint16_t)sizeof(dc_generic_byte_table_t));


  /* set generic bool1 entry to invalid */
  generic_bool1.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_1, (void *)&generic_bool1, sizeof(dc_generic_bool_info_t));
  /* set generic bool2 entry to invalid */
  generic_bool2.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_2, (void *)&generic_bool2, sizeof(dc_generic_bool_info_t));
  /* set generic bool2 entry to invalid */
  generic_bool3.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_BOOL_3, (void *)&generic_bool3, sizeof(dc_generic_bool_info_t));

  /* set generic byte1 entry to invalid */
  generic_byte1.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT8_1, (void *)&generic_byte1, sizeof(dc_generic_uint8_info_t));
  /* set generic byte1 entry to invalid */
  generic_byte2.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT8_2, (void *)&generic_byte2, sizeof(dc_generic_uint8_info_t));

  /* set generic long1 entry to invalid */
  generic_long1.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT32_1, (void *)&generic_long1, sizeof(dc_generic_uint32_info_t));
  /* set generic long2 entry to invalid */
  generic_long2.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_UINT32_2, (void *)&generic_long2, sizeof(dc_generic_uint32_info_t));

  /* set generic float1 entry to invalid */
  generic_float1.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_FLOAT_1, (void *)&generic_float1, sizeof(dc_generic_float_info_t));
  /* set generic float2 entry to invalid */
  generic_float2.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_FLOAT_2, (void *)&generic_float2, sizeof(dc_generic_float_info_t));

  generic_byte_table.rt_state        =  DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_GENERIC_BYTE_TABLE, (void *)&generic_byte_table, sizeof(dc_generic_byte_table_t));

}

/**
  * @brief  component start
  * @param  none
  * @retval none
  */
void dc_generic_start(void)
{
  static uint8_t *dc_generic_wb55_rec_cmd_label = (uint8_t *)"+";
#if (USE_CMD_CONSOLE == 1)
  /* generic command record */
  CMD_Declare(dc_generic_cmd_label, dc_generic_cmd, (uint8_t *)"generic data cache entry management");
#if (USE_LINK_UART == 1)
  /* CES demo command record */
  CMD_Declare(dc_generic_wb55_cmd_label, dc_generic_wb55_cmd, (uint8_t *)"wb55 command");

  /* CES demo received command record */
  CMD_Declare(dc_generic_wb55_rec_cmd_label, dc_generic_wb55_rec_cmd, (uint8_t *)"wb55 rec command");

  /* CES generic command record */
  CMD_Declare(dc_generic_device_cmd_label, dc_generic_device_cmd, (uint8_t *)"device command");

  /* register to Data Cache events */
  (void)dc_com_register_gen_event_cb(&dc_com_db, dc_gen_notif_cb, (void *) NULL);
#endif /* USE_LINK_UART */
#endif /* USE_CMD_CONSOLE */
}

#endif /* (USE_DC_GENERIC == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
