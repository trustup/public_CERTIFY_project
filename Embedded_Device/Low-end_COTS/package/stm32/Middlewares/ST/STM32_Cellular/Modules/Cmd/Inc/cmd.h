/**
  ******************************************************************************
  * @file    cmd.h
  * @author  MCD Application Team
  * @brief   Header for cmd.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CMD_H
#define CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include <stdint.h>

typedef enum
{
  CMD_OK = 0U,
  CMD_SYNTAX_ERROR,
  CMD_PROCESS_ERROR
} cmd_status_t;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef cmd_status_t (*CMD_HandlerCmd)(uint8_t *);

/* Exported functions ------------------------------------------------------- */
void CMD_Declare(uint8_t *cmd_name_p, CMD_HandlerCmd cmd_handler, uint8_t *cmd_label_p);
void CMD_init(void);
void CMD_start(void);
void CMD_RxCpltCallback(UART_HandleTypeDef *uart_handle_p);
void CMD_SetLine(uint8_t *command_line);

#if (USE_LINK_UART == 1)
void CMD_RxLinkCpltCallback(UART_HandleTypeDef *uart_handle_p);
#endif /* USE_LINK_UART == 1 */
void CMD_print_help(uint8_t *label);
uint32_t CMD_GetValue(uint8_t *string_p, uint32_t *value_p);

#ifdef __cplusplus
}
#endif

#endif /* CMD_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
