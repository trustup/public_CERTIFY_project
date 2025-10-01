/**
  ******************************************************************************
  * @file    cmd.c
  * @author  MCD Application Team
  * @brief   console cmd management
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "plf_config.h"
#include "cmd.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "usart.h"
#include "rtosal.h"
#include "error_handler.h"

#if (USE_LINK_UART == 1)
#include "dc_generic.h"
#endif  /* (USE_LINK_UART == 1) */

#if (USE_CMD_CONSOLE == 1)

/* Private defines -----------------------------------------------------------*/

#if !defined CUSTOM_CLIENT_CMD
#define CUSTOM_CLIENT_CMD        (1U) /* no custom client command usage */
#endif /* !defined CUSTOM_CLIENT_CMD */

#define CMD_MAX_CMD         (22U + (CUSTOM_CLIENT_CMD))   /* number max of recorded components */
#define CMD_MAX_LINE_SIZE   100U                          /* maximum size of command           */
#define CMD_READMEM_LINE_SIZE_MAX   256U                  /* maximum size of memory read       */
#define CMD_COMMAND_ALIGN_COLUMN    16U                   /* alignment size to display component description */


/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_UTILITIES, DBL_LVL_P0, "" format "", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)   (void)printf("" format "", ## args);
#endif  /* (USE_PRINTF == 0U) */

/* Private typedef -----------------------------------------------------------*/
/* structure to record registered components */
typedef struct
{
  uint8_t         *CmdName;             /* header command of component */
  uint8_t         *CmdLabel;            /* component description       */
  CMD_HandlerCmd  CmdHandler;
} CMD_Struct_t;

/* Private variables ---------------------------------------------------------*/

static uint8_t             CMD_ReceivedChar;                       /* char to receive from UART */

static CMD_Struct_t        CMD_a_cmd_list[CMD_MAX_CMD];            /* list of recorded components */
static uint8_t             CMD_LastCommandLine[CMD_MAX_LINE_SIZE]; /* last command reveived       */
static uint8_t             CMD_CommandLine[2][CMD_MAX_LINE_SIZE];  /* current command receiving    */
static osSemaphoreId       CMD_rcvSemaphore  = 0U;
static uint32_t            CMD_NbCmd         = 0U;       /* Number of recorded components */

static uint8_t  *CMD_current_cmd;                        /* pointer on current received command  */
static uint8_t  *CMD_current_rcv_line;                   /* pointer on current receiving command */
static uint8_t  *CMD_completed_line;
static uint32_t  CMD_CurrentPos    = 0U;

#if (USE_LINK_UART == 1)
static uint8_t  *CMD_link_current_rcv_line;
static uint8_t  *CMD_link_completed_line;
static uint8_t  CMD_LinkReceivedChar;
#endif /* (USE_LINK_UART == 1) */
/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void cmd_thread(const void *argument);
static void CMD_process(void);
static void CMD_GetLine(uint8_t *command_line, uint32_t max_size);
static void CMD_BoardReset(uint8_t *p_Cmd_p);
static cmd_status_t CMD_Help(uint8_t *p_Cmd_p);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief board reset command management
  * @param  p_Cmd_p command (not used because not parameter for this command)
  * @retval -
  */
static void CMD_GetLine(uint8_t *command_line, uint32_t max_size)
{
  uint32_t size;
  (void)rtosalSemaphoreAcquire(CMD_rcvSemaphore, RTOSAL_WAIT_FOREVER);

  size = crs_strlen(CMD_current_cmd) + 1U;
  if (max_size < size)
  {
    size = max_size;
  }

  (void)memcpy((CRC_CHAR_t *)command_line, (CRC_CHAR_t *)CMD_current_cmd, size);
}

void CMD_SetLine(uint8_t *command_line)
{
  (void)memcpy((CRC_CHAR_t *)CMD_current_cmd, (CRC_CHAR_t *)command_line, crs_strlen(command_line) + 1U);
  (void)rtosalSemaphoreRelease(CMD_rcvSemaphore);
}


/**
  * @brief  command processing
  * @note   this function find the component associated to the command (among the recorded components)
  * @param  -
  * @retval return value
  */
static void CMD_process(void)
{
  static uint8_t             *cmd_prompt = (uint8_t *)"$>";           /* command prompt to display */
  uint8_t  command_line[CMD_MAX_LINE_SIZE];
  uint32_t i;
  uint32_t cmd_size;

  uint32_t cmd_line_len;

  /* get command line */
  CMD_GetLine(command_line, CMD_MAX_LINE_SIZE);
  if (command_line[0] != (uint8_t)'#')
  {
    /* not a comment line    */
    if (command_line[0] == 0U)
    {
      if (CMD_LastCommandLine[0] == 0U)
      {
        /* no last command: display help  */
        (void)memcpy((CRC_CHAR_t *)command_line, (CRC_CHAR_t *)"help", crs_strlen((const uint8_t *)"help") + 1U);
      }
      else
      {
        /* execute again last command  */
        /* No memory overflow: sizeof(command_line) == sizeof(CMD_LastCommandLine) */
        (void)memcpy((CRC_CHAR_t *)command_line, (CRC_CHAR_t *)CMD_LastCommandLine,
                     crs_strlen(CMD_LastCommandLine) + 1U);
      }
    }
    else
    {
      cmd_line_len = crs_strlen(command_line);
      if (cmd_line_len > 1U)
      {
        /* store last command             */
        /* No memory overflow: sizeof(command_line) == sizeof(CMD_LastCommandLine) */
        (void)memcpy((CRC_CHAR_t *)CMD_LastCommandLine, (CRC_CHAR_t *)command_line, crs_strlen(command_line) + 1U);
      }
    }

    /* command analysis                     */
    for (i = 0; i < CMD_MAX_LINE_SIZE ;  i++)
    {
      if ((command_line[i] == (uint8_t)' ') || (command_line[i] == (uint8_t)0))
      {
        break;
      }
    }

    cmd_size = i;

    if (memcmp((CRC_CHAR_t *)"reset", (CRC_CHAR_t *)command_line, cmd_size) == 0)
    {
      CMD_BoardReset((uint8_t *)command_line);
    }
    else if (i != CMD_MAX_LINE_SIZE)
    {
      /* not an empty line        */
      for (i = 0U; i < CMD_NbCmd ; i++)
      {

        if (memcmp((CRC_CHAR_t *)CMD_a_cmd_list[i].CmdName, (CRC_CHAR_t *)command_line, cmd_size) == 0)
        {
          /* Command  found => call processing  */
          PRINT_FORCE("\r\n")
          (void)CMD_a_cmd_list[i].CmdHandler((uint8_t *)command_line);
          break;
        }
      }
      if (i >= CMD_NbCmd)
      {
        /* unknown command   */
        PRINT_FORCE("\r\nCMD : unknown command : %s\r\n", command_line)
        (void)CMD_Help(command_line);
      }
    }
    else
    {
      __NOP(); /* Nothing to do */
    }
  }
  else
  {
    PRINT_FORCE("\r\n")
  }
  PRINT_FORCE("%s", (CRC_CHAR_t *)cmd_prompt)
}




#if (USE_LINK_UART == 1)
/**
  * @brief  transmit the command to the link UART
  * @note   this command allow to redirect a command on the link UART
  * @param  cmd_line_p   command to redirect
  * @retval return value
  */
static cmd_status_t CMD_UartCmd(uint8_t *cmd_line_p)
{
  /*_VAR_*/

  uint32_t i;
  uint8_t *ptr;
  ptr = cmd_line_p;
  uint16_t len;
  uint16_t size;
  len = (uint16_t)strlen((CRC_CHAR_t *)(cmd_line_p));
  size = len;
  PRINT_FORCE("uartcmd: %s\r\n", cmd_line_p);

  /* remove 'uartcmd' pattern from the beginning of the command to send */
  for (i = 0U; i < len; i++)
  {
    if (*ptr == (uint8_t)' ')
    {
      ptr++;
      size--;
      break;
    }
    ptr++;
    size--;
  }

  if (size > 0U)
  {
    /* adding end of line delimiter */
    ptr[size] = (uint8_t)'\r';
    size++;
    (void)HAL_UART_Transmit(&COM_INTERFACE_UART_HANDLE, (uint8_t *)ptr, size, 0xFFFFU);
  }
  return CMD_OK;
}
#endif  /* (USE_LINK_UART == 1) */

/**
  * @brief board reset command management
  * @param  p_Cmd_p command (not used because not parameter for this command)
  * @retval -
  */
static void CMD_BoardReset(uint8_t *p_Cmd_p)
{
  UNUSED(p_Cmd_p);
  PRINT_FORCE("Board reset requested !\r\n");
  (void)rtosalDelay(1000);

  /* display declared commands  */
  NVIC_SystemReset();
  /* NVIC_SystemReset never return  */
}

/**
  * @brief help command management
  * @note  display all recorded component (component command header and description)
  * @param  -
  * @retval return value
  */
static cmd_status_t CMD_Help(uint8_t *p_Cmd_p)
{
  UNUSED(p_Cmd_p);
  uint32_t i;
  uint32_t align_offset;
  uint32_t cmd_size;
  PRINT_FORCE("***** help *****\r\n");

  PRINT_FORCE("\r\nList of commands\r\n")
  PRINT_FORCE("----------------\r\n")
  uint8_t   CMD_CmdAlignOffsetString[CMD_COMMAND_ALIGN_COLUMN];

  /* display registered commands  */
  for (i = 0U; i < CMD_NbCmd ; i++)
  {
    cmd_size = (uint32_t)crs_strlen(CMD_a_cmd_list[i].CmdName);
    align_offset = CMD_COMMAND_ALIGN_COLUMN - cmd_size;
    if ((align_offset < CMD_COMMAND_ALIGN_COLUMN))
    {
      /* alignment of the component descriptions */
      (void)memset(CMD_CmdAlignOffsetString, (int32_t)' ', align_offset);
      CMD_CmdAlignOffsetString[align_offset] = 0U;
    }
    PRINT_FORCE("%s%s %s\r\n", CMD_a_cmd_list[i].CmdName, CMD_CmdAlignOffsetString, CMD_a_cmd_list[i].CmdLabel);
  }

  /* display general syntax of the commands */
  PRINT_FORCE("\r\nHelp syntax\r\n");
  PRINT_FORCE("-----------\r\n");
  PRINT_FORCE("warning: case sensitive commands\r\n");
  PRINT_FORCE("[optional parameter]\r\n");
  PRINT_FORCE("<parameter value>\r\n");
  PRINT_FORCE("<val_1>|<val_2>|...|<val_n>: parameter value list\r\n");
  PRINT_FORCE("(command description)\r\n");
  PRINT_FORCE("return key: last command re-execution\r\n");
  PRINT_FORCE("#: comment line\r\n");
  PRINT_FORCE("\r\nAdvice\r\n");
  PRINT_FORCE("-----------\r\n");
  PRINT_FORCE("to use commands it is advised to use one of the following command to disable traces\r\n");
  PRINT_FORCE("trace off (allows disable all traces)\r\n");
  PRINT_FORCE("cst polling off  (allows to disable modem polling and avoid to display uncomfortable modem traces\r\n");
  PRINT_FORCE("\r\n");

  return CMD_OK;
}


/**
  * @brief thread core of the command management
  * @param  argument (not used)
  * @retval -
  */
static void cmd_thread(const void *argument)
{
  for (;;)
  {
    CMD_process();
  }
}


/* -------------------------*/
/* External functions       */
/* -------------------------*/
/**
  * @brief  get an integer value from the argument
  * @param  string_p   (IN) acscii value to convert
  * @param  value_p    (OUT) converted uint32_t value
  * @retval return value
  */
uint32_t CMD_GetValue(uint8_t *string_p, uint32_t *value_p)
{
  uint32_t ret;
  uint8_t digit8;
  uint32_t digit;
  ret = 0U;

  if (string_p == NULL)
  {
    ret = 1U;
    *value_p = 0U;
  }
  else
  {
    if (memcmp((CRC_CHAR_t *)string_p, "0x", 2U) == 0)
    {
      *value_p = (uint32_t)crs_atoi_hex(&string_p[2]);
    }
    else
    {
      digit8 = (*string_p - (uint8_t)'0');
      digit  = (uint32_t)digit8;
      if (digit <= 9U)
      {
        *value_p = (uint32_t)crs_atoi(string_p);
      }
      else
      {
        ret = 1U;
        *value_p = 0U;
      }
    }
  }
  return ret;
}

/**
  * @brief register a component
  * @param  cmd_name_p     command header of the component
  * @param  cmd_handler    callback of the component to manage the command
  * @param  cmd_label_p    description of the component to display at the help  command
  * @retval -
  */
void CMD_Declare(uint8_t *cmd_name_p, CMD_HandlerCmd cmd_handler, uint8_t *cmd_label_p)
{
  if (CMD_NbCmd >= CMD_MAX_CMD)
  {
    /* too many recorded components */
    ERROR_Handler(DBG_CHAN_UTILITIES, 10, ERROR_WARNING);
  }
  else
  {
    CMD_a_cmd_list[CMD_NbCmd].CmdName    = cmd_name_p;
    CMD_a_cmd_list[CMD_NbCmd].CmdLabel   = cmd_label_p;
    CMD_a_cmd_list[CMD_NbCmd].CmdHandler = cmd_handler;

    CMD_NbCmd++;
  }
}

/**
  * @brief console UART receive IT Callback
  * @param  uart_handle_p       console UART handle
  * @retval -
  */
void CMD_RxCpltCallback(UART_HandleTypeDef *uart_handle_p)
{
  static UART_HandleTypeDef *CMD_CurrentUart;

  CMD_CurrentUart = uart_handle_p;
  uint8_t rec_char;
  uint8_t *temp;

  /* store the received char */
  rec_char = CMD_ReceivedChar;

  /* rearm the IT  receveive for the next char */
  if (HAL_UART_Receive_IT(CMD_CurrentUart, (uint8_t *)&CMD_ReceivedChar, 1U) != HAL_OK)
  {
    __NOP(); /* Nothing to do */
  }

  /* ignore '\n' char */
  if (rec_char != (uint8_t)'\n')
  {
    if ((rec_char == (uint8_t)'\r') || (CMD_CurrentPos >= (CMD_MAX_LINE_SIZE - 1U)))
    {
      /* end of line reached: switch between reveived buffer and receiving buffer */
      CMD_current_rcv_line[CMD_CurrentPos] = 0;
      temp = CMD_completed_line;
      CMD_completed_line = CMD_current_rcv_line;
      CMD_current_cmd    = CMD_completed_line;
      CMD_current_rcv_line = temp;
      CMD_CurrentPos = 0;
      (void)rtosalSemaphoreRelease(CMD_rcvSemaphore);
    }
    else
    {
      /* not the end of line */
      if (rec_char == (uint8_t)'\b')
      {
        /* back space */
        if (CMD_CurrentPos > 0U)
        {
          /* remove the last char received only if the receiving buffer is not empty */
          CMD_CurrentPos--;
        }
      }
      else
      {
        /* normal char  */
        CMD_current_rcv_line[CMD_CurrentPos] = rec_char;
        CMD_CurrentPos++;
      }
    }
  }
}

#if (USE_LINK_UART == 1)
/**
  * @brief link UART receive IT Callback
  * @param  uart_handle_p      link UART handle
  * @retval -
  */
void CMD_RxLinkCpltCallback(UART_HandleTypeDef *uart_handle_p)
{
  static uint32_t  CMD_LinkCurrentPos  = 0U;   /* position of last received char in the current line */
  static UART_HandleTypeDef *CMD_CurrentUart;  /* current UART */

  CMD_CurrentUart = uart_handle_p;
  uint8_t rec_char;
  uint8_t *temp;
  HAL_StatusTypeDef ret;

  /* store the received char */
  rec_char = CMD_LinkReceivedChar;

  /* rearm the IT  receveive for the next char */
  ret = HAL_UART_Receive_IT(CMD_CurrentUart, (uint8_t *)&CMD_LinkReceivedChar, 1U);
  if (ret != HAL_OK)
  {
    __NOP(); /* Nothing to do */
  }

  if ((rec_char == (uint8_t)'\r') || (CMD_LinkCurrentPos >= (CMD_MAX_LINE_SIZE - 1U)))
  {
    /* end of line reached: switch between reveived buffer and receiving buffer */
    CMD_link_current_rcv_line[CMD_LinkCurrentPos] = 0;
    temp = CMD_link_completed_line;
    CMD_link_completed_line = CMD_link_current_rcv_line;
    CMD_current_cmd    = CMD_link_completed_line;
    CMD_link_current_rcv_line = temp;
    CMD_LinkCurrentPos = 0;
    (void)rtosalSemaphoreRelease(CMD_rcvSemaphore);
  }
  else
  {
    /* not the end of line */
    if (rec_char == (uint8_t)'\b')
    {
      /* back space */
      if (CMD_LinkCurrentPos > 0U)
      {
        /* remove the last char received only if the receiving buffer is not empty */
        CMD_LinkCurrentPos--;
      }
    }
    else
    {
      /* normal char  */
      CMD_link_current_rcv_line[CMD_LinkCurrentPos] = rec_char;
      CMD_LinkCurrentPos++;
    }
  }
}
#endif  /* (USE_LINK_UART == 1) */

/**
  * @brief display component help
  * @param  label   component description
  * @retval -
  */
void CMD_print_help(uint8_t *label)
{
  PRINT_FORCE("***** %s help *****\r\n", label);
}


/**
  * @brief  module initialization
  * @param  -
  * @retval -
  */
void CMD_init(void)
{
#if (USE_LINK_UART == 1)
  static uint8_t   CMD_LinkCommandLine[2][CMD_MAX_LINE_SIZE];
#endif  /* (USE_LINK_UART == 1) */
  static osThreadId CMD_ThreadId;

  CMD_NbCmd           = 0U;

  CMD_CommandLine[0][0] = 0;
  CMD_CommandLine[1][0] = 0;
  CMD_current_rcv_line  = CMD_CommandLine[0];
  CMD_current_cmd       = CMD_CommandLine[1];
  CMD_completed_line    = CMD_CommandLine[1];
  CMD_CurrentPos        = 0;

#if (USE_LINK_UART == 1)
  CMD_link_current_rcv_line = CMD_LinkCommandLine[0];
  CMD_link_completed_line   = CMD_LinkCommandLine[1];
#endif  /* (USE_LINK_UART == 1) */

  CMD_Declare((uint8_t *)"help", CMD_Help, (uint8_t *)"help command");
#if (USE_LINK_UART == 1)
  CMD_Declare((uint8_t *)"uartcmd", CMD_UartCmd, "send command to link uart");
#endif  /* (USE_LINK_UART == 1) */

  CMD_LastCommandLine[0] = 0;

  CMD_rcvSemaphore = rtosalSemaphoreNew(NULL, 1);
  (void)rtosalSemaphoreAcquire(CMD_rcvSemaphore, RTOSAL_WAIT_FOREVER);

  CMD_ThreadId = rtosalThreadNew((const rtosal_char_t *)"CMD_THREAD_DEF", cmd_thread, CMD_THREAD_PRIO,
                                 USED_CMD_THREAD_STACK_SIZE, NULL);
  if (CMD_ThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_UTILITIES, 2, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1U)
    (void)stackAnalysis_addStackSizeByHandle(CMD_ThreadId, USED_CMD_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

/**
  * @brief  module start
  * @param  -
  * @retval -
  */
void CMD_start(void)
{
  HAL_StatusTypeDef ret;

  CMD_CommandLine[0][0] = 0;
  CMD_CommandLine[1][0] = 0;

#if (USE_LINK_UART == 1)
  COM_INTERFACE_UART_INIT
  while (true)
  {
    ret = HAL_UART_Receive_IT(&COM_INTERFACE_UART_HANDLE, &CMD_LinkReceivedChar, 1U);
    if (ret == HAL_OK)
    {
      break;
    }
    (void)rtosalDelay(10);
  }
#endif /* USE_LINK_UART */
  while (true)
  {
    ret = HAL_UART_Receive_IT(&TRACE_INTERFACE_UART_HANDLE, &CMD_ReceivedChar, 1U);
    if (ret == HAL_OK)
    {
      break;
    }
    (void)rtosalDelay(10);
  }
}

#endif  /* USE_CMD_CONSOLE */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
