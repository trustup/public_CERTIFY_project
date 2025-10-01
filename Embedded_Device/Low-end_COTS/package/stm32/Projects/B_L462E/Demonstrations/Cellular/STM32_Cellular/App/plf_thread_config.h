/**
  ******************************************************************************
  * @file    plf_thread_config.h
  * @author  MCD Application Team
  * @brief   This file contains thread configuration
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
#ifndef PLF_THREAD_CONFIG_H
#define PLF_THREAD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "plf_features.h"

/* Exported constants --------------------------------------------------------*/

/* ========================*/
/* BEGIN - Stack Priority  */
/* ========================*/
#define TCPIP_THREAD_PRIO                  osPriorityBelowNormal
#define PPPOSIF_CLIENT_THREAD_PRIO         osPriorityHigh
#define DC_MEMS_THREAD_PRIO                osPriorityNormal
#define ATCORE_THREAD_STACK_PRIO           osPriorityNormal
#define CELLULAR_SERVICE_THREAD_PRIO       osPriorityNormal
#define CTRL_THREAD_PRIO                   osPriorityAboveNormal
#define BOARD_BUTTONS_THREAD_PRIO          osPriorityNormal
#define ECHOCLIENT_THREAD_PRIO             osPriorityNormal
#define HTTPCLIENT_THREAD_PRIO             osPriorityNormal
#define PINGCLIENT_THREAD_PRIO             osPriorityNormal
#define COMCLIENT_THREAD_PRIO              osPriorityNormal
#define UICLIENT_THREAD_PRIO               osPriorityNormal
#define CMD_THREAD_PRIO                    osPriorityBelowNormal
#define MQTTCLIENT_THREAD_PRIO             osPriorityNormal
#if (USE_NETWORK_LIBRARY == 1)
#define NET_CELLULAR_THREAD_PRIO           osPriorityAboveNormal
#endif /* (USE_NETWORK_LIBRARY == 1) */
#if ((USE_STACK_ANALYSIS == 1) && (STACK_ANALYSIS_TIMER != 0U))
#define STACK_ANALYSIS_THREAD_PRIO         osPriorityNormal
#endif /* (USE_STACK_ANALYSIS == 1) && (STACK_ANALYSIS_TIMER != 0U) */

/* ========================*/
/* END - Stack Priority    */
/* ========================*/

/* ========================*/
/* BEGIN - Stack Size      */
/* ========================*/
#define TCPIP_THREAD_STACK_SIZE             (512U)
#if (USE_NETWORK_LIBRARY == 1)
#define DEFAULT_THREAD_STACK_SIZE           (1024U)
#else   /* USE_NETWORK_LIBRARY == 1 */
#define DEFAULT_THREAD_STACK_SIZE           (384U)
#endif  /* USE_NETWORK_LIBRARY == 0 */
#define FREERTOS_TIMER_THREAD_STACK_SIZE    (256U)
#define FREERTOS_IDLE_THREAD_STACK_SIZE     (128U)

#define ATCORE_THREAD_STACK_SIZE            (384U)
#define CELLULAR_SERVICE_THREAD_STACK_SIZE  (512U)

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#define PPPOSIF_CLIENT_THREAD_STACK_SIZE    (640U)
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#if (USE_BUTTONS == 1)
#define BOARD_BUTTONS_THREAD_STACK_SIZE     (256U)
#endif /* USE_BUTTONS == 1 */

#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
#define DC_MEMS_THREAD_STACK_SIZE           (320U)
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if !defined CUSTOMCLIENT_THREAD_STACK_SIZE
#define CUSTOMCLIENT_THREAD_STACK_SIZE      (0U)
#endif /* !defined CUSTOMCLIENT_THREAD_STACK_SIZE */
#if !defined CUSTOMCLIENT_THREAD
#define CUSTOMCLIENT_THREAD                 (0)
#endif /* !defined CUSTOMCLIENT_THREAD */

#if (USE_ECHO_CLIENT == 1)
#define ECHOCLIENT_THREAD_STACK_SIZE        (448U)
#endif /* (USE_ECHO_CLIENT == 1) */

#if (USE_HTTP_CLIENT == 1)
#define HTTPCLIENT_THREAD_STACK_SIZE        (448U)
#endif /* (USE_HTTP_CLIENT == 1) */

#if (USE_PING_CLIENT == 1)
#define PINGCLIENT_THREAD_STACK_SIZE        (448U)
#endif /* (USE_PING_CLIENT == 1) */

#if (USE_COM_CLIENT == 1)
#define COMCLIENT_THREAD_STACK_SIZE         (448U)
#endif /* (USE_COM_CLIENT == 1) */

#if (USE_MQTT_CLIENT == 1)
#define MQTTCLIENT_THREAD_STACK_SIZE        (4096U)
#endif /* (USE_MQTT_CLIENT == 1) */

#if (USE_UI_CLIENT == 1)
#define UICLIENT_THREAD_STACK_SIZE          (576U)
#endif /* (USE_UI_CLIENT == 1) */

#if (USE_MBEDTLS == 1)
#if !defined MBEDTLS_STACK_SIZE
#define MBEDTLS_STACK_SIZE                  (60000U)
#endif /* !defined MBEDTLS_STACK_SIZE */
#else
#define MBEDTLS_STACK_SIZE                  (0U)
#endif /* USE_MBEDTLS == 1 */

#if (USE_CMD_CONSOLE == 1)
#if (USE_MQTT_CLIENT == 1)
#define CMD_THREAD_STACK_SIZE               (2048U)
#else /* USE_MQTT_CLIENT == 0 */
#define CMD_THREAD_STACK_SIZE               (600U)
#endif /* USE_MQTT_CLIENT == 1 */
#endif /* (USE_CMD_CONSOLE == 1) */

#if (USE_NETWORK_LIBRARY == 1)
#define NET_CELLULAR_BASE_THREAD_STACK_SIZE  DEFAULT_THREAD_STACK_SIZE
#endif /* (USE_NETWORK_LIBRARY == 1) */

#if ((USE_STACK_ANALYSIS == 1) && (STACK_ANALYSIS_TIMER != 0U))
#define STACK_ANALYSIS_THREAD_STACK_SIZE    (384U)
#endif /* (USE_STACK_ANALYSIS == 1) && (STACK_ANALYSIS_TIMER != 0U) */

/* ========================*/
/* END - Stack Size        */
/* ========================*/

#define USED_ATCORE_THREAD_STACK_SIZE            ATCORE_THREAD_STACK_SIZE
#define USED_CELLULAR_SERVICE_THREAD_STACK_SIZE  CELLULAR_SERVICE_THREAD_STACK_SIZE
#define USED_DEFAULT_THREAD_STACK_SIZE           DEFAULT_THREAD_STACK_SIZE
#define USED_FREERTOS_TIMER_THREAD_STACK_SIZE    FREERTOS_TIMER_THREAD_STACK_SIZE
#define USED_FREERTOS_IDLE_THREAD_STACK_SIZE     FREERTOS_IDLE_THREAD_STACK_SIZE

#define USED_ATCORE_THREAD            1
#define USED_CELLULAR_SERVICE_THREAD  1
#define USED_DEFAULT_THREAD           1
#define USED_FREERTOS_TIMER_THREAD    1
#define USED_FREERTOS_IDLE_THREAD     1


#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
/* check value in FreeRTOSConfig.h */
#define USED_TCPIP_THREAD_STACK_SIZE             TCPIP_THREAD_STACK_SIZE
#define USED_TCPIP_THREAD                        1
#else
#define USED_TCPIP_THREAD_STACK_SIZE             0U
#define USED_TCPIP_THREAD                        0
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#define USED_PPPOSIF_CLIENT_THREAD_STACK_SIZE    PPPOSIF_CLIENT_THREAD_STACK_SIZE
#define USED_PPPOSIF_CLIENT_THREAD               1
#else
#define USED_PPPOSIF_CLIENT_THREAD_STACK_SIZE    0U
#define USED_PPPOSIF_CLIENT_THREAD               0
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#if (USE_BUTTONS == 1)
#define USED_BOARD_BUTTONS_THREAD_STACK_SIZE      BOARD_BUTTONS_THREAD_STACK_SIZE
#define USED_BOARD_BUTTONS_THREAD                 1
#else
#define USED_BOARD_BUTTONS_THREAD_STACK_SIZE      0U
#define USED_BOARD_BUTTONS_THREAD                 0
#endif /* USE_BUTTONS == 1 */

#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
#define USED_DC_MEMS_THREAD_STACK_SIZE           DC_MEMS_THREAD_STACK_SIZE
#define USED_DC_MEMS_THREAD                      1
#else
#define USED_DC_MEMS_THREAD_STACK_SIZE           0U
#define USED_DC_MEMS_THREAD                      0
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if (USE_CMD_CONSOLE == 1)
#define USED_CMD_THREAD_STACK_SIZE               CMD_THREAD_STACK_SIZE
#define USED_CMD_THREAD                          1
#else
#define USED_CMD_THREAD_STACK_SIZE               0U
#define USED_CMD_THREAD                          0
#endif /* (USE_CMD_CONSOLE == 1) */

#define USED_CUSTOMCLIENT_THREAD_STACK_SIZE      CUSTOMCLIENT_THREAD_STACK_SIZE
#define USED_CUSTOMCLIENT_THREAD                 CUSTOMCLIENT_THREAD

#if (USE_ECHO_CLIENT == 1)
#define USED_ECHOCLIENT_THREAD_STACK_SIZE        ECHOCLIENT_THREAD_STACK_SIZE
#define USED_ECHOCLIENT_THREAD                   1
#else
#define USED_ECHOCLIENT_THREAD_STACK_SIZE        0U
#define USED_ECHOCLIENT_THREAD                   0
#endif /* (USE_ECHO_CLIENT == 1) */

#if (USE_HTTP_CLIENT == 1)
#define USED_HTTPCLIENT_THREAD_STACK_SIZE        HTTPCLIENT_THREAD_STACK_SIZE
#define USED_HTTPCLIENT_THREAD                   1
#else
#define USED_HTTPCLIENT_THREAD_STACK_SIZE        0U
#define USED_HTTPCLIENT_THREAD                   0
#endif /* (USE_HTTP_CLIENT == 1) */

#if (USE_PING_CLIENT == 1)
#define USED_PINGCLIENT_THREAD_STACK_SIZE        PINGCLIENT_THREAD_STACK_SIZE
#define USED_PINGCLIENT_THREAD                   1
#else
#define USED_PINGCLIENT_THREAD_STACK_SIZE        0U
#define USED_PINGCLIENT_THREAD                   0
#endif /* (USE_PING_CLIENT == 1) */

#if (USE_COM_CLIENT == 1)
#define USED_COMCLIENT_THREAD_STACK_SIZE         COMCLIENT_THREAD_STACK_SIZE
#define USED_COMCLIENT_THREAD                    1
#else
#define USED_COMCLIENT_THREAD_STACK_SIZE         0U
#define USED_COMCLIENT_THREAD                    0
#endif /* (USE_COM_CLIENT == 1) */

#if (USE_MQTT_CLIENT == 1)
#define USED_MQTTCLIENT_THREAD_STACK_SIZE        MQTTCLIENT_THREAD_STACK_SIZE
#define USED_MQTTCLIENT_THREAD                   1
#else
#define USED_MQTTCLIENT_THREAD_STACK_SIZE        0U
#define USED_MQTTCLIENT_THREAD                   0
#endif /* (USE_MQTT_CLIENT == 1) */

#if (USE_UI_CLIENT == 1)
#define USED_UICLIENT_THREAD_STACK_SIZE          UICLIENT_THREAD_STACK_SIZE
#define USED_UICLIENT_THREAD                     1
#else
#define USED_UICLIENT_THREAD_STACK_SIZE          0U
#define USED_UICLIENT_THREAD                     0
#endif /* (USE_UI_CLIENT == 1) */

#if (USE_NETWORK_LIBRARY == 1)
#define USED_NET_CELLULAR_THREAD_STACK_SIZE      NET_CELLULAR_BASE_THREAD_STACK_SIZE
#define USED_NET_CELLULAR_THREAD                 1
#else
#define USED_NET_CELLULAR_THREAD_STACK_SIZE      0U
#define USED_NET_CELLULAR_THREAD                 0
#endif /* (USE_NETWORK_LIBRARY == 1) */

#if ((USE_STACK_ANALYSIS == 1) && (STACK_ANALYSIS_TIMER != 0U))
#define USED_STACK_ANALYSIS_THREAD_STACK_SIZE    STACK_ANALYSIS_THREAD_STACK_SIZE
#define USED_STACK_ANALYSIS_THREAD               1
#else
#define USED_STACK_ANALYSIS_THREAD_STACK_SIZE    0U
#define USED_STACK_ANALYSIS_THREAD               0
#endif /* (USE_STACK_ANALYSIS == 1) && (STACK_ANALYSIS_TIMER != 0U) */

/* ============================================*/
/* BEGIN - Total Stack Size/Number Calculation */
/* ============================================*/

#define TOTAL_THREAD_STACK_SIZE                \
  (size_t)(USED_TCPIP_THREAD_STACK_SIZE        \
           +USED_DEFAULT_THREAD_STACK_SIZE              \
           +USED_FREERTOS_TIMER_THREAD_STACK_SIZE       \
           +USED_FREERTOS_IDLE_THREAD_STACK_SIZE        \
           +USED_PPPOSIF_CLIENT_THREAD_STACK_SIZE       \
           +USED_ATCORE_THREAD_STACK_SIZE               \
           +USED_CELLULAR_SERVICE_THREAD_STACK_SIZE     \
           +USED_BOARD_BUTTONS_THREAD_STACK_SIZE        \
           +USED_DC_MEMS_THREAD_STACK_SIZE              \
           +USED_CMD_THREAD_STACK_SIZE                  \
           +USED_CUSTOMCLIENT_THREAD_STACK_SIZE         \
           +USED_ECHOCLIENT_THREAD_STACK_SIZE           \
           +USED_HTTPCLIENT_THREAD_STACK_SIZE           \
           +USED_PINGCLIENT_THREAD_STACK_SIZE           \
           +USED_COMCLIENT_THREAD_STACK_SIZE            \
           +USED_MQTTCLIENT_THREAD_STACK_SIZE           \
           +USED_UICLIENT_THREAD_STACK_SIZE             \
           +USED_NET_CELLULAR_THREAD_STACK_SIZE         \
           +USED_STACK_ANALYSIS_THREAD_STACK_SIZE)

#define THREAD_NUMBER                \
  (uint8_t)(USED_TCPIP_THREAD        \
            +USED_DEFAULT_THREAD               \
            +USED_FREERTOS_TIMER_THREAD        \
            +USED_FREERTOS_IDLE_THREAD         \
            +USED_PPPOSIF_CLIENT_THREAD        \
            +USED_ATCORE_THREAD                \
            +USED_CELLULAR_SERVICE_THREAD      \
            +USED_DC_MEMS_THREAD               \
            +USED_BOARD_BUTTONS_THREAD         \
            +USED_CMD_THREAD                   \
            +USED_CUSTOMCLIENT_THREAD          \
            +USED_ECHOCLIENT_THREAD            \
            +USED_HTTPCLIENT_THREAD            \
            +USED_PINGCLIENT_THREAD            \
            +USED_COMCLIENT_THREAD             \
            +USED_MQTTCLIENT_THREAD            \
            +USED_UICLIENT_THREAD              \
            +USED_NET_CELLULAR_THREAD          \
            +USED_STACK_ANALYSIS_THREAD)

#ifndef APPLICATION_HEAP_SIZE
#define APPLICATION_HEAP_SIZE       (0U)
#endif  /* APPLICATION_HEAP_SIZE */

/*
PARTIAL_HEAP_SIZE is used by:
- RTOS Timer/Mutex/Semaphore/Message objectd and extra pvPortMalloc call
- MBEDTLS if activated
*/
/* cost by:
   Mutex/Semaphore # 88 bytes
   Queue           # 96 bytes
   Thread          #104 bytes
   Timer           # 56 bytes
*/
#define PARTIAL_HEAP_SIZE   ((THREAD_NUMBER * 600U)           \
                             + (size_t)(MBEDTLS_STACK_SIZE))
#define TOTAL_HEAP_SIZE     ((TOTAL_THREAD_STACK_SIZE * 4U)   \
                             + (size_t)(PARTIAL_HEAP_SIZE)     \
                             + (size_t)(APPLICATION_HEAP_SIZE))

/* ============================================*/
/* END - Total Stack Size/Number Calculation   */
/* ============================================*/

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_THREAD_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
