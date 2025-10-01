/**
  ******************************************************************************
  * @file    comclient.h
  * @author  MCD Application Team
  * @brief   Header for comclient.c module
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
#ifndef COM_CLIENT_H
#define COM_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_COM_CLIENT == 1)

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */


#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_COMCLIENT, DBL_LVL_P0, "" format, ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_COMCLIENT, DBL_LVL_P0, "ComClt: " format, ## args)
#define PRINT_DBG(format, args...) \
  TRACE_PRINT(DBG_CHAN_COMCLIENT, DBL_LVL_P1, "ComClt: " format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#define PRINT_INFO(format, args...)  (void)printf("CustClt: " format, ## args);
#define PRINT_DBG(format, args...)   (void)printf("CustClt: " format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */
#else /* USE_TRACE_CUSTOM_CLIENT == 0 */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_CUSTOMCLIENT, DBL_LVL_P0, "" format, ## args)
#else
#include <stdio.h>
#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#endif  /* (USE_PRINTF == 0U) */
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */


/**
  * @brief  Initialization
  * @note   COMClient initialization - first function to call
  * @param  -
  * @retval -
  */
void comclient_init(void);

/**
  * @brief  Start
  * @note   COMClient start - must be called after comclient_init
  * @param  -
  * @retval -
  */
void comclient_start(void);

#endif /* USE_COM_CLIENT == 1 */

#ifdef __cplusplus
}
#endif

#endif /* COM_CLIENT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
