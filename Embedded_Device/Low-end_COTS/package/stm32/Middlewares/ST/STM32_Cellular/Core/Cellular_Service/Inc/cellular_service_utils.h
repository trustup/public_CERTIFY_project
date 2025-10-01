/**
  ******************************************************************************
  * @file    cellular_service_task.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_util.c module
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
#ifndef CELLULAR_SERVICE_UTILS_H
#define CELLULAR_SERVICE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "cellular_service.h"
#include "cellular_service_task.h"
#include "cellular_datacache.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  init modem management
  * @param  -
  * @retval -
  */
void CST_modem_sim_init(void);

/**
  * @brief  update Cellular Info entry of Data Cache
  * @param  dc_service_state - new entry state to set
  * @param  ip_addr - new IP address (null if not defined)
  * @retval -
  */
void CST_data_cache_cellular_info_set(dc_service_rt_state_t dc_service_state, dc_network_addr_t *ip_addr);

/**
  * @brief  configuration failure management
  * @param  msg_fail   - failure message (only for trace)
  * @param  fail_cause - failure cause
  * @param  fail_count - count of failures
  * @param  fail_max   - max of allowed failures
  * @retval -
  */
void CST_config_fail(const uint8_t *msg_fail, cst_fail_cause_t fail_cause, uint8_t *fail_count, uint8_t fail_max);

/**
  * @brief  sets current signal quality values in DC
  * @param  -
  * @retval CS_Status_t - error code
  */
CS_Status_t CST_set_signal_quality(void);

/**
  * @brief  subscribes to modem events
  * @param  -
  * @retval -
  */

void CST_subscribe_modem_events(void);

/**
  * @brief  gets automaton event from message event
  * @param  event  - message event
  * @retval cst_autom_event_t - automaton event
  */
cst_autom_event_t CST_get_autom_event(cst_message_t event);

/**
  * @brief  gets network status
  * @param  -
  * @retval cst_network_status_t - network status
  */
cst_network_status_t  CST_get_network_status(void);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_UTILS_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
