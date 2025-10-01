/**
  ******************************************************************************
  * @file    comclient.c
  * @author  MCD Application Team
  * @brief   Interface Com usage
  *          Example of com_icc_generic_access usage
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#include "..\..\PKCS11\Test\stpkcs11_certify_example.h"
#include "custom_se_pkcs11.h"

#include "plf_config.h"

#if (USE_COM_CLIENT == 1)

#include <stdbool.h>

#include "comclient.h"

#include "rtosal.h"

#include "dc_common.h"
#include "cellular_datacache.h"

#include "error_handler.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

#if (USE_COM_ICC == 1)
#include "com_icc.h"
#endif /* USE_COM_ICC == 1 */


/* Private defines -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  COMCLIENT_SLEEP      = 0x0000, /* No cmd to test */
  COMCLIENT_ICC_ALWAYS = 0x0001, /* in case of ICC cmds - always done it */
  COMCLIENT_ICC_ICCID  = 0x0002, /* ICCID          */
  COMCLIENT_ICC_IMSI   = 0x0003, /* IMSI           */
  /* Add other ICC cmd */
  COMCLIENT_ICC_ERROR  = 0x000E, /* Error is expected           */
  COMCLIENT_ICC_FULL   = 0x000F, /* All ICC cmds will be tested */
  /* Add other xxx cmd */
  /*
  COMCLIENT_xxx_ALWAYS = 0x0010,
  COMCLIENT_xxx_yyy    = 0x0020,
  COMCLIENT_xxx_FULL   = 0x00F0,
  */
  COMCLIENT_FULL       = 0xFFFF /* All cmds will be tested */
} comclient_cmd_value_t;

/* e.g: COMCLIENT_ICC_ALWAYS, "Select MF", "00A4000C023F00", 4U */
typedef struct
{
  comclient_cmd_value_t cmd_value; /* when use the cmd    */
  uint8_t *p_comment;             /* comment to display  */
  uint8_t *p_cmd;                 /* cmd to send         */
  uint8_t rsp_len;                /* expected rsp length */
} comclient_cmd_desc_t; /* command description */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_COM_CLIENT == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_COMCLIENT, DBL_LVL_P0, "" format, ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_COMCLIENT, DBL_LVL_P0, "ComClt: " format, ## args)
#define PRINT_DBG(format, args...) \
  TRACE_PRINT(DBG_CHAN_COMCLIENT, DBL_LVL_P1, "ComClt: " format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#define PRINT_INFO(format, args...)  (void)printf("ComClt: " format, ## args);
#define PRINT_DBG(format, args...)   (void)printf("ComClt: " format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */
#else /* USE_TRACE_COM_CLIENT == 0 */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_COMCLIENT, DBL_LVL_P0, "" format, ## args)
#else
#include <stdio.h>
#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#endif  /* (USE_PRINTF == 0U) */
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_COM_CLIENT */

/* Is a command a ICC command ? */
#define COMCLIENT_IS_ICC_COMMAND(a) (((((uint32_t)(a))&((uint32_t)COMCLIENT_ICC_FULL)) \
                                      == ((uint32_t)COMCLIENT_SLEEP)) ? \
                                     false : true)
/* Is a command a xxx command ? */
/*
#define COMCLIENT_IS_xxx_COMMAND(a) (((a)&(COMCLIENT_xxx_FULL)) == COMCLIENT_SLEEP ? false : true)
*/

/* Private variables ---------------------------------------------------------*/
static osMessageQId comclient_queue;

/* State of ComClient process */
static comclient_cmd_value_t comclient_process_flag;      /* default value : COMCLIENT_SLEEP */
static comclient_cmd_value_t comclient_next_process_flag; /* default value : comclient_process_flag */

/* comclient icc handle */
static int32_t comclient_icc_handle;
/* comclient xxx handle if needed */
/* static int32_t comclient_xxx_handle; */

/* State of Network connection */
static bool comclient_network_is_on; /* Normally no need to wait network is on to do commands
                                        but maybe low-level not enough protected
                                        against concurrent access during modem boot */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Callback */
static void comclient_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);

static bool comclient_allocate_handle(void);
static void comclient_release_handle(void);
static bool comclient_do_command(comclient_cmd_value_t cmd_value,
                                 comclient_cmd_value_t process_value);
static bool comclient_was_error_expected(comclient_cmd_value_t cmd_value);
static void comclient_thread(void *p_argument);

#if (USE_CMD_CONSOLE == 1)
static void comclient_cmd_help(void);
static bool comclient_cmd_processing(void);
static cmd_status_t comclient_cmd(uint8_t *cmd_line_p);
#endif  /*  (USE_CMD_CONSOLE == 0) */

/* Private functions ---------------------------------------------------------*/

#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  help cmd management
  * @param  -
  * @retval -
  */
static void comclient_cmd_help(void)
{
  CMD_print_help((uint8_t *)"comclt");
  PRINT_APP("comclt help\n\r")
  PRINT_APP("comclt : if no command in progress,\n\r")
  PRINT_APP("         send the full predefined command sequence\n\r")
  PRINT_APP("         else the request is ignored\n\r")
#if (USE_COM_ICC == 1)
  PRINT_APP("comclt icc : if no command in progress,\n\r")
  PRINT_APP("             send the full predefined icc command sequence\n\r")
  PRINT_APP("             else the request is ignored\n\r")
  PRINT_APP("comclt icc imsi : if no command in progress,\n\r")
  PRINT_APP("                  send the predefined icc imsi command sequence\n\r")
  PRINT_APP("                  else the request is ignored\n\r")
  PRINT_APP("comclt icc iccid : if no command in progress,\n\r")
  PRINT_APP("                   send the predefined icc imsi command sequence\n\r")
  PRINT_APP("                   else the request is ignored\n\r")
#endif /* USE_COM_ICC == 1 */
  /* Add here xxx commands help */
}

/**
  * @brief  cmd management
  * @param  -
  * @retval true/false cmd can be executed / cmd can't be executed
  */
static bool comclient_cmd_processing(void)
{
  bool result;
  result = false;

  if (comclient_process_flag == COMCLIENT_SLEEP)
  {
    if (comclient_next_process_flag == COMCLIENT_SLEEP)
    {
      PRINT_APP("ComClt: Command requested ... \n\r")
      result = true;
    }
    else
    {
      PRINT_APP("ComClt: Command already requested ... \n\r")
    }
  }
  else
  {
    PRINT_APP("ComClt: Command already in progress ... \n\r")
  }

  return result;
}


/**
  * @brief  cmd management
  * @param  cmd_line_p - command parameters
  * @retval cmd_status_t - status of cmd management
  */
static cmd_status_t comclient_cmd(uint8_t *cmd_line_p)
{
  uint32_t argc;
  uint8_t  *argv_p[10];
  uint8_t  *cmd_p;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PRINT_APP("\n\r")
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (memcmp((CRC_CHAR_t *)cmd_p,
             "comclt",
             crs_strlen(cmd_p))
      == 0)
  {
    /* parameters parsing */
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
      if (comclient_cmd_processing() == true)
      {
        comclient_next_process_flag = COMCLIENT_FULL;
      }
    }
    /* 1st parameter analysis */
    /* ICC command ? */
    else if (memcmp((CRC_CHAR_t *)argv_p[0],
                    "icc",
                    crs_strlen(argv_p[0])) == 0)
    {
#if (USE_COM_ICC == 1)
      if (argc == 1U)
      {
        if (comclient_cmd_processing() == true)
        {
          comclient_next_process_flag = COMCLIENT_ICC_FULL;
        }
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[1],
                      "imsi",
                      crs_strlen(argv_p[1])) == 0)
      {
        if (comclient_cmd_processing() == true)
        {
          comclient_next_process_flag = COMCLIENT_ICC_IMSI;
        }
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[1],
                      "iccid",
                      crs_strlen(argv_p[1])) == 0)
      {
        if (comclient_cmd_processing() == true)
        {
          comclient_next_process_flag = COMCLIENT_ICC_ICCID;
        }
      }
      else
      {
        PRINT_APP("ComClt: %s bad parameter %s !!!\n\r", cmd_p, argv_p[1])
        cmd_status = CMD_SYNTAX_ERROR;
        comclient_cmd_help();
      }
#else /* USE_COM_ICC == 0 */
      PRINT_APP("ComClt: %s bad parameter: option %s not embedded !!!\n\r", cmd_p, argv_p[0])
      cmd_status = CMD_SYNTAX_ERROR;
      comclient_cmd_help();
#endif /* USE_COM_ICC == 1 */
    }
    /*  xxx Command ? */

    else if (memcmp((CRC_CHAR_t *)argv_p[0],
                    "test",
                    crs_strlen(argv_p[0])) == 0)
    {
    	certifyApiExample();
    }

    else if (memcmp((CRC_CHAR_t *)argv_p[0],
                    "help",
                    crs_strlen(argv_p[0])) == 0)
    {
      comclient_cmd_help();
    }
    else
    {
      PRINT_APP("ComClt: %s bad parameter %s !!!\n\r", cmd_p, argv_p[0])
      cmd_status = CMD_SYNTAX_ERROR;
      comclient_cmd_help();
    }
  }

  return cmd_status;
}
#endif /* USE_CMD_CONSOLE */

/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  p_private_gui_data - value provided at service subscription
  * @note   Unused parameter
  * @retval -
  */
static void comclient_notif_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);

  if (dc_event_id == DC_CELLULAR_NIFMAN_INFO)
  {
    dc_nifman_info_t  dc_nifman_info;
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO,
                      (void *)&dc_nifman_info,
                      sizeof(dc_nifman_info));
    if (dc_nifman_info.rt_state == DC_SERVICE_ON)
    {
      PRINT_APP("ComClt: Network is up\n\r")
      comclient_network_is_on = true;
      (void)rtosalMessageQueuePut(comclient_queue, (uint32_t)dc_event_id, 0U);
    }
    else
    {
      PRINT_APP("ComClt: Network is down\n\r")
      comclient_network_is_on = false;
    }
  }
  else
  {
    /* Nothing to do */
    __NOP();
  }
}

/**
  * @brief  Check if a command has to be done or not
  * @param  cmd_value - command to ckeck
  * @param  process_value - process requested
  * @retval bool true/false command has to be done or not
  */
static bool comclient_do_command(comclient_cmd_value_t cmd_value,
                                 comclient_cmd_value_t process_value)
{
  bool result;

  if (cmd_value == COMCLIENT_SLEEP)
  {
    result = false;
  }
  else if ((process_value == COMCLIENT_FULL)   /* Do all commands ? */
           || (process_value == cmd_value))    /* Do this command ? */
  {
    result = true;
  }
  else     /* Do the command according to its family ? */
  {
    /* Command is a ICC family ? */
    if ((COMCLIENT_IS_ICC_COMMAND(cmd_value)) == true) /* Command is a ICC cmd */
    {
      if ((process_value == COMCLIENT_ICC_FULL)   /* and must test all ICC commands */
          || (cmd_value == COMCLIENT_ICC_ALWAYS)) /* or this ICC cmd must always be done */
      {
        result = true;
      }
      else
      {
        result = false;
      }
    }
    /* Command is a xxx family ? */
    /*
    else if (COMCLIENT_IS_xxx_COMMAND(process_value) == true)
    {
    }
    */
    else /* Command is not in a known family */
    {
      result = false;
    }
  }

  return (result);
}

/**
  * @brief  Check if a command has to be done or not
  * @param  cmd_value - command to ckeck
  * @retval bool true/false command has to be done or not
  */
static bool comclient_was_error_expected(comclient_cmd_value_t cmd_value)
{
  bool result;

  /* Command is a ICC family ? */
  if ((cmd_value == COMCLIENT_ICC_ERROR) /* Command is a ICC error */
      /* || (cmd_value == COMCLIENT_xxx_ERROR) */
     )
  {
    result = true;
  }
  else
  {
    result = false;
  }

  return (result);
}

/**
  * @brief  Allocate needed handles
  * @param  -
  * @retval bool true/false resource needed allocated / issue during allocation
  */
static bool comclient_allocate_handle(void)
{
  bool result;

  result = true; /* If a resource is missing result will be set to false */

  /* Allocate ICC handle */
  if ((COMCLIENT_IS_ICC_COMMAND(comclient_process_flag) == true)
      && (comclient_icc_handle == COM_HANDLE_INVALID_ID))
  {
#if (USE_COM_ICC == 1)
    PRINT_INFO("icc handle creation\n\r")
    comclient_icc_handle = com_icc(COM_AF_UNSPEC, COM_SOCK_SEQPACKET, COM_PROTO_CSIM);
#else /* USE_COM_ICC == 0 */
    comclient_icc_handle = 1;
#endif /* USE_COM_ICC == 1 */
  }
  /* Allocate xxx handle */
  /*
  if ((COMCLIENT_IS_xxx_COMMAND(comclient_process_flag) == true)
      && (comclient_xxx_handle == COM_HANDLE_INVALID_ID))
  {
    PRINT_INFO("xxx handle creation\n\r")
  }
  */

  /* Check that needed resources are available */
  /* Check that ICC resource is available */
  if ((COMCLIENT_IS_ICC_COMMAND(comclient_process_flag) == true)
      && (comclient_icc_handle == COM_HANDLE_INVALID_ID))
  {
    result = false;
    PRINT_INFO("icc handle creation NOK\n\r")
  }
  /* Check that xxx resource is available */
  /*
  if ((COMCLIENT_IS_xxx_COMMAND(comclient_process_flag) == true)
      && (comclient_xxx_handle == COM_HANDLE_INVALID_ID))
  {
    result = false;
    PRINT_INFO("xxx handle creation NOK\n\r")
  }
  */

  /* Is the command known ? */
  if (COMCLIENT_IS_ICC_COMMAND(comclient_process_flag) == false)
    /* &&(COMCLIENT_IS_xxx_COMMAND(comclient_process_flag) == false) */
  {
    result = false;
  }
  PRINT_APP("\n\r")

  return (result);
}

/**
  * @brief Release handles
  * @param  -
  * @retval -
  */
static void comclient_release_handle(void)
{
  /* Release ICC handle */
  if (comclient_icc_handle != COM_HANDLE_INVALID_ID)
  {
#if (USE_COM_ICC == 1)
    if (com_closeicc(comclient_icc_handle) == COM_ERR_OK)
    {
      comclient_icc_handle = COM_HANDLE_INVALID_ID;
      PRINT_INFO("icc handle release OK\n\r")
    }
#else /* USE_COM_ICC == 0 */
    comclient_icc_handle = COM_HANDLE_INVALID_ID;
#endif /* USE_COM_ICC == 1 */
  }
  /* Release xxx handle */
  /*
  if (comclient_xxx_handle != COM_HANDLE_INVALID_ID)
  {
    comclient_xxx_handle = COM_HANDLE_INVALID_ID;
    PRINT_INFO("xxx handle release OK\n\r")
  }
  */
}

/**
  * @brief  ComClient thread
  * @note   Infinite loop body
  * @param  p_argument - parameter osThread
  * @note   Unused parameter
  * @retval -
  */
static void comclient_thread(void *p_argument)
{
#if (USE_COM_ICC == 1)
#define COMCLIENT_COMMAND_ICC_ARRAY_NB 10U
#else /* USE_COM_ICC == 0 */
#define COMCLIENT_COMMAND_ICC_ARRAY_NB  0U
#endif /* USE_COM_ICC == 1 */
#define COMCLIENT_COMMAND_ARRAY_NB (1U + COMCLIENT_COMMAND_ICC_ARRAY_NB)
  static const comclient_cmd_desc_t cmd_desc[COMCLIENT_COMMAND_ARRAY_NB] =
  {
    {COMCLIENT_SLEEP, ((uint8_t *)""), ((uint8_t *)""), 0U}, /* define to empty in order to not have an array null */
#if (USE_COM_ICC == 1)
    /* Begin ICC interaction */
    {COMCLIENT_ICC_ALWAYS, ((uint8_t *)"Select Master File"), ((uint8_t *)"00A4000C023F00"), 4U},
    /* Begin ICCID */
    {COMCLIENT_ICC_ICCID, ((uint8_t *)"Select EF ICCID"), ((uint8_t *)"00A4000C022FE2"), 4U},
    {COMCLIENT_ICC_ICCID, ((uint8_t *)"Read Binary ICCID"), ((uint8_t *)"00B000000A"), (20U + 4U)},
    /* End   ICCID */
    /* Begin IMSI */
    {COMCLIENT_ICC_IMSI, ((uint8_t *)"Select DF GSM"), ((uint8_t *)"00A4000C027F20"), 4U},
    {COMCLIENT_ICC_IMSI, ((uint8_t *)"Select EF IMSI"), ((uint8_t *)"00A4000C026F07"), 4U},
    {COMCLIENT_ICC_IMSI, ((uint8_t *)"Read Binary IMSI"), ((uint8_t *)"00B0000009"), (18U + 4U)},
    /* End   IMSI */
    /* Begin Large return */
    {COMCLIENT_ICC_FULL, ((uint8_t *)"Select Master File"), ((uint8_t *)"00A4000C023F00"), 4U},
    {COMCLIENT_ICC_ERROR, ((uint8_t *)"Select Applet"), ((uint8_t *)"00A40400051122334455"), 4U},
    /* End   Large return */
    /* Begin Large return */
    {COMCLIENT_ICC_FULL, ((uint8_t *)"Select Master File"), ((uint8_t *)"00A4000C023F00"), 4U},
    {COMCLIENT_ICC_FULL, ((uint8_t *)"Status MF"), ((uint8_t *)"80F2000016"), (255U)} /* rsp length is variable */
    /* End   Large return */
    /* End   ICC interaction */

    /* Begin xxx interaction */
    /* Begin yyy */
    /* End   yyy */
    /* End   xxx interaction */
#endif /* USE_COM_ICC == 1 */
  };

  uint32_t msg_queue = 0U; /* Msg received from the queue */

  UNUSED(p_argument);

  for (;;)
  {
    /* Wait network is up to do something */
    (void)rtosalMessageQueueGet(comclient_queue, &msg_queue, RTOSAL_WAIT_FOREVER);

    (void)rtosalDelay(1000U); /* just to let apps to show menu */

    /* Release ComClient handle */
    comclient_release_handle();

    while (comclient_network_is_on == true)
    {
      /* Update comclient_process_flag value */
      comclient_process_flag = comclient_next_process_flag; /* comclient_process_value could also be updated
                                                             by the value put in message queue */

      if (comclient_process_flag == COMCLIENT_SLEEP)
      {
        /* Release ComClient handle */
        comclient_release_handle();
        /* Could also be managed wait receiving a message in message queue */
        (void)rtosalDelay(1000U);
      }
      else
      {
        PRINT_APP("\n\r<<< ComClt command in progress >>>\n\r")

        if (comclient_allocate_handle() == true)
        {
          uint8_t buf_cmd[150];
#if (USE_COM_ICC == 1)
          uint8_t buf_rsp[75];        /* used to receive a long response */
          uint8_t buf_rsp_sw1_sw2[5]; /* used to receive SW1 and SW2 status only */
#endif /* USE_COM_ICC == 1 */
          bool exit;      /* In case of error stop to scan the commands */
          uint8_t i;      /* Scan the whole array of the commands */
          int32_t result; /* Result of the command process
                             < 0 : error in the command execution
                             = 0 : command not done
                             > 0 : command done
                           */
          uint32_t cmd_len; /* command length  */
#if (USE_COM_ICC == 1)
          uint32_t rsp_len; /* response length */
          uint8_t *p_buf_rsp;
#endif /* USE_COM_ICC == 1 */

          exit = false;
          i = 0U;

          /* Scan all commands array */
          while ((i < COMCLIENT_COMMAND_ARRAY_NB)
                 && (exit == false))
          {
            if (comclient_do_command(cmd_desc[i].cmd_value, comclient_process_flag)
                == true)
            {
              cmd_len = crs_strlen((const uint8_t *)(cmd_desc[i].p_cmd));
              (void)memcpy(buf_cmd,
                           cmd_desc[i].p_cmd,
                           cmd_len + 1U); /* +1 to copy \0 */

              PRINT_INFO("\n\rSend Cmd%d: %.*s length:%ld cmd:%.*s\n\r",
                         i,
                         (int16_t)crs_strlen((const uint8_t *)(cmd_desc[i].p_comment)),
                         (CRC_CHAR_t *)(cmd_desc[i].p_comment),
                         cmd_len,
                         (int16_t)cmd_len,
                         (CRC_CHAR_t *)(cmd_desc[i].p_cmd))

              /* ICC command process */
              if (COMCLIENT_IS_ICC_COMMAND(cmd_desc[i].cmd_value) == true)
              {
#if (USE_COM_ICC == 1)
                /* Are SW1 and SW2 only expected ? */
                if (cmd_desc[i].rsp_len == 4U)
                {
                  p_buf_rsp = &buf_rsp_sw1_sw2[0];
                  rsp_len = sizeof(buf_rsp_sw1_sw2);
                }
                else
                {
                  p_buf_rsp = &buf_rsp[0];
                  rsp_len = sizeof(buf_rsp);
                }

                result = com_icc_generic_access(comclient_icc_handle,
                                                buf_cmd, (int32_t)cmd_len,
                                                p_buf_rsp, (int32_t)rsp_len);
#else /* USE_COM_ICC == 0 */
                result = 0;
#endif /* USE_COM_ICC == 1 */
              }
              /* xxx command process */
              /*
              else if (COMCLIENT_IS_xxx_COMMAND(cmd_desc[i].cmd_value) == true)
              {
              }
              */
              else /* Don't know how to proceed command */
              {
                result = 0;
#if (USE_COM_ICC == 1)
                rsp_len = 0;
                p_buf_rsp = NULL;
#endif /* USE_COM_ICC == 1 */
              }

              /* Command send and result OK */
              if (result > 0)
              {
                if (COMCLIENT_IS_ICC_COMMAND(cmd_desc[i].cmd_value) == true)
                {
#if (USE_COM_ICC == 1)
                  /* Response complete ? */
                  if (((uint32_t)result) < rsp_len) /* and not <= because '\0' take 1 byte in p_buf_rsp */
                  {
                    /* Response len OK ? */
                    if ((((uint32_t)result) == cmd_desc[i].rsp_len)
                        /* Response length is variable */
                        || ((cmd_desc[i].rsp_len == 255U) && ((uint32_t)result >= 4U)))
                    {
                      if (p_buf_rsp[(uint8_t)result - 4U] == ((uint8_t)'9'))
                      {
                        PRINT_INFO("Cmd%d rsp received OK: length:%ld rsp:%.*s\n\r",
                                   i, (uint32_t)result, (int16_t)result, p_buf_rsp)
                      }
                      else
                      {
                        if (comclient_was_error_expected(cmd_desc[i].cmd_value) == true)
                        {
                          PRINT_INFO("Cmd%d rsp ERROR on status was expected: length:%ld rsp:%.*s\n\r",
                                     i, (uint32_t)result, (int16_t)result, p_buf_rsp)
                        }
                        else
                        {
                          PRINT_INFO("Cmd%d rsp received ERROR on status: length:%ld rsp:%.*s\n\r",
                                     i, (uint32_t)result, (int16_t)result, p_buf_rsp)
                          exit = true;
                        }
                      }
                    }
                    else
                    {
                      PRINT_INFO("Cmd%d rsp received ERROR on length: length:%ld rsp:%.*s\n\r",
                                 i, (uint32_t)result, (int16_t)result, p_buf_rsp)
                      exit = true;
                    }
                  }
                  else
                  {
                    PRINT_INFO("Cmd%d rsp received ERROR rsp cut: full rsp lgth:%ld buffer lgth:%ld rsp:%.*s\n\r",
                               i, (uint32_t)result, rsp_len, (int16_t)rsp_len, p_buf_rsp)
                    exit = true;
                  }
#else /* USE_COM_ICC == 0 */
                  __NOP();
#endif  /* USE_COM_ICC == 1 */
                }
                /* xxx command process */
                /*
                else if (COMCLIENT_IS_xxx_COMMAND(cmd_desc[i].cmd_value) == true)
                {
                }
                */
                else
                {
                  PRINT_INFO("Cmd%d rsp OK but don't know how to analyze it: result:%ld\n\r",
                             i, (uint32_t)result)
                }
              }
              /* Error in the command */
              else if (result < 0)
              {
                if (comclient_was_error_expected(cmd_desc[i].cmd_value) == true)
                {
                  PRINT_INFO("Cmd%d rsp ERROR on status was expected\n\r", i)
                }
                else
                {
                  PRINT_INFO("Cmd%d rsp received ERROR on status\n\r", i)
                  exit = true;
                }
              }
              else /* result == 0 command not proceed */
              {
                __NOP();
              }
            }

            /* Go to next command */
            i++;
          }

          if (exit == false)
          {
            PRINT_INFO("Send COM OK\n\r")
            TRACE_VALID("@valid@:comclient:ok\n\r")
          }
          else
          {
            PRINT_INFO("Send COM NOK\n\r")
            TRACE_VALID("@valid@:comclient:nok\n\r")
          }

          PRINT_APP("\n\r<<< ComClt command finished >>>\n\r")

          /* Release ComClient handle */
          comclient_release_handle();

          /* Return to Sleep mode */
          comclient_process_flag = COMCLIENT_SLEEP;
          comclient_next_process_flag = COMCLIENT_SLEEP;
        }
        else
        {
          /* ComClient handle not received */
          PRINT_INFO("low-level not ready - Wait before to try again\n\r")
          (void)rtosalDelay(1000U);
        }
      }
    }
  }
}

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialization
  * @note   ComClient initialization
  * @param  -
  * @retval -
  */
void comclient_init(void)
{
  /* ComClient initialization */
  /* ICC handle initialization */
  comclient_icc_handle = COM_HANDLE_INVALID_ID;
  /* xxx handle initialization */
  /* comclient_xxx_handle = COM_HANDLE_INVALID_ID; */

  /* ComClient deactivated by default */
  comclient_process_flag = COMCLIENT_SLEEP;
  comclient_next_process_flag = comclient_process_flag;

  /* Create message queue used to exchange information between callback and comclient thread */
  comclient_queue = rtosalMessageQueueNew(NULL, 1U);
  if (comclient_queue == NULL)
  {
    ERROR_Handler(DBG_CHAN_COMCLIENT, 1, ERROR_FATAL);
  }
}

/**
  * @brief  Start
  * @note   ComClient start
  * @param  -
  * @retval -
  */
void comclient_start(void)
{
  static osThreadId ComClientTaskHandle;

  /* Registration to datacache - for Network On/Off and Button */
  (void)dc_com_register_gen_event_cb(&dc_com_db, comclient_notif_cb, (void *) NULL);

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare((uint8_t *)"comclt", comclient_cmd, (uint8_t *)"comclt commands");
#endif  /*  (USE_CMD_CONSOLE == 1) */

  /* Create ComClient thread  */
  ComClientTaskHandle = rtosalThreadNew((const rtosal_char_t *)"ComCltThread", (os_pthread)comclient_thread,
                                        COMCLIENT_THREAD_PRIO, USED_COMCLIENT_THREAD_STACK_SIZE, NULL);

  if (ComClientTaskHandle == NULL)
  {
    ERROR_Handler(DBG_CHAN_COMCLIENT, 2, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    (void)stackAnalysis_addStackSizeByHandle(ComClientTaskHandle, USED_COMCLIENT_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

#endif /* USE_COM_CLIENT == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
