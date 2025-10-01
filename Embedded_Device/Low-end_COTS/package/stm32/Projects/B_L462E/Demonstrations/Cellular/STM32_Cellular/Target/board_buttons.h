/**
  ******************************************************************************
  * @file    board_buttons.h
  * @author  MCD Application Team
  * @brief   Header for board_buttons.c module
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
#ifndef BOARD_BUTTONS_H
#define BOARD_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_BUTTONS == 1)

#include <stdbool.h>
#include <stdint.h>

#include "dc_common.h"

/* Exported types ------------------------------------------------------------*/
typedef uint8_t board_buttons_type_t;
#define   BOARD_BUTTONS_USER_TYPE  (board_buttons_type_t)0
#define   BOARD_BUTTONS_UP_TYPE    (board_buttons_type_t)1
#define   BOARD_BUTTONS_DOWN_TYPE  (board_buttons_type_t)2
#define   BOARD_BUTTONS_LEFT_TYPE  (board_buttons_type_t)3
#define   BOARD_BUTTONS_RIGHT_TYPE (board_buttons_type_t)4
#define   BOARD_BUTTONS_SEL_TYPE   (board_buttons_type_t)5
#define   BOARD_BUTTONS_MAX_TYPE   (board_buttons_type_t)6

/* Data Cache structure for button entries */
typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state; /* - DC_SERVICE_UNAVAIL: service not initialized,
                                    *                       the field values of structure are not significant.
                                    * - DC_SERVICE_ON: button is initialized,
                                    *                  all the field values of structure are significant.
                                    * - Other state values not used
                                    */
  bool press_type;    /* false: short press, true: long press  */
} dc_board_buttons_t; /* board button datacache structure      */

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Defined DC_BOARD_BUTTONS_xxx whatever the Hardware to not have issue at compilation in Samples */
/** @brief Control Button USER Data Cache entries       */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_USER;

/** @brief Control Button UP Data Cache entries         */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_UP;

/** @brief Control Button DOWN Data Cache entries       */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_DOWN;

/** @brief Control Button LEFT Data Cache entries       */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_LEFT;

/** @brief Control Button RIGHT Data Cache entries      */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_RIGHT;
/** @brief Control Button SELECTION Data Cache entries  */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_SEL;

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void board_buttons_pressed(uint8_t buttons);

void board_buttons_released(uint8_t buttons);

/**
  * @brief  Component initialisation
  * @param  -
  * @retval true/false - init OK/NOK
  */
bool board_buttons_init(void);

/**
  * @brief  Component start
  * @param  -
  * @retval true/false start OK/NOK
  */
bool board_buttons_start(void);

#ifdef __cplusplus
}
#endif

#endif /* USE_BUTTONS == 1 */

#endif /* BOARD_BUTTONS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
