/**
  ******************************************************************************
  * @file    ndlc_commands.h
  * @author  MCD Application Team
  * @brief   Header for ndlc_commands.c module
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
#ifndef NDLC_COMMANDS_H
#define NDLC_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ndlc_config.h"

#include <stdint.h>

#if (USE_ST33 == 1)

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

void NDLC_Initialization(ndlc_device_t *p_dev);
void NDLC_DeInitialization(ndlc_device_t *p_dev);

/**
  * @brief  Abort Command
  * @param  dev - pointer of the device
  * @param  outdata - pointer of the response
  * @retval int32_t - length of the response or error value
  */
int32_t NDLC_abort(ndlc_device_t *p_dev, uint8_t *outdata);

/**
  * @brief  APDU Command
  * @param  dev  - pointer of the device
  * @param  apdu - pointer of the apdu
  * @param  apdu_len - apdu length
  * @param  outdata  - pointer of the response
  * @retval int32_t  - length of the response or error value
  */
int32_t NDLC_apdu(ndlc_device_t *p_dev, uint8_t *apdu, uint16_t apdu_len, uint8_t *outdata);

#endif /* USE_ST33 == 1 */


#ifdef __cplusplus
}
#endif

#endif /* NDLC_COMMANDS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
