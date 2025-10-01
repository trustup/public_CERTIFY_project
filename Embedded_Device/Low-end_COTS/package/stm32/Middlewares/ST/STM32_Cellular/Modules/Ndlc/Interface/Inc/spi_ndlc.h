/**
  ******************************************************************************
  * @file    spi_ndlc.h
  * @author  MCD Application Team
  * @brief   Header for spi_ndlc.c module
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
#ifndef SPI_NDLC_H
#define SPI_NDLC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if ((USE_ST33 == 1) && (NDLC_INTERFACE == NDLC_SPI_INTERFACE))

#include <stdint.h>
#include <stdbool.h>

#include "ndlc_config.h"

/* Exported constants --------------------------------------------------------*/
#define SPI_NDLC_OK                      0
#define SPI_NDLC_COMMUNICATION_ERROR    -1
#define SPI_NDLC_COMMUNICATION_TIMEOUT  -2
#define SPI_NDLC_INIT_ERROR             -3

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* PIL: Temporary patch- Normal correction : use rsp buffer provided as parameter by Applications */
extern uint8_t ST33Data[260];

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
bool spi_ndlc_init(void *hspi, ndlc_device_t *p_handler);

bool spi_ndlc_deinit(ndlc_device_t *p_handler);

bool spi_ndlc_power(ndlc_device_t *p_handler, bool state_on);

int32_t spi_ndlc_atr(ndlc_device_t *p_handler);

int32_t spi_ndlc_transceive_apdu(ndlc_device_t *p_handler, uint8_t *buffer_tx, uint16_t buffer_tx_len,
                                 uint8_t *buffer_rx);

int32_t spi_ndlc_abort_apdu(ndlc_device_t *p_handler);

int32_t spi_ndlc_send_receive_phy(void *p_handler, uint16_t length, uint8_t *tx_data, uint8_t *rx_data);

#endif /* #if (USE_ST33 == 1) && (NDLC_INTERFACE == NDLC_SPI_INTERFACE) */

#ifdef __cplusplus
}
#endif

#endif /* SPI_NDLC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

