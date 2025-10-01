/**
  ******************************************************************************
  * @file    plf_custom_config.h
  * @author  MCD Application Team
  * @brief   Custom features configuration
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
#ifndef PLF_CUSTOM_CONFIG_H
#define PLF_CUSTOM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* ================================================= */
/*          USER MODE                                */
/* ================================================= */

#if (USE_CUSTOM_CONFIG == 1)

/* Overwrites X-Cube-Cellular features  */
/* Below an example to have a minimal configuration */

/* ===================================== */
/* BEGIN - Applications to include       */
/* ===================================== */
#define USE_COM_CLIENT       (0) /* 0: not activated, 1: activated */
#define USE_ECHO_CLIENT      (0)  /* 0: not activated, 1: activated */
#define USE_HTTP_CLIENT      (0)  /* 0: not activated, 1: activated */
#define USE_PING_CLIENT      (0)  /* 0: not activated, 1: activated */
#define USE_MQTT_CLIENT      (0)  /* 0: not activated, 1: activated */
#define USE_UI_CLIENT        (0)  /* 0: not activated, 1: activated */

#define USE_DC_MEMS          (1)  /* 0: not activated, 1: activated */
#define USE_SIMU_MEMS        (0)  /* 0: not activated, 1: activated */
#define USE_DC_GENERIC       (0)  /* 0: not activated, 1: activated */
/* ===================================== */
/* END   - Applications to include       */
/* ===================================== */

/* ======================================= */
/* BEGIN -  Miscellaneous functionalities  */
/* ======================================= */

/* If included then com_ping interfaces are defined in com module */
/* Note: USE_COM_PING must be included when USE_PING_CLIENT is activated */
#define USE_COM_PING               (0)  /* 0: not included, 1: included */

/* If included then com_sim interfaces are defined in com module */
#define USE_COM_ICC                (1)  /* 0: not included, 1: included */

/* If COM_SOCKETS_STATISTIC is activated then sockets statistic displayed
   on command request and/or every COM_SOCKETS_STATISTIC_PERIOD minutes */
#define COM_SOCKETS_STATISTIC      (0U) /* 0: not activated, 1: activated */

/* To interact with Cellular by connecting a Terminal and send commands */
#define USE_CMD_CONSOLE            (1) /* 0: not activated, 1: activated */

/* To include RTC service */
#define USE_RTC                    (1) /* 0: not activated, 1: activated */

/* To setup Cellular configuration through a menu displayed in a Terminal */
#define USE_DEFAULT_SETUP          (1) /* 0: Use setup menu,
                                          1: Use default parameters, no setup menu */

/* Begin Stack analysis tools configuration */
#define USE_STACK_ANALYSIS         (0) /* 0: Stack analysis is not embedded
                                          1: Stack analysis is available */
/* End Stack analysis tools configuration */


/* use buttons */
#define USE_BUTTONS                (0) /* 0: not activated, 1: activated */

/* use display */
#define USE_DISPLAY                (0)  /* 0: not activated, 1: activated */

/* use leds */
#define USE_LEDS                   (1)  /* 0: not activated, 1: activated */

/* use UART Communication between two boards */
#define USE_LINK_UART              (0) /* 0: not activated, 1: activated */

/* ======================================= */
/* END   -  Miscellaneous functionalities  */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#define SW_DEBUG_VERSION           (1U)  /* 0 for SW release version (no traces),
                                            1 for SW debug version */

/* ======================================= */
/* BEGIN -  Custom Client specific defines */
/* ======================================= */

/* To use a custom client application, set next define to 1 */
#define USE_CUSTOM_CLIENT                    (1)  /* 0: not activated, 1: activated */

#if (USE_CUSTOM_CLIENT == 1)

/* CUSTOMCLIENT_THREAD_STACK_SIZE: define the STACK size needed for CUSTOM_CLIENT main thread */
#define CUSTOMCLIENT_THREAD_STACK_SIZE        (8192)
#define CUSTOMCLIENT_THREAD_PRIO              osPriorityNormal
#define CUSTOMCLIENT_THREAD                   1 /* Number of threads created by CUSTOM_CLIENT */

/* What to do if more than 1 thread is created by CUSTOMCLIENT ?
Then for each XXX thread :
1) Add a #define CUSTOMCLIENT_XXX_THREAD_STACK_SIZE (YYYU)
2) Add next code after creation of CUSTOMCLIENT_XXX thread:
#if (USE_STACK_ANALYSIS == 1)
  (void)stackAnalysis_addStackSizeByHandle(CustomClient_xxx_TaskHandle,
                                           CUSTOMCLIENT_XXX_THREAD_STACK_SIZE);
#endif
(see: void custom_client_start(void))
Note: doing this stack analysis will be able to monitor all xxx threads
3) Add a #define APPLICATION_HEAP_SIZE: SUM of all CUSTOMCLIENT_XXX_THREAD_STACK_SIZE
(in this sum do not include CUSTOMCLIENT_THREAD_STACK_SIZE)
#define APPLICATION_HEAP_SIZE       SUM of all (CUSTOMCLIENT_XXX_THREAD_STACK_SIZE)
Note: doing this the global heap allocated in FreeRTOS will be fine
4) Change #define CUSTOMCLIENT_THREAD value: SUM of threads created by CUSTOMCLIENT (including CUSTOMCLIENT thread)
*/

/* To use a terminal to interact with Custom client through CMD module, set next define to 1 */
#define CUSTOM_CLIENT_CMD                    (1U)  /* 0U: No usage of command module
                                                      1U: One command registration */

/* Active or not the debug trace in Custom Client */
#if (SW_DEBUG_VERSION == 1U)
#define USE_TRACE_CUSTOM_CLIENT              (1)  /* 1: Trace in Custom Client activated */
#else
#define USE_TRACE_CUSTOM_CLIENT              (0)  /* 0: No trace in Custom Client */
#endif /* SW_DEBUG_VERSION == 1U */

#endif /* USE_CUSTOM_CLIENT == 1 */

/* ======================================= */
/* END   -  Custom Client specific defines */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* USE_CUSTOM_CONFIG == 1 */

#ifdef __cplusplus
}
#endif

#endif /* PLF_CUSTOM_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
