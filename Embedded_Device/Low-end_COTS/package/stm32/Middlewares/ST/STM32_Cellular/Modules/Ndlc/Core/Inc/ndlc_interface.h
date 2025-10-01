/**
  ******************************************************************************
  * @file    ndlc_interface.h
  * @author  MCD Application Team
  * @brief   Ndlc interfcae definition
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
#ifndef NDLC_INTERFACE_H
#define NDLC_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ndlc_config.h"

#if (USE_ST33 == 1)
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/* bool ndlc_init(void *p_handler_predefined, ndlc_device_t *p_handler); */

/* bool ndlc_deinit(ndlc_device_t *p_handler); */

/* bool ndlc_power(ndlc_device_t *p_handler, bool state_on); */

/* int32_t ndlc_atr(ndlc_device_t *p_handler); */

/* int32_t ndlc_apdu_snd_rcv(ndlc_device_t *p_handler, uint8_t *p_buf_tx, uint16_t buffer_tx_len, uint8_t *p_buf_rx); */

#endif /* USE_ST33 == 1 */

#ifdef __cplusplus
}
#endif

#endif /* NDLC_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
