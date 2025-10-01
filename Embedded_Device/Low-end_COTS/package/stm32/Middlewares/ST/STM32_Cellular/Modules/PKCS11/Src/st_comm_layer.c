/**
  ******************************************************************************
  * @file    st_comm_layer.c
  * @author  MCD Application Team
  * @brief   This file provides the functions that send commands to the card.
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

#include "st_p11.h"
#include "st_comm_layer.h"
#include "st_util.h"
#include "com_common.h"
#include "com_sockets_net_compat.h"
#include "com_icc.h"



/*ICC handle*/
static int32_t h_icc;

CK_ULONG InitCommunicationLayer()
{
  CK_ULONG retVal = CKR_OK;
  h_icc = com_icc(COM_AF_UNSPEC, COM_SOCK_SEQPACKET, COM_PROTO_NDLC);
  if (h_icc >= 0x00000000L)
  {
    int32_t ret;
    com_char_t buf_rsp[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ]; /* Only SW + '\0' expected */
    (void)memset(buf_rsp, 0x00, COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ); /* Cleaning Response buffer */
    /* Select Applet */
    com_char_t pSendBuffer[35] = {0x30, 0x30, /* CLA */
                                  0x41, 0x34, /* INS */
                                  0x30, 0x34, /* P1 */
                                  0x30, 0x30, /* P2 */
                                  0x30, 0x43, /* Length of the AID */
                                  /* *** AID ****/
                                  0x41, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x39, 0x35,
                                  0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x39, 0x32,
                                  0x30, 0x30, 0x30, 0x31,
                                  /* End of AID */
                                  '\0' /* Terminator */
                                 };

    /* Transmit to lower level */
    ret = com_icc_generic_access(h_icc, pSendBuffer, 34, buf_rsp, (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

    if (ret >= ((int32_t)(COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1)))
    {
      if (!is9000(buf_rsp, ret))
      {
        /* answer of SE not proper */
        /* transform the wrong answer into PKCS11 style */
        uint8_t charstr[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ];
        hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
        retVal = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
      }

      /*Load Security Environment*/
       com_char_t  pSendBuffer_load[11] = {0x30, 0x30, 0x32, 0x32, 0x46, 0x33,
                                           SETTING_EC256_BSO_KEY_SE_HI_BYTE, SETTING_EC256_BSO_KEY_SE_LO_BYTE,
                                           0x30, 0x30, '\0'
                                          };

       (void)memset(buf_rsp, 0x00, COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ); /* Cleaning Response buffer */

       /* Transmit to lower level */
       ret = com_icc_generic_access(h_icc, pSendBuffer_load, 10, buf_rsp, (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

       if (ret < 0) /* Generic communication error */
       {
         /* Generic communication error */
         /* transform the wrong answer into PKCS11 style */
    	   retVal = composePKCS11Return(ret);
       }
       /*Load Security Environment*/
    }
    else
    {
      /* response Length is too short */
      retVal = composePKCS11Return(ret);
    }
  }
  else
  {
    /* Communication Error */
    retVal = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE - (uint32_t)h_icc);
  }

  return retVal;
}

CK_ULONG CloseCommunication()
{
  CK_ULONG ret;
  int32_t retVal = com_closeicc(h_icc);
  if (retVal == COM_ERR_OK)
  {
    h_icc = 0;
    ret = CKR_OK;
  }
  else
  {
    /* Generic communication error */
    /* transform the wrong answer into PKCS11 style */
    ret = composePKCS11Return(retVal);
  }
  return ret;
}


CK_ULONG ComputeSignature(P11SessionPtr_t pxSessionObj, CK_BYTE *pDataToSign, CK_BYTE DataToSignLen,
                          CK_BYTE *pSignature, CK_ULONG_PTR puSignatureLen)
{
	CK_ULONG retValue = CKR_OK;

	/* Signature length is 32 bytes + SW(2) + '\0' (64 characters + 4 + 1) */
	com_char_t buf_rsp[70]; // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 70
	int32_t ret;

	uint8_t charstr[5];

  	(void)memset(buf_rsp, 0x00, COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ); /* Cleaning Response buffer */

	com_char_t data[3];
	(void)convertHexByteToAscii(pxSessionObj->targetKeyParameter, &data[0], &data[1]);

	/*Compute signature*/
	com_char_t pSendBuffer[SIGN_BUFFER_LENGTH];
	(void)memset(pSendBuffer, 0x00, SIGN_BUFFER_LENGTH);

	/* prepare input buffer - command */
	pSendBuffer[0] = 0x42; /* CLA - 0xB0*/
	pSendBuffer[1] = 0x30;

	pSendBuffer[2] = 0x46; /* INS - 0xF3*/
	pSendBuffer[3] = 0x33;

						   /* P1        */
	//(void)convertHexByteToAscii(DataToSignLen, &data[0], &data[1]);
	pSendBuffer[4] = data[0];
	pSendBuffer[5] = data[1];

	pSendBuffer[6] = 0x30; /* P2  - 0x00*/
	pSendBuffer[7] = 0x30;

	pSendBuffer[8] = 0x32; /* Lc  - 0x20*/
	pSendBuffer[9] = 0x30;

	/* prepare input buffer - data to be signed (digest) */
	CK_BYTE *ppDataSign = pDataToSign;
	for (CK_BYTE i = 0; i < DataToSignLen; i++)
	{
		(void)convertHexByteToAscii(*ppDataSign, &data[0], &data[1]);
		ppDataSign++;
		pSendBuffer[10U + (2U * i)]           = data[0];
		pSendBuffer[10U + (2U * i) + 1U]      = data[1];
	}
	/********************************/
	pSendBuffer[10U + (2U * DataToSignLen)] = (uint8_t)'\0'; /* Terminator */

	int32_t len = (int32_t)10 + ((int32_t)2 * (int32_t)DataToSignLen);

	/* Transmit to lower level */
//	ret = com_icc_generic_access(h_icc, pSendBuffer, len, buf_rsp, (int32_t)MAX_BUFFER_LENGTH_FOR_SIGNATURE);
	ret = com_icc_generic_access(h_icc, pSendBuffer, len, buf_rsp, 70); // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 70

	if (ret < 0)
	{
		/* Generic communication error */
		/* transform the wrong answer into PKCS11 style */
		retValue = composePKCS11Return(ret);
	}

	if (retValue == CKR_OK)
	{
		if (is9000(buf_rsp, ret))
		{
			/* SW OK */
			hexstr_to_char((com_char_t *)buf_rsp, pSignature, (uint32_t)ret - (uint32_t)4);
			*puSignatureLen = (uint32_t)((uint32_t)ret - (uint32_t)4) / (uint32_t)2;
		}
		else
		{
			/* answer of SE not proper */
			/* transform the wrong answer into PKCS11 style */
			hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
			retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
														  * (uint32_t)256) + (uint32_t)charstr[1]);
		}
	}

	return retValue;
}


CK_ULONG encrypt(P11SessionPtr_t pxSessionObj, CK_BYTE *pDataToEncrypt, CK_BYTE DataToEncryptLen,
                          CK_BYTE *pEncryptedData, CK_ULONG_PTR puEncryptedDataLen)
{
	CK_ULONG retValue = CKR_OK;


	com_char_t buf_rsp[140];
	int32_t ret;

	uint8_t charstr[5];

  	(void)memset(buf_rsp, 0x00, COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ); /* Cleaning Response buffer */

	com_char_t data[3];
	(void)convertHexByteToAscii(pxSessionObj->targetKeyParameter, &data[0], &data[1]);

	/*Compute signature*/
	com_char_t pSendBuffer[ENCDEC_BUFFER_LENGTH];
	(void)memset(pSendBuffer, 0x00, ENCDEC_BUFFER_LENGTH);

	/* prepare input buffer - command  da completare con l'APDU di ENC*/
	pSendBuffer[0] = 0x30; /* CLA - */
	pSendBuffer[1] = 0x30;

	pSendBuffer[2] = 0x30; /* INS -*/
	pSendBuffer[3] = 0x30;

						   /* P1        */
	pSendBuffer[4] = data[0];
	pSendBuffer[5] = data[1];

	pSendBuffer[6] = 0x30; /* P2  */
	pSendBuffer[7] = 0x30;

	pSendBuffer[8] = 0x30; /* Lc  */
	pSendBuffer[9] = 0x30;

	/* prepare input buffer - data to be signed (digest) */
	CK_BYTE *ppDataENC = pDataToEncrypt;
	for (CK_BYTE i = 0; i < DataToEncryptLen; i++)
	{
		(void)convertHexByteToAscii(*ppDataENC, &data[0], &data[1]);
		ppDataENC++;
		pSendBuffer[10U + (2U * i)]           = data[0];
		pSendBuffer[10U + (2U * i) + 1U]      = data[1];
	}
	/********************************/
	pSendBuffer[10U + (2U * DataToEncryptLen)] = (uint8_t)'\0'; /* Terminator */

	int32_t len = (int32_t)10 + ((int32_t)2 * (int32_t)DataToEncryptLen);

	/* Transmit to lower level */
//	ret = com_icc_generic_access(h_icc, pSendBuffer, len, buf_rsp, (int32_t)MAX_BUFFER_LENGTH_FOR_SIGNATURE);
	ret = com_icc_generic_access(h_icc, pSendBuffer, len, buf_rsp, 140); // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 140

	if (ret < 0)
	{
		/* Generic communication error */
		/* transform the wrong answer into PKCS11 style */
		retValue = composePKCS11Return(ret);
	}

	if (retValue == CKR_OK)
	{
		if (is9000(buf_rsp, ret))
		{
			/* SW OK */
			hexstr_to_char((com_char_t *)buf_rsp, pEncryptedData, (uint32_t)ret - (uint32_t)4);
			*puEncryptedDataLen = (uint32_t)((uint32_t)ret - (uint32_t)4) / (uint32_t)2;
		}
		else
		{
			/* answer of SE not proper */
			/* transform the wrong answer into PKCS11 style */
			hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
			retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
														  * (uint32_t)256) + (uint32_t)charstr[1]);
		}
	}

	return retValue;
}

CK_ULONG decrypt(P11SessionPtr_t pxSessionObj, CK_BYTE *pEncryptedData, CK_BYTE pEncryptedDataLen,
                          CK_BYTE *pData, CK_ULONG_PTR puDataLen)
{
	CK_ULONG retValue = CKR_OK;


	com_char_t buf_rsp[520];
	int32_t ret;

	uint8_t charstr[5];

  	(void)memset(buf_rsp, 0x00, COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ); /* Cleaning Response buffer */

	com_char_t data[3];
	(void)convertHexByteToAscii(pxSessionObj->targetKeyParameter, &data[0], &data[1]);

	/*Compute signature*/
	com_char_t pSendBuffer[ENCDEC_BUFFER_LENGTH];
	(void)memset(pSendBuffer, 0x00, ENCDEC_BUFFER_LENGTH);

	/* prepare input buffer - command  da completare con l'APDU di ENC*/
	pSendBuffer[0] = 0x30; /* CLA - */
	pSendBuffer[1] = 0x30;

	pSendBuffer[2] = 0x30; /* INS -*/
	pSendBuffer[3] = 0x30;

						   /* P1        */
	pSendBuffer[4] = data[0];
	pSendBuffer[5] = data[1];

	pSendBuffer[6] = 0x30; /* P2  */
	pSendBuffer[7] = 0x30;

	pSendBuffer[8] = 0x30; /* Lc  */
	pSendBuffer[9] = 0x30;

	/* prepare input buffer - data to be signed (digest) */
	CK_BYTE *ppDataDEC = pEncryptedData;
	for (CK_BYTE i = 0; i < pEncryptedDataLen; i++)
	{
		(void)convertHexByteToAscii(*ppDataDEC, &data[0], &data[1]);
		ppDataDEC++;
		pSendBuffer[10U + (2U * i)]           = data[0];
		pSendBuffer[10U + (2U * i) + 1U]      = data[1];
	}
	/********************************/
	pSendBuffer[10U + (2U * pEncryptedDataLen)] = (uint8_t)'\0'; /* Terminator */

	int32_t len = (int32_t)10 + ((int32_t)2 * (int32_t)pEncryptedDataLen);

	/* Transmit to lower level */
//	ret = com_icc_generic_access(h_icc, pSendBuffer, len, buf_rsp, (int32_t)MAX_BUFFER_LENGTH_FOR_SIGNATURE);
	ret = com_icc_generic_access(h_icc, pSendBuffer, len, buf_rsp, 520); // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 140

	if (ret < 0)
	{
		/* Generic communication error */
		/* transform the wrong answer into PKCS11 style */
		retValue = composePKCS11Return(ret);
	}

	if (retValue == CKR_OK)
	{
		if (is9000(buf_rsp, ret))
		{
			/* SW OK */
			hexstr_to_char((com_char_t *)buf_rsp, pData, (uint32_t)ret - (uint32_t)4);
			*puDataLen = (uint32_t)((uint32_t)ret - (uint32_t)4) / (uint32_t)2;
		}
		else
		{
			/* answer of SE not proper */
			/* transform the wrong answer into PKCS11 style */
			hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
			retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
														  * (uint32_t)256) + (uint32_t)charstr[1]);
		}
	}

	return retValue;
}



CK_ULONG retrieveSecretKeysPresence(CK_BYTE *pResponse, CK_BYTE_PTR pResponseLen)
{
	CK_ULONG retValue = CKR_OK;

	/* Signature length is 32 bytes + SW(2) + '\0' (64 characters + 4 + 1) */
	com_char_t buf_rsp[LEN_RESP]; // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 70
	int32_t ret;

	uint8_t charstr[5];

  	(void)memset(buf_rsp, 0x00, LEN_RESP);//COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ); /* Cleaning Response buffer */


	com_char_t pSendBuffer[SIGN_BUFFER_LENGTH];
	(void)memset(pSendBuffer, 0x00, SIGN_BUFFER_LENGTH);

	/* prepare input buffer - command */
	pSendBuffer[0] = 0x42; /* CLA - 0xB0*/
	pSendBuffer[1] = 0x30;

	pSendBuffer[2] = 0x46; /* INS - 0xF4*/
	pSendBuffer[3] = 0x34;

	pSendBuffer[4] = 0x30; /* P1  != 0x00 all keys status      */
	pSendBuffer[5] = 0x31;

	pSendBuffer[6] = 0x30; /* P2  - 0x00*/
	pSendBuffer[7] = 0x31;

	pSendBuffer[8] = 0x30; /* Le  - 0x00*/
	pSendBuffer[9] = 0x30;

	/* Transmit to lower level */
	ret = com_icc_generic_access(h_icc, pSendBuffer, 10, buf_rsp, LEN_RESP); // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 70

	if (ret < 0)
	{
		/* Generic communication error */
		/* transform the wrong answer into PKCS11 style */
		retValue = composePKCS11Return(ret);
	}

	if (retValue == CKR_OK)
	{
		if (is9000(buf_rsp, ret))
		{
			/* SW OK */
			hexstr_to_char((com_char_t *)buf_rsp, pResponse, (uint32_t)ret - (uint32_t)4);
			*pResponseLen = (uint32_t)((uint32_t)ret - (uint32_t)4) / (uint32_t)2;
		}
		else
		{
			/* answer of SE not proper */
			/* transform the wrong answer into PKCS11 style */
			hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
			retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
														  * (uint32_t)256) + (uint32_t)charstr[1]);
		}
	}

	return 0; //retValue;
}

/*This function generates a new public-private key pair*/
CK_ULONG GenerateKeyPair(CK_BYTE BsoId, uint16_t PubKeyFid)
{
  com_char_t buf_rsp[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ]; /* Only SW + '\0' expected */
  int32_t ret;
  CK_BYTE BsoClass = 0x20;

  CK_ULONG retValue = SelectEx(PubKeyFid, NULL);
  if (retValue == CKR_OK)
  {
    /* Prepare BsoClass */
    com_char_t dataBsoClass[3];
    (void)convertHexByteToAscii(BsoClass & (CK_BYTE)0xFC, &dataBsoClass[0], &dataBsoClass[1]);
    /************************/

    /* Prepare BsoId */
    com_char_t dataBsoId[3];
    (void)convertHexByteToAscii(BsoId, &dataBsoId[0], &dataBsoId[1]);

    /* Prepare HiByte for PubKeyFid */
    com_char_t HiBytePubKeyFid[3];
    (void)convertHexByteToAscii(HI_BYTE(PubKeyFid), &HiBytePubKeyFid[0], &HiBytePubKeyFid[1]);

    /* Prepare LoByte for PubKeyFid */
    com_char_t LoBytePubKeyFid[3];
    (void)convertHexByteToAscii(LO_BYTE(PubKeyFid), &LoBytePubKeyFid[0], &LoBytePubKeyFid[1]);
    /********************************************************/

    uint8_t pGenKeyPair[27] = { 0x30, 0x30, /* CLA */
                                0x34, 0x36, /* INS */
                                0x30, 0x30, /* P1 */
                                0x30, 0x30, /* P2 */
                                0x30, 0x38, /* Lc(8)*/
                                dataBsoClass[0], dataBsoClass[1], /* BS object class */
                                dataBsoId[0], dataBsoId[1],       /* BS object id    */
                                HiBytePubKeyFid[0], HiBytePubKeyFid[1], /* fid High Byte pub key */
                                LoBytePubKeyFid[0], LoBytePubKeyFid[1], /* fid Low Byte pub key  */
                                0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, '\0'
                              };
    /* Transmit to lower level */
    ret = com_icc_generic_access(h_icc, pGenKeyPair, 26, buf_rsp, (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

    if (ret < 0)
    {
      /* Generic communication error */
      /* transform the wrong answer into PKCS11 style */
      retValue = composePKCS11Return(ret);
    }

    if (retValue == CKR_OK)
    {
      if (ret == (int32_t)(COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1))
      {
        if (!is9000(buf_rsp, ret))
        {
          /* answer of SE not proper */
          /* transform the wrong answer into PKCS11 style */
          uint8_t charstr[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ];
          hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
          retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                              * (uint32_t)256) + (uint32_t)charstr[1]);
        }
      }
      else
      {
        /* Length of the response is not the one of the SW */
        retValue = CKR_NOT_STATUS_WORD;
      }
    }
  }

  return retValue;
}

/*The function reads all or part of the data in a transparent file inside the SE*/
CK_ULONG ReadBinary(uint16_t Offset, uint16_t dataLen, CK_BYTE_PTR pBuffer)
{
  CK_ULONG retValue = CKR_OK;
  int32_t ret;
  com_char_t buf_rsp[RESP_BUFFER_READ_BINARY_SIZE];

  com_char_t HiByteOffset[3];
  com_char_t LoByteOffset[3];
  com_char_t LoByteDataLen[3];

  int8_t retConvert = convertHexByteToAscii(LO_BYTE(Offset), &LoByteOffset[0], &LoByteOffset[1]);
  if (retConvert != -1)
  {
    retConvert = convertHexByteToAscii(HI_BYTE(Offset), &HiByteOffset[0], &HiByteOffset[1]);
  }

  if (retConvert != -1)
  {
    retConvert = convertHexByteToAscii(LO_BYTE(dataLen), &LoByteDataLen[0], &LoByteDataLen[1]);
  }

  if (retConvert != -1)
  {
    com_char_t pSendBuffer[11] = {0x30, 0x30, 0x42, 0x30, /* CLA + INS */
                                  HiByteOffset[0], HiByteOffset[1], /* P1 */
                                  LoByteOffset[0], LoByteOffset[1], /* P2 */
                                  LoByteDataLen[0], LoByteDataLen[1], '\0'
                                 };
    uint16_t len = dataLen;
    uint16_t dimData = len;
    uint16_t off = Offset;
    int32_t index = 0;

    com_char_t HiByte_off[3];
    com_char_t LoByte_off[3];

    com_char_t data_READ_BINARY_BLOCK_SIZE[3];

    /* Reading is performed in cycle, up to maximum READ_BINARY_BLOCK_SIZE bytes */
    /* Spare bytes are read outside the cycle */

    /* First: cycle */
    while ((dimData > READ_BINARY_BLOCK_SIZE) && (retValue == CKR_OK))
    {
      (void)memset(buf_rsp, 0, RESP_BUFFER_READ_BINARY_SIZE); /* Cleaning Response buffer */

      retConvert = convertHexByteToAscii(LO_BYTE(off), &LoByte_off[0], &LoByte_off[1]);
      if (retConvert != -1)
      {
        retConvert = convertHexByteToAscii(HI_BYTE(off), &HiByte_off[0], &HiByte_off[1]);
        if (retConvert != -1)
        {
          /* CLA + INS (first four items of pSendBuffer) don't change */
          pSendBuffer[4] = HiByte_off[0]; /* P1 */
          pSendBuffer[5] = HiByte_off[1];

          pSendBuffer[6] = LoByte_off[0]; /* P2 */
          pSendBuffer[7] = LoByte_off[1];

          retConvert = convertHexByteToAscii(READ_BINARY_BLOCK_SIZE, &data_READ_BINARY_BLOCK_SIZE[0], \
                                             &data_READ_BINARY_BLOCK_SIZE[1]);
          if (retConvert != -1)
          {
            pSendBuffer[8] = data_READ_BINARY_BLOCK_SIZE[0]; /* Length */
            pSendBuffer[9] = data_READ_BINARY_BLOCK_SIZE[1];

            len = ((dimData + (uint16_t)2) * (uint16_t)2) + (uint16_t)1;

            /* Transmit to lower level */
            ret = com_icc_generic_access(h_icc, pSendBuffer, 10, buf_rsp, (int32_t)len);

            if (ret < 0)
            {
              /* Generic communication error */
              /* transform the wrong answer into PKCS11 style */
              retValue = composePKCS11Return(ret);
            }

            if ((ret >= 0) && (ret < (int32_t)len))
            {
              if (is9000(buf_rsp, ret))
              {
                /* OK */
                hexstr_to_char((com_char_t *)buf_rsp, (pBuffer + (index / (int32_t)2)), (uint32_t)ret - (uint32_t)4);
                index += ret - (int32_t)4;
                off += ((uint16_t)ret / (uint16_t)2) - (uint16_t)2;
                dimData -= ((uint16_t)ret / (uint16_t)2) - (uint16_t)2;
              }
              else
              {
                /* answer of SE not proper */
                /* transform the wrong answer into PKCS11 style */
                uint8_t charstr[5];
                hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
                retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                                    * (uint32_t)256) + (uint32_t)charstr[1]);
              }
            }
            else
            {
              retValue = ((CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + (CK_ULONG)(COM_ERR_PARAMETER * (-1))));
            }
          }
          else
          {
            retValue = CKR_FUNCTION_FAILED; /* conversion error */
          }
        }
        else
        {
          retValue = CKR_FUNCTION_FAILED; /* conversion error */
        }
      }
      else
      {
        retValue = CKR_FUNCTION_FAILED; /* conversion error */
      }
    } /* End of While */

    /* Reading spare bytes */
    if (retValue == CKR_OK)
    {
      /* dimData - represents the number of bytes still to read */
      /* len - represents the number of characters expected in the response buffer */
      len = ((dimData + (uint16_t)2) * (uint16_t)2) + (uint16_t)1; /* Twice(Num of bytes + SW) + Terminator */

      retConvert = convertHexByteToAscii(LO_BYTE(off), &LoByte_off[0], &LoByte_off[1]);
      if (retConvert != -1)
      {
        /* Conversion OK */
        retConvert = convertHexByteToAscii(HI_BYTE(off), &HiByte_off[0], &HiByte_off[1]);
        if (retConvert != -1)
        {
          /* Conversion OK */
          pSendBuffer[4] = HiByte_off[0]; /* P1 */
          pSendBuffer[5] = HiByte_off[1];

          pSendBuffer[6] = LoByte_off[0]; /* P2 */
          pSendBuffer[7] = LoByte_off[1];

          com_char_t dimDataChars[3];
          retConvert = convertHexByteToAscii((uint8_t)dimData, &dimDataChars[0], &dimDataChars[1]);
          if (retConvert != -1)
          {
            /* Conversion OK */
            pSendBuffer[8] = dimDataChars[0]; /* Length */
            pSendBuffer[9] = dimDataChars[1];

            (void)memset(buf_rsp, 0, RESP_BUFFER_READ_BINARY_SIZE);

            /* Transmit to lower level */
            ret = com_icc_generic_access(h_icc, pSendBuffer, 10, buf_rsp, (int32_t)len);

            if (ret < 0)
            {
              /* Generic communication error */
              /* transform the wrong answer into PKCS11 style */
              retValue = composePKCS11Return(ret);
            }

            if (retValue == CKR_OK)
            {
              if (ret < (int32_t)len)
              {
                if (is9000(buf_rsp, ret))
                {
                  /* OK */
                  hexstr_to_char((com_char_t *)buf_rsp, (pBuffer + \
                                                         ((int32_t)index / (int32_t)2)), (uint32_t)ret - (uint32_t)4);
                }
                else
                {
                  /* KO */
                  uint8_t charstr[5];
                  hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
                  retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                                      * (uint32_t)256) + (uint32_t)charstr[1]);
                }
              }
              else
              {
                retValue = ((CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + (CK_ULONG)(COM_ERR_PARAMETER * (-1))));
              }
            }
          }
          else
          {
            retValue = CKR_FUNCTION_FAILED; /* conversion error */
          }
        }
        else
        {
          retValue = CKR_FUNCTION_FAILED; /* conversion error */
        }
      }
      else
      {
        retValue = CKR_FUNCTION_FAILED; /* conversion error */
      }
    }
  }
  else
  {
    retValue = CKR_FUNCTION_FAILED; /* conversion error */
  }

  return retValue;
}

/*The function reads the record rec of the selected record file on the SE */
CK_ULONG ReadRecord(CK_BYTE access_mode, CK_BYTE rec, CK_BYTE *pData, CK_BYTE *pcbData)
{
  CK_ULONG retValue = CKR_OK;
  int32_t ret;
  static com_char_t buf_rsp[RESP_BUFFER_READ_RECORD_SIZE];
  (void)memset(buf_rsp, 0, RESP_BUFFER_READ_RECORD_SIZE); /* Cleaning Response buffer */

  com_char_t datarec[3];
  (void)convertHexByteToAscii(rec, &datarec[0], &datarec[1]);

  com_char_t dataaccess_mode[3];
  (void)convertHexByteToAscii(access_mode, &dataaccess_mode[0], &dataaccess_mode[1]);

  com_char_t datapcbData[3];
  (void)convertHexByteToAscii(*pcbData, &datapcbData[0], &datapcbData[1]);

  com_char_t pSendBuffer[11] = {0x30, 0x30, /* CLA */
                                0x42, 0x32, /* INS */
                                datarec[0], datarec[1], /* Record Number */
                                dataaccess_mode[0], dataaccess_mode[1], /* Access Mode */
                                datapcbData[0], datapcbData[1], '\0'
                               };

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, 10, buf_rsp, (int32_t)RESP_BUFFER_READ_RECORD_SIZE);

  if (ret < 0)
  {
    /* Generic communication error */
    /* Transform the wrong answer into PKCS11 style */
    retValue = composePKCS11Return(ret);
  }

  if (retValue == CKR_OK)
  {
    if (ret < (int32_t)RESP_BUFFER_READ_RECORD_SIZE)
    {
      /* Length of the response doesn't exceed  maximum (RESP_BUFFER_READ_RECORD_SIZE) */
      if (is9000(buf_rsp, ret))
      {
        /* OK */
        hexstr_to_char((com_char_t *)buf_rsp, pData, (uint32_t)ret - (uint32_t)4);
        *pcbData = (CK_BYTE)(((CK_BYTE)ret - (CK_BYTE)4) / (CK_BYTE)2);
      }
      else
      {
        /* KO */
        /* Wrong Status Word */
        /* Transform the wrong SW into PKCS11 style */
        uint8_t charstr[5];
        hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
        retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                            * (uint32_t)256) + (uint32_t)charstr[1]);
      }
    }
    else
    {
      retValue = ((CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + (CK_ULONG)(COM_ERR_PARAMETER * (-1))));
    }
  }

  return retValue;
}

/*The function updates all or part of the selected transparent file on the token*/
CK_ULONG UpdateBinary(CK_BYTE *pData, uint16_t uDataLen, uint16_t ulOffset)
{
  CK_ULONG retValue = CKR_OK;
  int32_t ret;

  if ((pData == NULL) || (uDataLen == (uint16_t)0))
  {
    retValue = CKR_ARGUMENTS_BAD; /* PKCS11 Error to send up to the caller */
  }

  if (retValue == CKR_OK)
  {
    uint16_t ulOffset1 = ulOffset;
    uint16_t ulBlocks = uDataLen / UPDATE_BINARY_BLOCK_SIZE; /* Number of cycles to perform */
    uint16_t ulSpare = uDataLen % UPDATE_BINARY_BLOCK_SIZE;  /* Remaining bytes after cycle */

    com_char_t buf_rsp[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ]; /* Only SW + '\0' expected */
    uint16_t len;

    com_char_t pSendBuffer[11U + (2U * UPDATE_BINARY_BLOCK_SIZE)];
    (void)memset(pSendBuffer, 0, sizeof(pSendBuffer)); /* Cleaning Sending buffer */

    pSendBuffer[0] = 0x30; /* CLA */
    pSendBuffer[1] = 0x30;

    pSendBuffer[2] = 0x44; /* INS */
    pSendBuffer[3] = 0x36;

    com_char_t HiByteOffset1[3];
    com_char_t LoByteOffset1[3];

    com_char_t dataLen[3];

    /* Writing is performed in cycle */
    /* Spare bytes are written outside the cycle */
    for (uint16_t i = 0; ((i < ulBlocks) && (retValue == CKR_OK)); i++)
    {
      (void)convertHexByteToAscii(LO_BYTE(ulOffset1), &LoByteOffset1[0], &LoByteOffset1[1]);
      (void)convertHexByteToAscii(HI_BYTE(ulOffset1), &HiByteOffset1[0], &HiByteOffset1[1]);
      (void)convertHexByteToAscii(LO_BYTE(UPDATE_BINARY_BLOCK_SIZE), &dataLen[0], &dataLen[1]);

      pSendBuffer[4] = HiByteOffset1[0]; /* P1 */
      pSendBuffer[5] = HiByteOffset1[1];

      pSendBuffer[6] = LoByteOffset1[0]; /* P2 */
      pSendBuffer[7] = LoByteOffset1[1];

      pSendBuffer[8] = dataLen[0];
      pSendBuffer[9] = dataLen[1];

      for (uint16_t j = 0; j < UPDATE_BINARY_BLOCK_SIZE; j++)
      {
        (void)convertHexByteToAscii(*(pData + ulOffset1 + j), pSendBuffer + 10 + (j * (uint16_t)2),
                                    pSendBuffer + 10 + (j * (uint16_t)2) + 1);
      }

      len = ((uint16_t)UPDATE_BINARY_BLOCK_SIZE * (uint16_t)2) + (uint16_t)10;

      /* Transmit to lower level */
      ret = com_icc_generic_access(h_icc, pSendBuffer, (int32_t)len, buf_rsp, \
                                   (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

      if (ret < 0)
      {
        /* Generic communication error */
        /* transform the wrong answer into PKCS11 style */
        retValue = composePKCS11Return(ret);
      }
      else
      {
        if (!is9000(buf_rsp, ret))
        {
          /* Wrong Status Word */
          /* Transform the wrong SW into PKCS11 style */
          uint8_t charstr[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ];
          hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
          retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                              * (uint32_t)256) + (uint32_t)charstr[1]);
        }
        else
        {
          ulOffset1 += UPDATE_BINARY_BLOCK_SIZE;
        }
      }
    } /* END OF CYCLE */

    /* Writing spare bytes */
    if (retValue == CKR_OK)
    {
      if (ulSpare > (uint16_t)0)
      {
        (void)convertHexByteToAscii(LO_BYTE(ulOffset1), &LoByteOffset1[0], &LoByteOffset1[1]);
        (void)convertHexByteToAscii(HI_BYTE(ulOffset1), &HiByteOffset1[0], &HiByteOffset1[1]);
        (void)convertHexByteToAscii(LO_BYTE(ulSpare), &dataLen[0], &dataLen[1]);

        (void)memset(pSendBuffer, 0, sizeof(pSendBuffer)); /* Cleaning Sending buffer */
        pSendBuffer[0] = 0x30; /* CLA */
        pSendBuffer[1] = 0x30;

        pSendBuffer[2] = 0x44; /* INS */
        pSendBuffer[3] = 0x36;

        pSendBuffer[4] = HiByteOffset1[0]; /* P1 */
        pSendBuffer[5] = HiByteOffset1[1];

        pSendBuffer[6] = LoByteOffset1[0]; /* P2 */
        pSendBuffer[7] = LoByteOffset1[1];

        pSendBuffer[8] = dataLen[0];
        pSendBuffer[9] = dataLen[1];

        /* convert data */
        for (uint8_t j = 0; j < ulSpare; j++)
        {
          (void)convertHexByteToAscii(*(pData + ulOffset1 + j), pSendBuffer + 10 + (j * (uint16_t)2),
                                      pSendBuffer + 10 + (j * (uint16_t)2) + 1);
        }
        len = (uint16_t)10 + (ulSpare * (uint16_t)2); /* 10 is the length of the command */

        /* Transmit to lower level */
        ret = com_icc_generic_access(h_icc, pSendBuffer, (int32_t)len, buf_rsp, \
                                     (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

        if (ret < 0)
        {
          /* Generic communication error */
          /* Transform the wrong answer into PKCS11 style */
          retValue = composePKCS11Return(ret);
        }
        else
        {
          if (!is9000(buf_rsp, ret))
          {
            /* Wrong Status Word */
            /* Transform the wrong SW into PKCS11 style */
            uint8_t charstr[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ];
            hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
            retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                                * (uint32_t)256) + (uint32_t)charstr[1]);
          }
        }
      }
    }
  }

  return retValue;
}

static CK_ULONG SelectEx(uint16_t fid, uint32_t *SelOpt)
{
  CK_ULONG retValue = CKR_OK;
  int32_t ret;
  com_char_t buf_rsp[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ]; /* Only SW + '\0' expected */

  com_char_t byte_buf_rsp[131]; /* buffer for TLV_Validate and TLV_Get */

  com_char_t HiByte[3];
  com_char_t LoByte[3];

  (void)convertHexByteToAscii(LO_BYTE(fid), &LoByte[0], &LoByte[1]);
  (void)convertHexByteToAscii(HI_BYTE(fid), &HiByte[0], &HiByte[1]);

  uint8_t pSendBuffer[15] = { 0x30, 0x30, /* CLA */
                              0x41, 0x34, /* INS */
                              0x30, 0x30, /* P1 */
                              0x30, 0x43, /* P2  */
                              0x30, 0x32,
                              HiByte[0], HiByte[1], LoByte[0], LoByte[1], '\0'
                            };
  /*
    ** Commented for MISRA; left here for legacy, depending on the value of the second parameter of call
  if (SelOpt != NULL)
  {
  pSendBuffer[6] = 0x30;
  pSendBuffer[7] = 0x30;
  }
  */

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, 14, buf_rsp, (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

  if ((uint32_t)ret < (COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - 1UL))
  {
    /* Length is lower than SW len: it's an error */
    retValue = composePKCS11Return(ret);
  }

  if (retValue == CKR_OK)
  {
    if (is9000(buf_rsp, ret))
    {
      /* OK */
      hexstr_to_char((com_char_t *)buf_rsp, &byte_buf_rsp[0], (uint32_t)ret);

      *SelOpt = 0;
      const CK_BYTE *pTlvData1 = NULL;
      CK_ULONG ulTlvDataLen1 = 0;
      const CK_BYTE *pTlvData2 = NULL;
      CK_ULONG ulTlvDataLen2 = 0;

      CK_BBOOL isOK = false;
      if (!TLV_Validate(byte_buf_rsp, (CK_ULONG)ret - (CK_ULONG)2))
      {
        isOK = true; /* Don't perform following shift and OR */
      }
      else
      {
        if (!TLV_Get(0x6f, byte_buf_rsp, (CK_ULONG)ret - (CK_ULONG)2, &pTlvData1, &ulTlvDataLen1))
        {
          isOK = true; /* Don't perform following shift and OR */
        }
      }

      if (!isOK)
      {
        if (!TLV_Validate(pTlvData1, ulTlvDataLen1))
        {
          isOK = true; /* Don't perform following shift and OR */
        }
        else
        {
          if (!TLV_Get(0x80, pTlvData1, ulTlvDataLen1, &pTlvData2, &ulTlvDataLen2) || (ulTlvDataLen2 != 2UL))
          {
            isOK = true; /* Don't perform following shift and OR */
          }
        }
      }

      if (!isOK)
      {
        /* To be performed only if isOK remained false */
        *SelOpt = (uint32_t)((uint8_t)((pTlvData2[1] << 8) | pTlvData2[2]));
      }

    }
    else
    {
      /* KO */
      /* Wrong Status Word */
      /* Transform the wrong SW into PKCS11 style */
      uint8_t charstr[5];
      hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
      retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
    }
  }

  return retValue;
}

CK_ULONG PathSelectWORD(uint16_t *path, CK_ULONG ulFidsInPath)
{
  CK_BYTE pPath[16];
  for (CK_ULONG i = 0; i < ulFidsInPath; i++) /* copy of path, with the bytes inverted */
  {
    pPath[(CK_BYTE)i * (CK_BYTE)2] = HI_BYTE(path[i]);
    pPath[((CK_BYTE)i * (CK_BYTE)2) + (CK_BYTE)1] = LO_BYTE(path[i]);
  }

  return PathSelect(pPath, ulFidsInPath);
}

static CK_ULONG PathSelect(CK_BYTE *pPath, CK_ULONG ulFidsInPath)
{
  CK_BYTE path1[] = { 0x3F, 0x00 };
  CK_ULONG ulRes = 0;
  uint16_t fid;
  CK_BYTE *ppPath = pPath;

  CK_ULONG internalFidsInPath = ulFidsInPath;
  if ((ulFidsInPath == 0UL) || (pPath == NULL))
  {
    internalFidsInPath = 1;
    ppPath = path1;
  }

  for (CK_ULONG i = 0; i < internalFidsInPath; i++)
  {
    fid = (uint16_t)(((uint16_t)ppPath[i * (CK_ULONG)2] << (uint16_t)8) | \
                     (uint16_t)(ppPath[(i * (CK_ULONG)2) + (CK_ULONG)1]));
    ulRes = SelectEx(fid, NULL);
    if (ulRes != CKR_OK)
    {
      break;
    }
  }

  return ulRes;
}

CK_ULONG GetChallenge(CK_BYTE *pChallenge, CK_ULONG ulChallenge)
{
  CK_ULONG retValue = CKR_OK;
  com_char_t buf_rsp[MAX_BUFFER_LENGTH_FOR_CHALLENGE]; /* Max Len: 128*2 + 4 for SW + 1 for '\0'*/
  int32_t ret;

  (void)memset(buf_rsp, 0x00, MAX_BUFFER_LENGTH_FOR_CHALLENGE); /* Cleaning Response buffer */

  com_char_t data[3];
  (void)convertHexByteToAscii((uint8_t)ulChallenge, &data[0], &data[1]);

  com_char_t pSendBuffer[11] = {0x30, 0x30, /* CLA */
                                0x38, 0x34, /* INS */
                                0x30, 0x30, /* P1 */
                                0x30, 0x30, /* P2 */
                                data[0], data[1], '\0'
                               };

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, 10, buf_rsp, (int32_t)MAX_BUFFER_LENGTH_FOR_CHALLENGE);

  if (ret < 0)
  {
    /* Generic communication error */
    /* transform the wrong answer into PKCS11 style */
    retValue = composePKCS11Return(ret);
  }

  if (retValue == CKR_OK)
  {
    if (ret < MAX_BUFFER_LENGTH_FOR_CHALLENGE)
    {
      uint8_t charstr[MAX_BYTES_FOR_CHALLENGE];
      if (is9000(buf_rsp, ret))
      {
        /* SW is OK */
        hexstr_to_char((com_char_t *)buf_rsp, &charstr[0], (uint32_t)ret - (uint32_t)4);
        (void)memcpy(pChallenge, &charstr[0], ((uint32_t)ret - 4UL) / 2UL);
      }
      else   /* KO */
      {
        /* Wrong Status Word */
        /* Transform the wrong SW into PKCS11 style */
        hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
        retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] \
                                                            * (uint32_t)256) + (uint32_t)charstr[1]);
      }
    }
    else
    {
      /* Response is too long */
      /* An error is sent up in PKCS11 style */
      retValue = ((CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + (CK_ULONG)(COM_ERR_PARAMETER * (-1))));
    }
  }

  return retValue;
}

CK_ULONG GenerateSecret(CK_ECDH1_DERIVE_PARAMS pParams, CK_BYTE *pSecret, int32_t *pulSecret)
{
  CK_ULONG retValue = CKR_OK;
  int32_t ret;

  com_char_t buf_rsp[81];
  (void)memset(buf_rsp, 0x00, 81); /* Cleaning Response buffer */

  com_char_t lenPubkey[3];
  uint32_t lenAPDU;
  com_char_t pSendBuffer[MAX_BUFFER_LENGTH_FOR_GENERATE_SECRET];
  (void)memset(pSendBuffer, 0x00, MAX_BUFFER_LENGTH_FOR_GENERATE_SECRET);

  pSendBuffer[0] = 0x30; /* CLA */
  pSendBuffer[1] = 0x30;

  pSendBuffer[2] = 0x38; /* INS */
  pSendBuffer[3] = 0x36;

  pSendBuffer[4] = 0x30; /* P1 */
  pSendBuffer[5] = 0x30;

  pSendBuffer[6] = 0x30; /* P2 */
  pSendBuffer[7] = 0x30;

  pSendBuffer[8] = 0x34;
  pSendBuffer[9] = 0x41;

  /* Following values do not vary with Public Key value */
  pSendBuffer[10] = 0x37;
  pSendBuffer[11] = 0x43;
  pSendBuffer[12] = 0x34;
  pSendBuffer[13] = 0x38;
  pSendBuffer[14] = 0x41;
  pSendBuffer[15] = 0x30;
  pSendBuffer[16] = 0x34;
  pSendBuffer[17] = 0x36;
  pSendBuffer[18] = 0x38;
  pSendBuffer[19] = 0x34;
  pSendBuffer[20] = 0x30;
  pSendBuffer[21] = 0x31;
  pSendBuffer[22] = 0x31;
  pSendBuffer[23] = 0x37;
  pSendBuffer[24] = 0x38;
  pSendBuffer[25] = 0x36;
  /* *********************************************/

  /* Converting Public Key value */
  (void)convertHexByteToAscii((uint8_t)pParams.ulPublicDataLen, &lenPubkey[0], &lenPubkey[1]);
  pSendBuffer[26] = lenPubkey[0];
  pSendBuffer[27] = lenPubkey[1];

  for (uint16_t j = 0; j < pParams.ulPublicDataLen; j++)
  {
    (void)convertHexByteToAscii(pParams.pPublicData[j], pSendBuffer + 28 + (j * (uint16_t)2),
                                pSendBuffer + 28 + (j * (uint16_t)2) + 1);
  }

  lenAPDU = (pParams.ulPublicDataLen * 2UL) + 28UL;

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, (int32_t)lenAPDU, buf_rsp, (int32_t)81);
  if (ret < 0)
  {
    /* Generic communication error */
    /* Transform the wrong answer into PKCS11 style */
    retValue = composePKCS11Return(ret);
  }
  else
  {
    if (ret <= 76)
    {
      /* 76 = 72 (max expected response length) + 4 (SW) */
      uint8_t charstr[SECRET_KEY_GEN_BUFFER_LEN];
      if (is9000(buf_rsp, ret))
      {
        /* SW is OK */
        hexstr_to_char((com_char_t *)buf_rsp, &charstr[0], ((uint32_t)ret - (uint32_t)4));
        (void)memcpy(pSecret, &charstr[4], (((uint32_t)ret - 4UL) / 2UL) - 4UL);
        *pulSecret = ((ret - 4) / 2) - 4; /* Length of the Secret */
      }
      else
      {
        /* Wrong Status Word */
        /* Transform the wrong SW into PKCS11 style */
        hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
        retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + \
                              ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
      }
    }
    else
    {
      /* LENGTH GREATER THAN EXPECTED: IT'S AN ERROR */
      /* Transform this error into PKCS11 style */
      retValue = ((CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + (CK_ULONG)(COM_ERR_PARAMETER * (-1))));
    }
  }

  return retValue;
}

CK_ULONG DeriveSecret(CK_HKDF_PARAMS pParams, int8_t * baseKeyLabel, char * derivedKeyLabel, CK_BYTE *pSecret, int32_t *pulSecret)
{
	CK_ULONG retValue = CKR_OK;
	int32_t ret;

	com_char_t buf_rsp[81];
	(void)memset(buf_rsp, 0x00, 81); /* Cleaning Response buffer */

	com_char_t lenPubkey[3];
	uint32_t lenAPDU;
	com_char_t pSendBuffer[MAX_BUFFER_LENGTH_FOR_GENERATE_SECRET];
	(void)memset(pSendBuffer, 0x00, MAX_BUFFER_LENGTH_FOR_GENERATE_SECRET);

	pSendBuffer[0] = 0x42; /* CLA - 0xB0*/
	pSendBuffer[1] = 0x30;

	pSendBuffer[2] = 0x46; /* INS - 0xF2*/
	pSendBuffer[3] = 0x32;

	bool result = true;

	/* Set P1 and P2 */
	if (strcmp((CCHAR *)baseKeyLabel, MAIN_SECRET_KEY) == 0)
	{
		/* PSK->MSK */
		pSendBuffer[4] = 0x30; /* P1 */
		pSendBuffer[5] = 0x30;

		pSendBuffer[6] = 0x30; /* P2 */
		pSendBuffer[7] = 0x30;
	}
	else if (strcmp((CCHAR *)baseKeyLabel, FIRST_DERIVED_KEY) == 0)
	{
		/* MSK->PMK_i */
		pSendBuffer[4] = 0x30; /* P1 - 0x00*/
		pSendBuffer[5] = 0x30;

		(void)convertHexByteToAscii((CK_BYTE)derivedKeyLabel[4], &pSendBuffer[6], &pSendBuffer[7]);
	}
	else
	{
		/* PMK_j->PMK_i */
		(void)convertHexByteToAscii((CK_BYTE)baseKeyLabel[4], &pSendBuffer[4], &pSendBuffer[5]);
		(void)convertHexByteToAscii((CK_BYTE)derivedKeyLabel[4], &pSendBuffer[6], &pSendBuffer[7]);
	}


  /* Converting total length */
    (void)convertHexByteToAscii((uint8_t)(pParams.ulSaltLen + pParams.ulInfoLen), &pSendBuffer[8], &pSendBuffer[9]);

  for (uint16_t j = 0; j < pParams.ulSaltLen; j++)
  {
    (void)convertHexByteToAscii(pParams.pSalt[j], pSendBuffer + 10 + (j * (uint16_t)2),
                                pSendBuffer + 10 + (j * (uint16_t)2) + 1);
  }

  for (uint16_t j = 0; j < pParams.ulInfoLen; j++)
  {
    (void)convertHexByteToAscii(pParams.pInfo[j], pSendBuffer + 10 + pParams.ulSaltLen*2 + (j * (uint16_t)2),
                                pSendBuffer + 10 + pParams.ulSaltLen*2 + (j * (uint16_t)2) + 1);
  }

  lenAPDU = ((pParams.ulSaltLen + pParams.ulInfoLen) * 2UL) + 10UL;

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, (int32_t)lenAPDU, buf_rsp, (int32_t)81);
  if (ret < 0)
  {
    /* Generic communication error */
    /* Transform the wrong answer into PKCS11 style */
    retValue = composePKCS11Return(ret);
  }
  else
  {
	  uint8_t charstr[SECRET_KEY_GEN_BUFFER_LEN];
	  if (is9000(buf_rsp, ret))
      {
        /* SW is OK */
    	  retValue = CKR_OK;
      }
      else
      {
        /* Wrong Status Word */
        /* Transform the wrong SW into PKCS11 style */
        hexstr_to_char((com_char_t *)(buf_rsp + ret - 4), &charstr[0], 4);
        retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + \
                              ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
      }

  }

  return retValue;
}

CK_ULONG VerifySignature(const CK_BYTE *pDataToVerify, CK_BYTE DataToVerifyLen, CK_BYTE *pSignature,
                         CK_ULONG puSignatureLen)
{
  CK_ULONG retValue = CKR_OK;
  com_char_t buf_rsp[10]; /* response buffer */
  (void)memset(buf_rsp, 0, 10); /* Cleaning Response buffer */

  com_char_t data[3];
  (void)memset(data, 0, 3);

  int32_t ret;

  com_char_t  pSendBufferVerify[250];
  (void)memset(pSendBufferVerify, 0, 250); /* Cleaning Sending buffer */

  /* preparing buffer - command */
  pSendBufferVerify[0] = 0x30; /* CLA */
  pSendBufferVerify[1] = 0x30;

  pSendBufferVerify[2] = 0x32; /* INS */
  pSendBufferVerify[3] = 0x41;

  pSendBufferVerify[4] = 0x30; /* P1 */
  pSendBufferVerify[5] = 0x30;

  pSendBufferVerify[6] = 0x41; /* P2 */
  pSendBufferVerify[7] = 0x38;

  /* Global Length = Length of data to be verified + Length of Signature + Length of SW */
  CK_BYTE GlobalLength = DataToVerifyLen + (CK_BYTE)puSignatureLen + 4U;
  (void)convertHexByteToAscii(GlobalLength, &data[0], &data[1]);
  pSendBufferVerify[8] = data[0];
  pSendBufferVerify[9] = data[1];
  /* ************************************************************************************/

  pSendBufferVerify[10] = 0x39;
  pSendBufferVerify[11] = 0x41;

  /* **** DataToVerify *****/
  (void)convertHexByteToAscii(DataToVerifyLen, &data[0], &data[1]);
  pSendBufferVerify[12] = data[0];
  pSendBufferVerify[13] = data[1];
  /* ***********************/

  /* The buffer contains both the data to verify and the signature to check */
  /* preparing buffer - value to verify */
  for (uint16_t j = 0; j < DataToVerifyLen; j++)
  {
    (void)convertHexByteToAscii(pDataToVerify[j], pSendBufferVerify + 14 + (j * (uint16_t)2),
                                pSendBufferVerify + 14 + (j * (uint16_t)2) + 1);
  }
  /* ************************************************************************/

  /* Fixed: 0x9E */
  pSendBufferVerify[(CK_BYTE)14 + (DataToVerifyLen * (CK_BYTE)2)]               = 0x39;
  pSendBufferVerify[(CK_BYTE)14 + (DataToVerifyLen * (CK_BYTE)2) + (CK_BYTE)1]  = 0x45;

  /* **** Signature *****/
  (void)convertHexByteToAscii((uint8_t)puSignatureLen, &data[0], &data[1]);

  pSendBufferVerify[(CK_BYTE)14 + (DataToVerifyLen * (CK_BYTE)2) + (CK_BYTE)2] = data[0];
  pSendBufferVerify[(CK_BYTE)14 + (DataToVerifyLen * (CK_BYTE)2) + (CK_BYTE)3] = data[1];

  /* preparing buffer - signature */
  for (uint16_t i = 0; i < puSignatureLen; i++)
  {
    (void)convertHexByteToAscii(pSignature[i],
                                pSendBufferVerify + (uint16_t)14 + (DataToVerifyLen * (uint16_t)2) + \
                                (i * (uint16_t)2) + (uint16_t)4,
                                pSendBufferVerify + (uint16_t)14 + \
                                (DataToVerifyLen * (uint16_t)2) + (i * (uint16_t)2) + (uint16_t)4 + (uint16_t)1);
  }

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBufferVerify, ((int32_t)GlobalLength * (int32_t)2) + (int32_t)10, buf_rsp,
                               (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

  if (ret < 0)
  {
    /* Generic communication error */
    /* transform the wrong answer into PKCS11 style */
    retValue = composePKCS11Return(ret);
  }

  if (retValue == CKR_OK)
  {
    if (ret == (int32_t)(COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1))
    {
      /* expected 4 bytes, length of the SW */
      if (!is9000(buf_rsp, ret))
      {
        /* Wrong SW, sent as a PKCS11 error */
        uint8_t charstr[COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ];
        hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
        retValue = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + \
                              ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
      }
    }
    else
    {
      /* The length of the response is not the one of the SW */
      retValue = CKR_NOT_STATUS_WORD;
    }
  }

  return retValue;
}

CK_ULONG SelectPublicKey(CK_BYTE SelectedBSO)
{
  CK_ULONG retVal = CKR_OK;
  com_char_t buf_rsp[10];
  (void)memset(buf_rsp, 0, 10);

  com_char_t data[3];

  int32_t ret;

  (void)convertHexByteToAscii(SelectedBSO, &data[0], &data[1]);

  com_char_t  pSendBuffer[] =
  {
    0x30, 0x30, 0x32, 0x32, /* CLA + INS */
    0x46, 0x31, 0x42, 0x36, 0x30, 0x33,
    0x38, 0x33, 0x30, 0x31, data[0], data[1], '\0'
  };

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, 16, buf_rsp, (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

  if (ret >= 0)
  {
    /* Communication is OK */
    if (ret == (int32_t)(COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1))
    {
      /* expected 4 bytes, length of the SW */
      if (!is9000(buf_rsp, ret))
      {
        /* Wrong SW, sent as a PKCS11 error */
        uint8_t charstr[5];
        hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
        retVal = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
      }
    }
    else
    {
      /* Wrong length of response */
      /* prepare return in PKCS11 style, VENDOR DEFINED value */
      retVal = CKR_NOT_STATUS_WORD;
    }
  }
  else
  {
    /* KO */
    /* error during communication phase */
    /* return error into PKCS11 style */
    retVal = composePKCS11Return(ret);
  }

  return retVal;
}

CK_ULONG UpdateKey(CK_BYTE *keyValue, CK_BYTE keyLen)
{
  CK_ULONG retVal = CKR_OK;
  com_char_t buf_rsp[10];
  (void)memset(buf_rsp, 0, 10); /* Cleaning Response buffer */

  int32_t ret;

  com_char_t  pSendBuffer[70]; // 32*2 + 4 + 1 = 69~70 *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 70
  (void)memset(pSendBuffer, 0, 70); /* Cleaning  buffer */

  pSendBuffer[0]  =    0x42; /* CLA   */
  pSendBuffer[1]  =    0x30; /* 0xB0  */

  pSendBuffer[2]  =    0x46; /* INS   */
  pSendBuffer[3]  =    0x31; /* 0xF1  */

  pSendBuffer[4]  =    0x30; /* P1    */
  pSendBuffer[5]  =    0x30; /* 0x00  */

  pSendBuffer[6]  =    0x30; /* P2    */
  pSendBuffer[7]  =    0x30; /* 0x00  */

  pSendBuffer[8]  =    0x32; /* Lc  */
  pSendBuffer[9]  =    0x30; /* 0x20 */

  for (uint16_t i = 0; i < keyLen; i++)
  {
    (void)convertHexByteToAscii(keyValue[i], pSendBuffer + 10 + ((uint16_t)2 * i),
                                pSendBuffer + 10 + ((uint16_t)2 * i) + 1);
  }

  /* Transmit to lower level */
  ret = com_icc_generic_access(h_icc, pSendBuffer, (int32_t)10 + ((int32_t)keyLen * (int32_t)2), buf_rsp,
                               (int32_t)COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ);

  if (ret >= 0)
  {
    /* Communication is OK */
    if (ret == (int32_t)(COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1))
    {
      /* expected 4 bytes, length of the SW */
      if (!is9000(buf_rsp, ret))
      {
        /* Wrong SW, sent as a PKCS11 error */
        uint8_t charstr[5];
        hexstr_to_char((com_char_t *)(buf_rsp), &charstr[0], COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ - (uint32_t)1);
        retVal = (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE + ((uint32_t)charstr[0] * (uint32_t)256) + (uint32_t)charstr[1]);
      }
    }
    else
    {
      retVal = CKR_NOT_STATUS_WORD; /* Vendor defined error */
    }
  }
  else
  {
    /* error during communication phase */
    /* Transform into PKCS11 style */
    retVal = composePKCS11Return(ret);
  }
  return retVal;
}

CK_ULONG ExecuteVerifySignature(P11SessionPtr_t pxSessionObj, const CK_BYTE *pDataToVerify, CK_BYTE DataToVerifyLen,
                                CK_BYTE *pSignature, CK_ULONG puSignatureLen)
{
  /* First the Public Key is selected */
  CK_ULONG res = SelectPublicKey(pxSessionObj->targetKeyParameter);
  if (res == CKR_OK)
  {
    /* If Selection is OK, verification is performed */
    res = VerifySignature(pDataToVerify, DataToVerifyLen, pSignature, puSignatureLen);
  }
  /* Result of verification is directly sent up to the caller */
  return res;
}





/* Function for internal utility */
CK_ULONG composePKCS11Return(int32_t ret)
{
  return (CK_ULONG)(CKR_STM32_CUSTOM_ATTRIBUTE - (uint32_t)ret);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
