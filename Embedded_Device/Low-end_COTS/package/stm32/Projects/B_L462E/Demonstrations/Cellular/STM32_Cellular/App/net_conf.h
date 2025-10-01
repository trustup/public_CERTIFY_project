/**
  ******************************************************************************
  * @file    net_conf.h
  * @author  MCD Application Team
  * @brief   This file provides the configuration for net
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

#ifndef NET_CONF_H
#define NET_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plf_config.h"

#if (USE_NETWORK_LIBRARY == 1)
#include "trace_interface.h"

#define NET_CELLULAR_CREDENTIAL_V2   (1)

#define ENABLE_NET_DBG_INFO 1

#if  (ENABLE_NET_DBG_INFO == 1)

#define NET_DBG_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_UTILITIES, DBL_LVL_P1, "network library: " format "\r\n", ## args)
#else
#define NET_DBG_INFO(...)
#endif /* ENABLE_NET_DBG_INFO */

#define NET_DBG_PRINT(format, args...) \
  TRACE_PRINT(DBG_CHAN_UTILITIES, DBL_LVL_P1, "network library: " format "\r\n", ## args)

#define NET_PRINT(format, args...) \
  TRACE_PRINT(DBG_CHAN_UTILITIES, DBL_LVL_P0, "network library: " format "\r\n", ## args)

#define NET_PRINT_WO_CR(format, args...) \
  TRACE_PRINT(DBG_CHAN_UTILITIES, DBL_LVL_P0, "network library: " format "\r\n", ## args)

#define NET_WARNING(format, args...) \
  TRACE_PRINT(DBG_CHAN_UTILITIES, DBL_LVL_P1, "network library: " format "\r\n", ## args)

#define NET_DBG_ERROR(format, args...) \
  TRACE_PRINT(DBG_CHAN_UTILITIES, DBL_LVL_ERR, "network library ERROR: " format "\r\n", ## args)


#if (RTOS_USED == 1)
#define NET_USE_RTOS
#endif /* RTOS_USED == 1 */


#if (USE_MBEDTLS == 1)
#define NET_MBEDTLS_HOST_SUPPORT
#define NET_MBEDTLS_DEVICE_SUPPORT
#define NET_MBEDTLS_CONNECT_TIMEOUT     10000U
#endif /* (USE_MBEDTLS == 1) */

#define NET_ASSERT(test,s)  do { if (!(test)) {\
                                 NET_DBG_ERROR(s) \
                                 while(true) {}; }\
                               } while (false)

#include "net_conf_template.h"
#endif /* USE_NETWORK_LIBRARY == 1 */

#ifdef __cplusplus
}
#endif

#endif /* NET_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
