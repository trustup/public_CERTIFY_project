/**
  ******************************************************************************
  * @file    uiclient.h
  * @author  MCD Application Team
  * @brief   Header for uiclient.c module
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
#ifndef UI_CLIENT_H
#define UI_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_UI_CLIENT == 1)

#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  Initialization
  * @note   UIClient initialization - first function called
  * @param  -
  * @retval -
  */
void uiclient_init(void);

/**
  * @brief  Start
  * @note   UIClient start - function called when platform is initialized
  * @param  -
  * @retval -
  */
void uiclient_start(void);

#endif /* USE_UI_CLIENT == 1 */

#ifdef __cplusplus
}
#endif

#endif /* UI_CLIENT_H */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

