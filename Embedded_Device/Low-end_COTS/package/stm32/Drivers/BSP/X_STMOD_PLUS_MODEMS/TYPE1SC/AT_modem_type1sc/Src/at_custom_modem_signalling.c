/**
  ******************************************************************************
  * @file    at_custom_modem_signalling.c
  * @author  MCD Application Team
  * @brief   This file provides all the 'signalling' code to the
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_core.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_custom_modem_signalling.h"
#include "at_custom_modem_specific.h"
#include "at_datapack.h"
#include "at_util.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "TYPE1SC API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "TYPE1SC ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P1, (const CRC_CHAR_t *)pbuf, size);
#else
#define PRINT_INFO(format, args...)  (void) printf("TYPE1SC:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("TYPE1SC ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

/* START_PARAM_LOOP and END_PARAM_LOOP macros are used to loop on all fields
*  received in a message.
*  Only non-null length fields are analysed.
*  End the analyze when the end of the message or an error has been detected.
*/
#define START_PARAM_LOOP()  uint8_t exitcode = 0U;\
  do {\
    if (atcc_extractElement(p_at_ctxt, p_msg_in, element_infos) != ATENDMSG_NO) {exitcode = 1U;}\
    if (element_infos->str_size != 0U)\
    {\

#define END_PARAM_LOOP()  }\
  if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
  } while (exitcode == 0U);

/* Private defines -----------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/* Build command functions ---------------------------------------------------*/
at_status_t fCmdBuild_ATD_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATD_TYPE1SC()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PRINT_INFO("Activate PDN (user cid = %d, modem cid = %d)", (uint8_t)current_conf_id, modem_cid)

    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "*99***%d#", modem_cid);
  }
  return (retval);
}

at_status_t fCmdBuild_SETCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SETCFG_TYPE1SC()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (type1sc_shared.setcfg_function == SETGETCFG_SIM_POLICY)
    {
      /* SIM_INIT_SELECT_POLICY
      *  0 : N/A, single SIM
      *  1 : SIM 1 only
      *  2 : SIM 2 only
      *  3 : SIM 1 with fallback to SIM 2
      *  4 : SIM 2 with fallback to SIM 1
      *  5 : iUICC
      */
      uint8_t asim_selected;
      switch (p_modem_ctxt->persist.sim_selected)
      {
        case CS_MODEM_SIM_SOCKET_0:
          asim_selected = 1;
          break;
        case CS_MODEM_SIM_ESIM_1:
          asim_selected = 2;
          break;
        case CS_STM32_SIM_2:
          PRINT_ERR("not supported yet");
          asim_selected = 0;
          retval = ATSTATUS_ERROR;
          break;
        default:
          asim_selected = 0;
          break;
      }

      if (retval != ATSTATUS_ERROR)
      {
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",\"%d\"",
                       "SIM_INIT_SELECT_POLICY",
                       asim_selected);
      }
    }
    else if (type1sc_shared.setcfg_function == SETGETCFG_HIFC_MODE)
    {
      /* advanced command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.hifc.mode,A\"");
    }
    else if (type1sc_shared.setcfg_function == SETGETCFG_BOOT_EVENT_TRUE)
    {
      /* advanced command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"manager.urcBootEv.enabled\",\"true\"");
    }
    else if (type1sc_shared.setcfg_function == SETGETCFG_BOOT_EVENT_FALSE)
    {
      /* advanced command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"manager.urcBootEv.enabled\",\"false\"");
    }
    else
    {
      PRINT_ERR("setcfg value not implemented !")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_GETCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_GETCFG_TYPE1SC()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    switch (type1sc_shared.getcfg_function)
    {
      case SETGETCFG_BAND:
        /* normal command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"BAND\"");
        break;

      case SETGETCFG_OPER:
        /* normal command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"OPER\"");
        break;

      case SETGETCFG_HIFC_MODE:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.hifc.mode\"");
        break;

      case SETGETCFG_PMCONF_SLEEP_MODE:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.conf.sleep_mode\"");
        break;

      case SETGETCFG_PMCONF_MAX_ALLOWED:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.conf.max_allowed_pm_mode\"");
        break;

      case SETGETCFG_UART_FLOW_CONTROL:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"manager.uartB.flowcontrol\"");
        break;

      default:
        retval = ATSTATUS_ERROR;
        break;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SETBDELAY_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SETBDELAY_TYPE1SC()")

  (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");

  return (retval);
}


at_status_t fCmdBuild_PDNSET_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_PDNSET_TYPE1SC()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PRINT_INFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id != CS_PDN_NOT_DEFINED)
    {
      /* check is an username is provided */
      if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username[0] == 0U)
      {
        /* no authentication parameters are provided */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\"",
                       modem_cid,
                       p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn,
                       atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type));
      }
      else
      {
        /* authentication parameters are provided */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\",\"PAP\",\"%s\",\"%s\"",
                       modem_cid,
                       p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn,
                       atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type),
                       p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username,
                       p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.password);

      }
    }
    else
    {
      PRINT_ERR("PDP context not defined for conf_id %d", current_conf_id)
      retval = ATSTATUS_ERROR;
    }
  }
  return (retval);
}


/* Analyze command functions -------------------------------------------------*/
at_action_rsp_t fRspAnalyze_Error_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval;
  PRINT_API("enter fRspAnalyze_Error_TYPE1SC()")

  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_DIAL_COMMAND:
      /* in case of error during socket connection,
      * release the modem CID for this socket_handle
      */
      (void) atcm_socket_release_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);
      break;

    default:
      /* nothing to do */
      break;
  }

  /* analyze Error for TYPE1SC */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case CMD_AT_IFC:
      /* error is ignored for backward compatibility:
       *   this command is not implemented in old FW versions
       */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_CREG:
    case CMD_AT_CGREG:
    case CMD_AT_CEREG:
      /* error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_SETCFG:
    case CMD_AT_GETCFG:
    case CMD_AT_SETACFG:
    case CMD_AT_GETACFG:
    case CMD_AT_SETBDELAY:
      /* modem specific commands, error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    default:
      retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      break;
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CPIN_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CPIN_TYPE1SC()")

  /*
  *   analyze parameters for +CPIN
  *
  *   if +CPIN is not received after AT+CPIN request, it's an URC
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CPIN)
  {
    retval = fRspAnalyze_CPIN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
  }
  else
  {
    START_PARAM_LOOP()
    /* this is an URC */
    if (element_infos->param_rank == 2U)
    {
      PRINT_DBG("URC +CPIN received")
      PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CFUN_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CFUN_TYPE1SC()")

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PRINT_DBG("URC +CFUN received")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_CCID_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fRspAnalyze_QCCID_TYPE1SC()")

  /* analyze parameters for +QCCID */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PRINT_DBG("ICCID:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* if ICCID reported by the modem includes a blank character (space, code=0x20) at the beginning
     *  remove it if this is the case
     */
    uint16_t src_idx = element_infos->str_start_idx;
    size_t ccid_size = element_infos->str_size;
    if ((p_msg_in->buffer[src_idx] == 0x20U) &&
        (ccid_size >= 2U))
    {
      ccid_size -= 1U;
      src_idx += 1U;
    }

    /* copy ICCID */
    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.iccid),
                  (const void *)&p_msg_in->buffer[src_idx],
                  (size_t) ccid_size);
  }
  else
  {
    /* other parameters ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fCmdpAnalyze_GETCFG_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                            const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fCmdpAnalyze_GETCFG_TYPE1SC()")

  /* analyze parameters for GETCFG or GETACFG */
  if (type1sc_shared.getcfg_function == SETGETCFG_UART_FLOW_CONTROL)
  {
    uint32_t hwFC_status = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
    PRINT_DBG("GETCFG/GETACFG: Hw Flow Control setting = %ld", hwFC_status)
    if (hwFC_status == 1U)
    {
      PRINT_DBG("Hw Flow Control setting is ON")
      p_modem_ctxt->persist.flowCtrl_RTS = 2U;
      p_modem_ctxt->persist.flowCtrl_CTS = 2U;
    }
    else
    {
      PRINT_DBG("Hw Flow Control setting is OFF")
      p_modem_ctxt->persist.flowCtrl_RTS = 0U;
      p_modem_ctxt->persist.flowCtrl_CTS = 0U;
    }
  }

  return (retval);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

