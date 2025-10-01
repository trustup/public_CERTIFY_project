/**
  ******************************************************************************
  * @file    ndlc_commands.c
  * @author  MCD Application Team
  * @brief   NDLC commands module
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
#include "ndlc_commands.h"

#if (USE_ST33 == 1)
#include <string.h>
#include "ndlc.h"

/* Private defines -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

void NDLC_Initialization(ndlc_device_t *p_dev)
{
  p_dev->status = NDLC_IDLE;
}

void NDLC_DeInitialization(ndlc_device_t *p_dev)
{
  p_dev->status = NDLC_IDLE;
}

/**
  * @brief  Abort Command
  * @param  p_dev - pointer of the device
  * @param  outdata - pointer of the response
  * @retval int32_t - length of the response or error value
  */
int32_t NDLC_abort(ndlc_device_t *p_dev, uint8_t *outdata)
{
  uint8_t cmd[4] = { 0x80, 0x02, 0xE0, 0x51 };
  int32_t result = -1;

  p_dev->status = NDLC_IDLE;

  if (ndlc_send_receive(p_dev, 4, cmd, p_dev->data)
      == 0)
  {
    /* Check that data doesn't exceed buffer */
    (void)memcpy(outdata, &p_dev->data[2], p_dev->dataLen);
    result = 0;
  }

  return (result);
}

/**
  * @brief  APDU Command
  * @param  p_dev  - pointer of the device
  * @param  apdu - pointer of the apdu
  * @param  apdu_len - apdu length
  * @param  outdata  - pointer of the response
  * @retval int32_t  - length of the response or error value
  */
int32_t NDLC_apdu(ndlc_device_t *p_dev, uint8_t *apdu, uint16_t apdu_len, uint8_t *outdata)
{
  uint16_t i;
  uint8_t cmd[4U + 256U];
  int32_t result;

  result = -1;
  (void)memset(cmd,0,4U + 256U);
  cmd[0] = 0x80;
  /* length calculation */
  cmd[1] = 2U + (uint8_t)apdu_len;
  cmd[2] = 0xE0;
  cmd[3] = 0x50;

  for (i = 0U; i < apdu_len; i++)
  {
    cmd[4U + i] = apdu[i];
  }
  p_dev->status = NDLC_IDLE;

  if (ndlc_send_receive(p_dev, (4U + apdu_len), cmd, p_dev->data)
      == 0)
  {
    (void)memcpy(outdata, &p_dev->data[2], p_dev->dataLen);
    result = (int32_t)p_dev->dataLen;
  }

  return (result);
}


#endif /* USE_ST33 == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

