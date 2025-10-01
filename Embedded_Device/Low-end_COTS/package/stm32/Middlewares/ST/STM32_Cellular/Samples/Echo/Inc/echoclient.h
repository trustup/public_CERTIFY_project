/**
  ******************************************************************************
  * @file    echoclient.h
  * @author  MCD Application Team
  * @brief   Header for echoclient.c module
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
#ifndef ECHO_CLIENT_H
#define ECHO_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_ECHO_CLIENT == 1)

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialization
  * @note   Echoclient initialization - first function to call
  * @param  -
  * @retval -
  */
void echoclient_init(void);

/**
  * @brief  Start
  * @note   Echoclient start - must be called after echoclient_init
  * @param  -
  * @retval -
  */
void echoclient_start(void);

#endif /* USE_ECHO_CLIENT == 1 */

#ifdef __cplusplus
}
#endif

#endif /* ECHO_CLIENT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
