/**
  ******************************************************************************
  * @file    board_display.c
  * @author  MCD Application Team
  * @brief   Implements functions for display actions
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
/* Includes ------------------------------------------------------------------*/
#include "board_display.h"

#if (USE_DISPLAY == 1)

#include "sys_spi.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialize the Display
  * @param  -
  * @retval bool - true/false Display init OK/NOK
  */
bool board_display_Init(void)
{
  bool result;

  result = sys_spi_init();

  if (result == true)
  {
    /* SPI must be Powered On and configure for the display */
    sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

    /* Before to use Display SPI must be powered on */
    if (sys_spi_power_on() == true)
    {
      if (BSP_LCD_Init() == (uint8_t)LCD_OK)
      {
        result = true;
      }
    }

    /* SPI resource release */
    sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);
  }

  return (result);
}

/**
  * @brief  De-Initialize the Display
  * @param  -
  * @retval bool - true/false Display de-init OK/NOK
  */
bool board_display_DeInit(void)
{
  bool result = false;

  /* SPI must be Powered On and configure for the display */
  sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

  if (BSP_LCD_DeInit() == (uint8_t)LCD_OK)
  {
    result = true;
  }

  /* SPI resource release */
  sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);

  return (result);
}


/**
  * @brief  Draw a frame buffer on the Display
  * @param  p_frame_buffer - pointer on frame buffer to draw
  * @retval -
  */
void board_display_DrawRawFrameBuffer(uint8_t *pFrameBuffer)
{
  /* SPI must be Powered On and configure for the display */
  sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

  BSP_LCD_DrawRawFrameBuffer(pFrameBuffer);

  /* SPI resource release */
  sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);
}

void board_display_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata)
{
  UNUSED(Xpos);
  UNUSED(Ypos);
  UNUSED(Xsize);
  UNUSED(Ysize);

  /* SPI must be Powered On and configure for the display */
  sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

  BSP_LCD_DrawRawFrameBuffer(pdata);

  /* SPI resource release */
  sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);
}

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
  * @retval -
  */
void board_display_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *pText, Line_ModeTypdef Mode)
{
  /* SPI must be Powered On and configure for the display */
  sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

  BSP_LCD_DisplayStringAt(Xpos, Ypos, pText, Mode);

  /* SPI resource release */
  sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);
}

/**
  * @brief  Set font to 0(default), 8, 12, 16, 20, 24
  * @retval None
  */
void board_display_SetFont(uint8_t size)
{
  switch (size)
  {
    case 0U :
      BSP_LCD_SetFont(&Font12);
      break;
    case 8U :
      BSP_LCD_SetFont(&Font8);
      break;
    case 12U :
      BSP_LCD_SetFont(&Font12);
      break;
    case 16U :
      BSP_LCD_SetFont(&Font16);
      break;
    case 20U :
      BSP_LCD_SetFont(&Font20);
      break;
    case 24U :
      BSP_LCD_SetFont(&Font24);
      break;
    default :
      break;
  }
}

void board_display_DecreaseFont(void)
{
  sFONT *pFont = BSP_LCD_GetFont();

  if (pFont != NULL)
  {
    if (pFont == &Font12)
    {
      BSP_LCD_SetFont(&Font8);
    }
    else if (pFont == &Font16)
    {
      BSP_LCD_SetFont(&Font12);
    }
    else if (pFont == &Font20)
    {
      BSP_LCD_SetFont(&Font16);
    }
    else if (pFont == &Font24)
    {
      BSP_LCD_SetFont(&Font20);
    }
    else
    {
      __NOP();
    }
  }
}

uint32_t board_display_CharacterNbPerLine(void)
{
  sFONT *pFont;
  uint32_t result = 0U;

  pFont = BSP_LCD_GetFont();

  if (pFont != NULL)
  {
    /* Characters number per line */
    result = (BSP_LCD_GetXSize() / pFont->Width);
  }

  return (result);
}

uint32_t board_display_GetFontHeight(void)
{
  sFONT *pFont;
  uint32_t result = 0U;

  pFont = BSP_LCD_GetFont();

  if (pFont != NULL)
  {
    /* Characters number per line */
    result = pFont->Height;
  }

  return (result);
}

/**
  * @brief  Clear the Display
  * @param  color - color to clear the buffer
  * @retval -
  */
void board_display_Clear(uint16_t color)
{
  /* SPI must be Powered On and configure for the display */
  sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

  BSP_LCD_Clear(color);

  /* SPI resource release */
  sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);
}

/**
  * @brief  Refresh the Display
  * @param  -
  * @retval -
  */
void board_display_Refresh(void)
{
  /* SPI must be Powered On and configure for the display */
  sys_spi_acquire(SYS_SPI_DISPLAY_CONFIGURATION);

  BSP_LCD_Refresh();

  /* SPI resource release */
  sys_spi_release(SYS_SPI_DISPLAY_CONFIGURATION);
}

#endif /* USE_DISPLAY == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
