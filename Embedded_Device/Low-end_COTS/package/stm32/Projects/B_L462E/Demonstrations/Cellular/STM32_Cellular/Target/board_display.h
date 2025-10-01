/**
  ******************************************************************************
  * @file    board_display.h
  * @author  MCD Application Team
  * @brief   Header for board_display.c module
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOARD_DISPLAY_H
#define BOARD_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_DISPLAY == 1)

#include <stdbool.h>
#include <stdint.h>

/* include BSP for B-L462E-CELL01 */
/* MISRAC messages linked to BSP include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l462e_cell01.h"
#include "stm32l462e_cell01_lcd.h"
/*cstat +MISRAC2012-* */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialize the Display
  * @param  -
  * @retval bool - true/false Display init OK/NOK
  */
bool board_display_Init(void);

/**
  * @brief  De-Initialize the Display
  * @param  -
  * @retval bool - true/false Display de-init OK/NOK
  */
bool board_display_DeInit(void);

/**
  * @brief  Clear the Display
  * @param  color - color to clear the buffer
  * @retval -
  */
void board_display_Clear(uint16_t color);

/**
  * @brief  Draw a frame buffer on the Display
  * @param  pFrameBuffer - pointer on frame buffer to draw
  * @retval -
  */
void board_display_DrawRawFrameBuffer(uint8_t *pFrameBuffer);
void board_display_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata);

uint32_t board_display_CharacterNbPerLine(void);
uint32_t board_display_GetFontHeight(void);

/**
  * @brief  Displays characters on the LCD.
  * @param  Xpos X position (in pixel)
  * @param  Ypos Y position (in pixel)
  * @param  pText Pointer to string to display on LCD
  * @param  Mode Display mode
  *          This parameter can be one of the following values:
  *            @arg  CENTER_MODE
  *            @arg  RIGHT_MODE
  *            @arg  LEFT_MODE
  * @retval None
  */
void board_display_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *pText, Line_ModeTypdef Mode);

/**
  * @brief  Set font to 0(default), 8, 12, 16, 20, 24
  * @retval None
  */
void board_display_SetFont(uint8_t size);
/**
  * @brief  Decrease font
  * @retval None
  */
void board_display_DecreaseFont(void);

/**
  * @brief  Refresh the Display
  * @param  -
  * @retval -
  */
void board_display_Refresh(void);

#endif /* USE_DISPLAY == 1 */

#ifdef __cplusplus
}
#endif

#endif /* USE_DISPLAY */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
