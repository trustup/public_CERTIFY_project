/**
  ******************************************************************************
  * @file    spi_ndlc_device.h
  * @author  MCD Application Team
  * @brief
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef SPI_NDLC_DEVICE_H
#define SPI_NDLC_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_ST33 == 1)

#include "spi.h"

/* Exported constants --------------------------------------------------------*/
#ifndef NDLC_DEBUG
#define NDLC_DEBUG  1
#endif /* NDLC_DEBUG */

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef  *cs_port;
  uint16_t      *cs_pin;
  void *power;
} STSpiDevHnd_t;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* USE_ST33 == 1 */

#ifdef __cplusplus
}
#endif

#endif /* SPI_NDLC_DEVICE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

