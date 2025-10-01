/**
  ******************************************************************************
  * @file    board_leds.h
  * @author  MCD Application Team
  * @brief   Header for board_leds.c module
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
#ifndef BOARD_LEDS_H
#define BOARD_LEDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_LEDS == 1)

#include <stdbool.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Power On a Led
  * @param  bl_led - led identifier see plf_hw_config.h
  * @retval -
  */
void board_leds_on(uint8_t bl_led);

/**
  * @brief  Power Off a Led
  * @param  bl_led - led identifier see plf_hw_config.h
  * @retval -
  */
void board_leds_off(uint8_t bl_led);

/**
  * @brief  Component initialisation
  * @param  -
  * @retval true/false - init OK/NOK
  */
bool board_leds_init(void);

/**
  * @brief  Component start
  * @param  -
  * @retval true/false start OK/NOK
  */
bool board_leds_start(void);

#endif /* USE_LEDS == 1 */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_LEDS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
