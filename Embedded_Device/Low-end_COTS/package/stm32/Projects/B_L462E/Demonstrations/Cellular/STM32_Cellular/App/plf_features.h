/**
  ******************************************************************************
  * @file    plf_features.h
  * @author  MCD Application Team
  * @brief   Includes feature list to include in firmware
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_FEATURES_H
#define PLF_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#if (USE_CUSTOM_CONFIG == 1)
#include "plf_custom_config.h"
#endif /* USE_CUSTOM_CONFIG == 1 */

/* Exported constants --------------------------------------------------------*/

/* ================================================= */
/*          USER MODE                                */
/* ================================================= */

/* ===================================== */
/* BEGIN - Cellular data mode            */
/* ===================================== */

/* Possible values for USE_SOCKETS_TYPE */
#define USE_SOCKETS_LWIP   (0)  /* define value affected to LwIP sockets type  */
#define USE_SOCKETS_MODEM  (1)  /* define value affected to Modem sockets type */

/* Sockets location */
#if !defined USE_SOCKETS_TYPE
#define USE_SOCKETS_TYPE   (USE_SOCKETS_MODEM)
#endif /* !defined USE_SOCKETS_TYPE */

/* ===================================== */
/* END - Cellular data mode              */
/* ===================================== */

/* ===================================== */
/* BEGIN - Applications to include       */
/* ===================================== */
#if !defined USE_ECHO_CLIENT
#define USE_ECHO_CLIENT    (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_ECHO_CLIENT */

#if !defined USE_HTTP_CLIENT
#define USE_HTTP_CLIENT    (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_HTTP_CLIENT */

#if !defined USE_PING_CLIENT
#define USE_PING_CLIENT    (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_PING_CLIENT */

#if !defined USE_COM_CLIENT
#define USE_COM_CLIENT     (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_COM_CLIENT */

#if !defined USE_MQTT_CLIENT
#define USE_MQTT_CLIENT    (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_MQTT_CLIENT */

#if !defined USE_UI_CLIENT
#define USE_UI_CLIENT      (1) /* 0: not activated, 1: activated */
#endif /* !defined USE_UI_CLIENT */

/* MEMS Setup BEGIN */
/* USE_DC_MEMS enables MEMS management */
#if !defined USE_DC_MEMS
#define USE_DC_MEMS        (1) /* 0: not activated, 1: activated */
#endif /* !defined USE_DC_MEMS */

/* USE_SIMU_MEMS enables MEMS simulation management */
#if !defined USE_SIMU_MEMS
#define USE_SIMU_MEMS      (1) /* 0: not activated, 1: activated */
#endif  /* !defined USE_SIMU_MEMS */
/* MEMS Setup END */

/* if USE_DC_MEMS and USE_SIMU_MEMS are both defined, the behaviour of availability of MEMS board:
 if  MEMS board is connected, true values are returned
 if  MEMS board is not connected, simulated values are returned
 Note: USE_DC_MEMS and USE_SIMU_MEMS are independent
*/

/* use generic datacache entries */
#if !defined USE_DC_GENERIC
#define USE_DC_GENERIC     (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_DC_GENERIC */


/* ===================================== */
/* END   - Applications to include       */
/* ===================================== */

/* ======================================= */
/* BEGIN -  Miscellaneous functionalities  */
/* ======================================= */

/* To activate Network Library */
#if !defined USE_NETWORK_LIBRARY
#if (USE_MQTT_CLIENT == 1)
#define USE_NETWORK_LIBRARY        (1) /* MqttClient use Network Library 1: activated */
#else  /* USE_MQTT_CLIENT == 0 */
#define USE_NETWORK_LIBRARY        (0) /* 0: not activated, 1: activated */
#endif /* USE_MQTT_CLIENT == 1 */
#endif /* !defined USE_NETWORK_LIBRARY */

/* If included then MbedTls Library is integrated */
/* USE_MBEDTLS must be included if USE_MQTT_CLIENT is activated */
#if !defined USE_MBEDTLS
#if (USE_MQTT_CLIENT == 1)
#define USE_MBEDTLS                (1) /* MqttClient use MbedTls Library 1: activated */
#else  /* USE_MQTT_CLIENT == 0 */
#define USE_MBEDTLS                (0) /* 0: not activated, 1: activated */
#endif /* USE_MQTT_CLIENT == 1 */
#endif /* !defined USE_MBEDTLS */

/* If included then ComPing library is integrated */
/* USE_COM_PING must be included if USE_NETWORK_LIBRARY or USE_PING_CLIENT are included */
#if !defined USE_COM_PING
#if ((USE_NETWORK_LIBRARY == 1) || (USE_PING_CLIENT == 1))
#define USE_COM_PING        (1) /* NetworkLibrary and PingClient use ComPing library 1: included */
#else  /* (USE_NETWORK_LIBRARY == 0) && (USE_PING_CLIENT == 0) */
#define USE_COM_PING        (0) /* 0: not included, 1: included */
#endif /* (USE_NETWORK_LIBRARY == 1) || (USE_PING_CLIENT == 1) */
#endif /* !defined USE_COM_PING */

/* If included then ComIcc library is integrated */
#if !defined USE_COM_ICC
#define USE_COM_ICC         (1)  /* 0: not included, 1: included */
#endif /* !defined USE_COM_ICC */

/* To include RTC service */
#if !defined USE_RTC
#define USE_RTC        (1) /* 0: not activated, 1: activated */
#endif /* !defined USE_RTC */

/* To configure some parameters of the software */
#if !defined USE_CMD_CONSOLE
#define USE_CMD_CONSOLE            (1) /* 0: not activated, 1: activated */
#endif /* !defined USE_CMD_CONSOLE */

#if !defined USE_DEFAULT_SETUP
#define USE_DEFAULT_SETUP          (0) /* 0: Use setup menu,
                                          1: Use default parameters, no setup menu */
#endif /* !defined USE_DEFAULT_SETUP */

/* Begin Stack analysis tools configuration */
#if !defined USE_STACK_ANALYSIS
#define USE_STACK_ANALYSIS         (0) /* 0: Stack analysis is not embedded
                                          1: Stack analysis is available */
#endif /* !defined USE_STACK_ANALYSIS */

#if (USE_STACK_ANALYSIS == 1)
#if !defined STACK_ANALYSIS_TIMER
/* Value of the timer to trace automatically the thread stack value
   To do this, stack analysis will create a thread
   unit is ms
   default value 0 : timer = 0U means feature not activated
   usage example :
   for long duration test, activate the timer to detect thread stack overflow.
   value example :
   1min:60000 - 5min: 300000 - 1h: 3600000 ...
*/
#define STACK_ANALYSIS_TIMER       (0U) /* default configuration: no thread stack display every x ms */
#endif /* !defined STACK_ANALYSIS_TIMER */
#endif /* USE_STACK_ANALYSIS == 1 */
/* End Stack analysis tools configuration */

#if !defined USE_BUTTONS
#define USE_BUTTONS               (1)  /* 0: not activated, 1: activated */
#endif /* !defined USE_BUTTONS */

#if !defined USE_LEDS
#define USE_LEDS                  (1)  /* 0: not activated, 1: activated */
#endif /* !defined USE_LEDS */

/* use UART Communication between two boards */
#if !defined USE_LINK_UART
#define USE_LINK_UART              (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_LINK_UART */

/* ======================================= */
/* END   -  Miscellaneous functionalities  */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_FEATURES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
