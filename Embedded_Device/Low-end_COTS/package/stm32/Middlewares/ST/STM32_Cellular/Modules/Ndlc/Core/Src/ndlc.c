/**
  ******************************************************************************
  * @file    ndlc_core.c
  * @author  MCD Application Team
  * @brief   NDLC module
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


/* Includes ------------------------------------------------------------------*/
#include "ndlc.h"

#if (USE_ST33 == 1)

/* Private defines -----------------------------------------------------------*/
#define NDLC_PCB_SUPERVISOR_ACK             (uint8_t)0xE0   /* PCB Supervisor Ack Frame */
#define NDLC_PCB_SUPERVISOR_ACK_RESEND      (uint8_t)0xE2   /* PCB Supervisor Ack Frame ReSend */
#define NDLC_PCB_SUPERVISOR_NACK            (uint8_t)0xD0   /* PCB Supervisor NAck Frame */
#define NDLC_PCB_SUPERVISOR_NACK_RESEND     (uint8_t)0xD2   /* PCB Supervisor Nack Frame ReSend */
#define NDLC_PCB_DATA                       (uint8_t)0x80   /* PCB Data Frame */
#define NDLC_PCB_DATA_RESEND                (uint8_t)0x84   /* PCB Data Frame ReSend */

/* -- NDLC Protocol, Packet Header possibles Values -- */
#define NDLC_PACKET_HEADER_CHAINING_BIT     (uint8_t)0x80
#define NDLC_PACKET_HEADER_PIPE_IDENTIFIER  (uint8_t)0x60

/* -- NDLC Protocol, Message Header possibles Values -- */
#define NDLC_MESSAGE_HEADER_EVT_C_APDU      (uint8_t)0x50   /* Message From Reader, to send APDU Data Frame */
#define NDLC_MESSAGE_HEADER_EVT_ABORT       (uint8_t)0x51   /* Message From Reader, after Reset */
#define NDLC_MESSAGE_HEADER_EVT_ENDOFAPDU   (uint8_t)0x61   /* Message From Reader, to indicate APDU messages end */

#define NDLC_MESSAGE_HEADER_EVT_R_APDU      (uint8_t)0x50   /* Message From Card, to send APDU Data Frame */
#define NDLC_MESSAGE_HEADER_EVT_WTX         (uint8_t)0x51   /* Message From Card, to send WTX */
#define NDLC_MESSAGE_HEADER_EVT_ATR         (uint8_t)0x52   /* Message From Card, to send ATR */

#define DUMMY_BYTE                          (uint8_t)0xFE   /* Dummy Byte value definition */

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Send and Receive through NDLC
  * @param  dev  - pointer of the device
  * @param  len - outdata length
  * @param  outdata  - pointer of the outdata
  * @param  indata   - pointer of the indata
  * @retval int32_t  - 0/-1 send receive ok/nok
  */

int32_t ndlc_send_receive(ndlc_device_t *p_dev, uint16_t len, uint8_t *outdata, uint8_t *indata)
{
  bool exit = false;
  uint8_t retry;
  uint8_t i;
  uint16_t plen;
  int32_t res;
  uint8_t ack[2];

  p_dev->dataLen = 0;
  retry = 0U;
  plen = 0U;

  while ((retry < 100U) && (exit == false))
  {

#if (NDLC_DEBUG == 1)
    /* log2("status %d retry %d\r\n",dev->status ,retry); */
#endif /* NDLC_DEBUG == 1 */

    switch (p_dev->status)
    {
      case NDLC_IDLE:
        p_dev->status = NDLC_SENDCMD;
        retry = 0U;
        break;

      case NDLC_SENDCMD:
        res = ndlc_send_receive_phy(p_dev->handlePtr, len, outdata, indata);

        if ((res > 0) && (((uint32_t)res) == ((uint32_t)len)))
        {
          if (indata[0] == DUMMY_BYTE)
          {
            p_dev->status = NDLC_READACK;
            retry = 0U;
          }
          else
          {
            retry++;
          }
        }
        else
        {
#if (NDLC_DEBUG == 1)
          /* log1("res %d \r\n",res); */
#endif /* NDLC_DEBUG == 1 */
          retry++;
        }
        break;

      case NDLC_READACK:
        ack[0] = DUMMY_BYTE;
        ack[1] = DUMMY_BYTE;
        if (ndlc_send_receive_phy(p_dev->handlePtr, 2, ack, indata) == 2)
        {
/*
          if ((((indata[0] == NDLC_PCB_SUPERVISOR_ACK) || (indata[0] == NDLC_PCB_SUPERVISOR_NACK)) && indata[1])
              == 0x00)
          {
            dev->status = NDLC_READHEADER;
            retry = 0U;
          }
          else
          {
            retry++;
          }
*/
          if (indata[0] == NDLC_PCB_SUPERVISOR_ACK)
          {
            p_dev->status = NDLC_READHEADER;
            retry = 0U;
          }
          else
          {
/*
            if (((indata[0] == NDLC_PCB_SUPERVISOR_NACK) && indata[1]) == 0x00)
*/
            if (indata[0] == NDLC_PCB_SUPERVISOR_NACK)
            {
              p_dev->status = NDLC_SENDCMD;
              retry = 0U;
            }
          }
        }
        else
        {
          retry++;
        }
        break;

      case NDLC_READHEADER:
        ack[0] = DUMMY_BYTE;
        ack[1] = DUMMY_BYTE;
        if (ndlc_send_receive_phy(p_dev->handlePtr, 2, ack, indata) == 2)
        {
          if (indata[0] == NDLC_PCB_DATA)
          {
            p_dev->status = NLDC_READDATA;
            plen = indata[1];
            retry = 0U;
          }
          else
          {
            retry++;
          }
        }
        else
        {
          retry++;
        }
        break;

      case NLDC_READDATA:
        for (i = 0; i < plen; i++)
        {
          indata[i] = DUMMY_BYTE;
        }
        if (ndlc_send_receive_phy(p_dev->handlePtr, plen, indata, indata) == ((int32_t)plen))
        {
          if ((indata[0] == NDLC_PCB_SUPERVISOR_ACK)
              && ((indata[1] == NDLC_MESSAGE_HEADER_EVT_C_APDU) || (indata[1] == NDLC_MESSAGE_HEADER_EVT_ATR)))
          {
            p_dev->status = NLDC_SENDACK;
            p_dev->dataLen = plen - 2U;
            retry = 0U;
          }
          else if ((indata[0] == NDLC_PCB_SUPERVISOR_ACK) && (indata[1] == NDLC_MESSAGE_HEADER_EVT_WTX)) /* WTX */
          {
            ack[0] = 0xe0;
            ack[1] = 0x00;
            ndlc_send_receive_phy(p_dev->handlePtr, 2, ack, indata);
            HAL_Delay(5);
            p_dev->status = NDLC_READHEADER;
            retry = 0U;
           
          }
          else
          {
            retry++;
          }
        }
        else
        {
          retry++;
        }
        break;

      case NLDC_SENDACK:
        ack[0] = NDLC_PCB_SUPERVISOR_ACK;
        ack[1] = 0x00;
        if (ndlc_send_receive_phy(p_dev->handlePtr, 2, ack, indata) == 2)
        {
          if (indata[0] == DUMMY_BYTE)
          {
            p_dev->status = NDLC_IDLE;
            /* retry = 0U; */
            exit = true;
          }
          else
          {
            retry++;
          }
        }
        else
        {
          retry++;
        }
        break;

      default:
        break;
    }
  }

  if (exit == true)
  {
    res = 0;
  }
  else
  {
    res = -1;
  }
  return (res);
}


#endif /* USE_ST33 == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

