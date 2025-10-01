/**
  ******************************************************************************
  * @file    board_leds.c
  * @author  MCD Application Team
  * @brief   Implements functions for leds actions
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
#include "board_leds.h"

#if (USE_LEDS == 1)

/* include BSP for B-L462E-CELL01 */
/* MISRAC messages linked to BSP include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l462e_cell01.h"
/*cstat +MISRAC2012-* */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Power On a Led
  * @param  bl_led - led identifier see plf_hw_config.h
  * @retval -
  */
void board_leds_on(uint8_t bl_led)
{
  if (bl_led != NO_LED)
  {
    BSP_LED_On((Led_TypeDef)bl_led);
  }
}

/**
  * @brief  Power Off a Led
  * @param  bl_led - led identifier see plf_hw_config.h
  * @retval -
  */
void board_leds_off(uint8_t bl_led)
{
  if (bl_led != NO_LED)
  {
    BSP_LED_Off((Led_TypeDef)bl_led);
  }
}

/**
  * @brief  Component initialisation
  * @param  -
  * @retval true/false - init OK/NOK
  */
bool board_leds_init(void)
{
  bool result = true;

  if (BOARD_LEDS_1 != NO_LED)
  {
    BSP_LED_Init((Led_TypeDef)BOARD_LEDS_1);
  }
  if (BOARD_LEDS_2 != NO_LED)
  {
    BSP_LED_Init((Led_TypeDef)BOARD_LEDS_2);
  }
  if (BOARD_LEDS_3 != NO_LED)
  {
    BSP_LED_Init((Led_TypeDef)BOARD_LEDS_3);
  }

  return (result);
}


/**
  * @brief  Component start
  * @param  -
  * @retval true/false - start OK/NOK
  */
bool board_leds_start(void)
{
  bool result = true;
  return (result);
}

#endif /* USE_LEDS == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
