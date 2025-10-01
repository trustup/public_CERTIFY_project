/**
  ******************************************************************************
  * @file    at_custom_modem_specific.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_specific.c module for
  *          MURATA-TYPE1SC-EVK module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_CUSTOM_MODEM_TYPE1SC_H
#define AT_CUSTOM_MODEM_TYPE1SC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_signalling.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
/* device specific parameters */
#define MODEM_MAX_SOCKET_TX_DATA_SIZE   CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE
#define MODEM_MAX_SOCKET_RX_DATA_SIZE   CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE
#define TYPE1SC_PING_LENGTH             ((uint16_t)56U)
#define TYPE1SC_ACTIVATE_PING_REPORT    (1)


/* Exported types ------------------------------------------------------------*/
typedef enum
{
  /* modem specific commands */
  CMD_AT_PDNSET = (CMD_AT_LAST_GENERIC + 1U), /* set run-time PDN parameters for data PDNs */
  CMD_AT_CCID,                                /* read the ICCID from SIM */
  CMD_AT_AND_K3,                              /* flow control activated (FAST UART) */
  CMD_AT_AND_K0,                              /* flow control deactivated */
  CMD_AT_SETCFG,                              /* set Modem Normal configuration parameters */
  CMD_AT_GETCFG,                              /* get Modem Normal configuration parameters */
  CMD_AT_SETACFG,                             /* set Modem Advanced configuration parameters */
  CMD_AT_GETACFG,                             /* get Modem Advanced configuration parameters */
  CMD_AT_SETBDELAY,                           /* set modem boot delay */
  CMD_AT_DNSRSLV,                             /* resolve a specific domain name */
  CMD_AT_PINGCMD,                             /* execute PING services */
  CMD_AT_PDNACT,                              /* activate or deactivate a specific PDN */
  CMD_AT_SOCKETCMD_ALLOCATE,                  /* allocate socket session */
  CMD_AT_SOCKETCMD_ACTIVATE,                  /* activate the predefined socket ID */
  CMD_AT_SOCKETCMD_INFO,                      /* returns the details of the specific socket ID */
  CMD_AT_SOCKETCMD_DEACTIVATE,                /* req to deactivate the specific socket ID and release its resources */
  CMD_AT_SOCKETCMD_FASTEND,                   /* activates the predefined socket ID,
                                               * writes to the socket and deactivates it */
  CMD_AT_SOCKETCMD_DELETE,                    /* delete the specific socket ID */
  CMD_AT_SOCKETCMD_LASTERROR,                 /* get last socket error code */
  CMD_AT_SOCKETCMD_CONFSEND,                  /* similar to FASTSEND with additions */
  CMD_AT_SOCKETCMD_SSLINFO,                   /* */
  CMD_AT_SOCKETDATA_SEND,                     /* send DATA */
  CMD_AT_SOCKETDATA_RECEIVE,                  /* received DATA */
  CMD_AT_SOCKETEV,                            /* notify about socket events */
  CMD_AT_BOOTEV,                              /* notify about modem boot or reset events */

} ATCustom_TYPE1SC_cmdlist_t;

typedef enum
{
  SETGETCFG_UNDEFINED,
  SETGETCFG_SIM_POLICY,  /* Normal command (setcfg/getcfg) */
  SETGETCFG_BAND,        /* Normal command (setcfg/getcfg) */
  SETGETCFG_OPER,        /* Normal command (setcfg/getcfg) */
  SETGETCFG_HIFC_MODE,   /* Advanced command (setacfg/getacfg) */
  SETGETCFG_PMCONF_SLEEP_MODE,   /* Advanced command (setacfg/getacfg) */
  SETGETCFG_PMCONF_MAX_ALLOWED,  /* Advanced command (setacfg/getacfg) */
  SETGETCFG_BOOT_EVENT_TRUE,          /* Advanced command (setacfg/getacfg) */
  SETGETCFG_BOOT_EVENT_FALSE,         /* Advanced command (setacfg/getacfg) */
  SETGETCFG_UART_FLOW_CONTROL,        /* Advanced command (setacfg/getacfg) */
} ATCustom_TYPE1SC_SETGETCFG_t;

typedef struct
{
  AT_CHAR_t hostIPaddr[MAX_SIZE_IPADDR]; /* = host_name parameter from CS_DnsReq_t */
} ATCustom_TYPE1SC_dns_t;

typedef struct
{
  ATCustom_TYPE1SC_dns_t        DNSRSLV_dns_info;      /* memorize infos received for DNS in %DNSRSLV */
  at_bool_t                     SocketCmd_Allocated_SocketID;
  ATCustom_TYPE1SC_SETGETCFG_t  getcfg_function;
  ATCustom_TYPE1SC_SETGETCFG_t  setcfg_function;
  bool                          wait_for_modem_low_power_ack;
  bool                          modem_in_low_power_state;
  bool                          modem_waiting_for_bootev;
  bool                          modem_bootev_received;
} type1sc_shared_variables_t;

/* External variables --------------------------------------------------------*/
extern type1sc_shared_variables_t type1sc_shared;

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void        ATCustom_TYPE1SC_init(atparser_context_t *p_atp_ctxt);
uint8_t     ATCustom_TYPE1SC_checkEndOfMsgCallback(uint8_t rxChar);
at_status_t ATCustom_TYPE1SC_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout);
at_endmsg_t ATCustom_TYPE1SC_extractElement(atparser_context_t *p_atp_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos);
at_action_rsp_t ATCustom_TYPE1SC_analyzeCmd(at_context_t *p_at_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos);
at_action_rsp_t ATCustom_TYPE1SC_analyzeParam(at_context_t *p_at_ctxt,
                                              const IPC_RxMessage_t *p_msg_in,
                                              at_element_info_t *element_infos);
at_action_rsp_t ATCustom_TYPE1SC_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos);

at_status_t ATCustom_TYPE1SC_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_TYPE1SC_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_TYPE1SC_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_TYPE1SC_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate);

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_MODEM_TYPE1SC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
