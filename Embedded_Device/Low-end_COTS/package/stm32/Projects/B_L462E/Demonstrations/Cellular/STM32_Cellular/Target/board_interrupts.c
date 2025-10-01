/**
  ******************************************************************************
  * @file    board_interrupts.c
  * @author  MCD Application Team
  * @brief   Implements HAL weak functions for Interrupts
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

#include "ipc_uart.h"
#include "at_modem_api.h"
#if (USE_BUTTONS == 1)
#include "board_buttons.h"
#endif  /* USE_BUTTONS == 1 */
#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

/* NOTE : this code is designed for FreeRTOS */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == MODEM_RING_PIN)
  {
    GPIO_PinState gstate = HAL_GPIO_ReadPin(MODEM_RING_GPIO_PORT, MODEM_RING_PIN);
    atcc_hw_event(DEVTYPE_MODEM_CELLULAR, HWEVT_MODEM_RING, gstate);
  }
#if (USE_BUTTONS == 1)
  else if (GPIO_Pin == BUTTON_USER_PIN)
  {
    if (BSP_PB_GetState(BUTTON_USER) == (uint32_t)GPIO_PIN_RESET)
    {
      board_buttons_pressed((uint8_t)BUTTON_USER);
    }
    else
    {
      board_buttons_released((uint8_t)BUTTON_USER);
    }
  }
#endif /* USE_BUTTONS == 1 */
  else
  {
    /* Nothing to do */
    __NOP();
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == MODEM_UART_INSTANCE)
  {
    IPC_UART_RxCpltCallback(huart);
  }
#if (USE_CMD_CONSOLE == 1)
  else if (huart->Instance == TRACE_INTERFACE_INSTANCE)
  {
    CMD_RxCpltCallback(huart);
  }
#if (USE_LINK_UART == 1)
  else if (huart->Instance == COM_INTERFACE_INSTANCE)
  {
    CMD_RxLinkCpltCallback(huart);
  }
#endif  /* USE_LINK_UART */
#endif  /* USE_CMD_CONSOLE */
  else
  {
    /* Nothing to do */
    __NOP();
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == MODEM_UART_INSTANCE)
  {
    IPC_UART_TxCpltCallback(huart);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == MODEM_UART_INSTANCE)
  {
    IPC_UART_ErrorCallback(huart);
  }
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
