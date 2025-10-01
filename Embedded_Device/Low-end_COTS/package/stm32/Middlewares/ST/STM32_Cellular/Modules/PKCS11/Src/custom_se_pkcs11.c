/**
  ******************************************************************************
  * @file    mbedtls_ssl_client.c
  * @author  SMD FAE EMEA
  * @brief   SSL example based on MbedTLS ssl_client.c for mutual authentication
  * 		 to server to get HTTP response
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

/* Includes ------------------------------------------------------------------*/
#include "custom_se_pkcs11.h"
#include "trace_interface.h"

/* Private defines -----------------------------------------------------------*/
# define 	SALT_LEN	32
/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

#define P11(X) g_pFunctionList->X

CK_FUNCTION_LIST_PTR g_pFunctionList;

#define PRINT_APP(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_CUSTOMCLIENT, DBL_LVL_P0, "" format, ## args)

/* Private variables ---------------------------------------------------------*/
static CK_BYTE crtLabel[] = "Identity Certificate";
static CK_BYTE objID[] = "MUD file URL";
static CK_BYTE secKeyLabel[] = "PSK";


static CK_BBOOL bFalse = CK_FALSE;
static CK_BBOOL bTrue  = CK_TRUE;


//static CK_FUNCTION_LIST_PTR g_pFunctionList = NULL;

static CK_SESSION_HANDLE hSession = 0;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private function Definition -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialization
  * @note   custom SE PKCS11 init - first function called
  * @param  -
  * @retval return 0 is OK
  */
int32_t csp_initialize()
{
	CK_RV rv;
	CK_SLOT_ID slotID = 1;
	/* Get PKCS11 Function List */
	rv = C_GetFunctionList(&g_pFunctionList);
	if (rv != CKR_OK)
	{
		PRINT_APP("C_GetFunctionList failed with 0x%08x \n", rv);
		goto exit;
	}
	/* Initialize */
	rv = P11(C_Initialize)(NULL_PTR);
	if (rv != CKR_OK)
	{
		PRINT_APP("C_Initialize, expected CKR_OK but was: 0x%08X \n", rv);
		goto exit;
	}

	/* OpenSession */
	rv = P11(C_OpenSession)(slotID, CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL_PTR, NULL_PTR, &hSession); /* open session on slot 1 */
	if (rv != CKR_OK)
	{
		PRINT_APP("C_OpenSession, expected CKR_OK but was: 0x%08X \n", rv);
		goto exit;
	}
exit:

	return rv;
}

/**
  * @brief  Import the AES-256 pre-shared key (PSK) into the SE
  * @note   A PSK is a 32-byte array in hexadecimal format.
  * @param  pSKValue - PSK value
  * @retval return 0 means OK
  */
int32_t csp_installPSK(unsigned char* pSKValue)
{
	CK_OBJECT_HANDLE objHandle;
	CK_RV rv;
	CK_OBJECT_CLASS seckeyClass = CKO_SECRET_KEY;
	CK_ATTRIBUTE secKeyTemplate[]=
	{
			{CKA_CLASS, &seckeyClass, sizeof(seckeyClass)},
			{CKA_LABEL, &secKeyLabel, sizeof(secKeyLabel)},
			{CKA_TOKEN, &bTrue, sizeof(bTrue)},
			{CKA_VALUE, pSKValue, 32}
	};
	rv = P11(C_CreateObject)(hSession, secKeyTemplate, sizeof(secKeyTemplate)/sizeof(CK_ATTRIBUTE), &objHandle);
	if (rv != CKR_OK)
	{
		PRINT_APP("Error in C_CreateObject 0x%08X\n", rv);
	}


	return rv;

}

/**
  * @brief  Finalization
  * @note   custom SE PKCS11 finalize
  * @param  -
  * @retval return 0 is OK
  */
int32_t csp_terminate()
{
	CK_RV rv;

	/* OpenSession */
	rv = P11(C_CloseSession)(hSession); /* close session */
	if (rv != CKR_OK)
	{
		PRINT_APP("C_CloseSession, expected CKR_OK but was: 0x%08X \n", rv);
		goto exit;
	}
	rv = P11(C_Finalize)(NULL_PTR);
	if (rv != CKR_OK)
	{
		PRINT_APP("C_Finalize, expected CKR_OK but was: 0x%08X\n", rv);
	}

exit:

	return rv;
}

/**
  * @brief  Calculate signature using the algorithm specified by the parameter "algo"
  * @note   signatureKeyID admitted values: "PSK", "MSK", "PSK_1", "PSK_2", "PSK_3", "PSK_4", "PSK_5", "PSK_6", "PSK_7", "PSK_8".
  * @note   algo admitted values: "HMAC", "CMAC"
  * @note   values for baseKeyID: "PSK_[1-8]" are only valid for "HMAC"
  * @param  signatureKeyID    - Key ID of the signature key
  * @param  dataToSign        - Data to sign (a 32-byte (16 for CMAC) array in hexadecimal format)
  * @param  dataToSignLen     - Size of data
  * @param  signature         - The location that receives the signature
  * @param  algo              - The algorithm used to sign
  * @retval return 0 means OK
  */
int32_t csp_sign(char* signatureKeyID, unsigned char* dataToSign, uint16_t dataToSignLen, unsigned char* signature,uint16_t algo)
{

  CK_RV rv;
  uint32_t *sig_size;
  CK_OBJECT_HANDLE objHandle, hSecretKey;

  CK_OBJECT_CLASS secClass = CKO_SECRET_KEY;

  CK_OBJECT_HANDLE arrayObj[10];
  CK_ULONG ulMaxObjectCount = 100;
  CK_ULONG pulObjCount;

  CK_MECHANISM mechanism = {CKM_AES_MAC, NULL_PTR, 0};
  CK_ATTRIBUTE secreyKeyTemplate[] =
  {
    {CKA_CLASS,    &secClass,   sizeof(secClass)},
    {CKA_LABEL,     signatureKeyID,   strlen(signatureKeyID)}

  };


  CK_ULONG ulObjectCount = 1;

  rv = P11(C_FindObjectsInit)(hSession, secreyKeyTemplate, 2);
  if (rv != CKR_OK)
  {
    PRINT_APP("C_FindObjectsInit expected CKR_OK but was: 0x%08X \n", rv);
    goto exit;
  }

  rv = P11(C_FindObjects)(hSession, arrayObj, ulMaxObjectCount, &pulObjCount);
  if (rv != CKR_OK)
  {
    PRINT_APP("C_FindObjects expected CKR_OK but was: 0x%08X \n", rv);
    goto exit;
  }
  else
  {
    if (pulObjCount != ulObjectCount)
    {
      PRINT_APP("C_FindObjects (private key), expected %d key(s) but was: %d", ulObjectCount, pulObjCount);
      goto exit;
    }
    else
    {
      hSecretKey = arrayObj[0];
    }
  }

  rv = P11(C_FindObjectsFinal)(hSession);
  if (rv != CKR_OK)
  {
    PRINT_APP("C_FindObjectsFinal expected CKR_OK but was: 0x%08X \n", rv);
    goto exit;
  }

  rv = P11(C_SignInit)(hSession, &mechanism, hSecretKey);
  if (rv != CKR_OK)
  {
    PRINT_APP("C_SignInit, expected CKR_OK but was: 0x%08X \n", rv);
    goto exit;
  }

  rv = P11(C_Sign)(hSession, dataToSign, dataToSignLen, signature,(uint32_t*)sig_size);
  if (rv != CKR_OK)
  {
    PRINT_APP("C_Sign, expected CKR_OK but was: 0x%08X \n", rv);
  }
exit:
  return rv;
}

/**
  * @brief  Import the Idenetity Certificate in the SE
  * @note   An Identity Certificate is a byte array in hexadecimal format.
  * @param  certificateValue - Value of the Identity Certificate
  * @param  certificateValueLen - Size of Identity Certificate
  * @retval return 0 means OK
  */
int32_t csp_installCertificate(unsigned char* certificateValue,uint16_t certificateValueLen)
{
	int len = 0;
	CK_OBJECT_HANDLE objHandle;
	CK_RV rv;
	CK_OBJECT_CLASS crtClass = CKO_CERTIFICATE;
	if (certificateValue[0] != 0x30 || !(certificateValue[1] & 0x80))
	{
		rv = CKR_DATA_INVALID;
		PRINT_APP("Error in C_CreateObject 0x%08X\n\r", rv);
		return rv;
	}
	if ((certificateValue[1] & 0x0F) <= 1)
		len = certificateValue[2];
	else
		len = (certificateValue[2] << 8) | certificateValue[3];
	CK_ATTRIBUTE crtTemplate[]=
	{
			{CKA_CLASS, &crtClass, sizeof(crtClass)},
			{CKA_LABEL, &crtLabel, sizeof(crtLabel)},
			{CKA_TOKEN, &bTrue, sizeof(bTrue)},
			{CKA_VALUE, certificateValue, len}
	};
	rv = P11(C_CreateObject)(hSession, crtTemplate, sizeof(crtTemplate)/sizeof(CK_ATTRIBUTE), &objHandle);
	if (rv != CKR_OK)
	{
		PRINT_APP("Error in C_CreateObject 0x%08X\n", rv);
	}

	return rv;
}

/**
  * @brief  Import the MUD file URL into the SE
  * @note   The URL of a MUD file is a string like https://www.example.com/yourmudfile.json.
  * @param  mUDuRLValue - Value of the MUD file URL
  * @retval return 0 means OK
  */
int32_t csp_installMudURL(char* mUDuRLValue)
{
	int len = 0;
	CK_OBJECT_HANDLE objHandle;
	CK_RV rv;
	CK_OBJECT_CLASS objClass = CKO_DATA;
    len = strlen(mUDuRLValue);
	CK_ATTRIBUTE dataTemplate[]=
	{
			{CKA_CLASS, &objClass, sizeof(objClass)},
			{CKA_LABEL, &objID, sizeof(objID)},
			{CKA_TOKEN, &bTrue, sizeof(bTrue)},
			{CKA_VALUE, mUDuRLValue, len}
	};
	rv = P11(C_CreateObject)(hSession, dataTemplate, sizeof(dataTemplate)/sizeof(CK_ATTRIBUTE), &objHandle);
	if (rv != CKR_OK)
	{
		PRINT_APP("Error in C_CreateObject 0x%08X\n", rv);
	}

	return rv;
}

/**
  * @brief  Get the value of the Identity Certificate
  * @param  certificateValue - The location that receives the value of the Identity Certificate
  * @param  certificateValueLen - The locateion taht receives the Identity Certificate size
  * @retval return 0 means OK
  */
int32_t csp_getCertificateValue(unsigned char* certificateValue, uint16_t *certificateValueLen)
{
	CK_OBJECT_CLASS crtClass = CKO_CERTIFICATE;
	uint32_t size=0;
	CK_BYTE certValue[2000];
	memset(certValue,0x00,2000);
	CK_ATTRIBUTE crtTemplateFind[] =
	{
			{CKA_CLASS, &crtClass, sizeof(crtClass)},
			{CKA_LABEL, &crtLabel, sizeof(crtLabel)}
	};

	CK_ATTRIBUTE crtTemplateGet[] =
	{
			{CKA_CLASS, &crtClass, sizeof(crtClass)},
			{CKA_VALUE, &certValue, 0}
	};

	CK_OBJECT_HANDLE arrayObj[2];
	CK_ULONG ulMaxObjectCount = 100;
	CK_ULONG ulObjectCount = 2;

	CK_ULONG pulObjCount;

	CK_RV rv;

	/* Retrieves the current certificate */
	rv = P11(C_FindObjectsInit)(hSession, crtTemplateFind, 2);
	if (rv != CKR_OK)
	{
		PRINT_APP("Error on C_FindObjectsInit 0x%08X\n\r", rv);
		goto exit;
	}
	else
	{
		rv = P11(C_FindObjects)(hSession, arrayObj, ulMaxObjectCount, &pulObjCount);
		if (rv != CKR_OK)
		{
			PRINT_APP("Error on C_FindObjects 0x%08X\n\r", rv);
			goto exit;
		}
		else
		{
			rv = P11(C_FindObjectsFinal)(hSession);
			if (rv != CKR_OK)
			{
				PRINT_APP("Error on C_FindObjectsFinal 0x%08X\n\r", rv);
				goto exit;
			}
			else
			{
				/* Gets the certificate data */
				rv = P11(C_GetAttributeValue)(hSession, arrayObj[0], crtTemplateGet, sizeof(crtTemplateGet) / sizeof(CK_ATTRIBUTE));
				if (rv != CKR_OK)
				{
					PRINT_APP("Error on C_GetAttributeValue 0x%08X\n\r", rv);
					goto exit;
				}
				else
				{
					memcpy(certificateValue,certValue,crtTemplateGet[1].ulValueLen );
					certificateValueLen = crtTemplateGet[1].ulValueLen;
				}
			}
		}
	}
exit:
	return rv;
}

/**
  * @brief  Get the URL value of the MUD file
  * @param  mUDuRLValue - The location that receives the MUD file URL
  * @retval return 0 means OK
  */
int32_t csp_getMuDFileURL(char* mUDuRLValue)
{
	CK_OBJECT_CLASS objClass = CKO_DATA;
	uint32_t size=0;
	CK_BYTE mudFileVal[128];
	memset(mudFileVal,0x00,128);
	CK_ATTRIBUTE objTemplateFind[] =
	{
			{CKA_CLASS, &objClass, sizeof(objClass)},
			{CKA_LABEL, &objID, sizeof(objID)},
	};

	CK_ATTRIBUTE objTemplateGet[] =
	{
			{CKA_CLASS, &objClass, sizeof(objClass)},
			{CKA_VALUE, &mudFileVal, 0}
	};

	CK_OBJECT_HANDLE arrayObj[2];
	CK_ULONG ulMaxObjectCount = 100;
	CK_ULONG ulObjectCount = 2;

	CK_ULONG pulObjCount;

	CK_RV rv;

	/* Retrieves the current certificate */
	rv = P11(C_FindObjectsInit)(hSession, objTemplateFind, 2);
	if (rv != CKR_OK)
	{
		PRINT_APP("Error on C_FindObjectsInit 0x%08X\n\r", rv);
		goto exit;
	}
	else
	{
		rv = P11(C_FindObjects)(hSession, arrayObj, ulMaxObjectCount, &pulObjCount);
		if (rv != CKR_OK)
		{
			PRINT_APP("Error on C_FindObjects 0x%08X\n\r", rv);
			goto exit;
		}
		else
		{
			rv = P11(C_FindObjectsFinal)(hSession);
			if (rv != CKR_OK)
			{
				PRINT_APP("Error on C_FindObjectsFinal 0x%08X\n\r", rv);
				goto exit;
			}
			else
			{
				/* Gets the certificate data */
				rv = P11(C_GetAttributeValue)(hSession, arrayObj[0], objTemplateGet, sizeof(objTemplateGet) / sizeof(CK_ATTRIBUTE));
				if (rv != CKR_OK)
				{
					PRINT_APP("Error on C_GetAttributeValue 0x%08X\n\r", rv);
					goto exit;
				}
				else
				{
					memcpy(mUDuRLValue,mudFileVal,objTemplateGet[1].ulValueLen );
				}
			}
		}
	}
exit:
	return rv;
}

/**
  * @brief  Derive a key from a base key using the algorithm specified by the parameter "algo"
  * @note   baseKeyID and derivedKeyID admitted values: "PSK", "MSK", "PSK_1", "PSK_2", "PSK_3", "PSK_4", "PSK_5", "PSK_6", "PSK_7", "PSK_8".
  * @note   algo admitted values: "HMAC", "AES128"
  * @note   values for baseKeyID: "PSK_[1-8]" are only valid for "HMAC"
  * @param  baseKeyID         - Key ID of the base key
  * @param  derivedKeyID      - Key ID of the derived key
  * @param  salt              - Salt value (a 32-byte (16 for AES128) array in hexadecimal format -> e.g., 'fingerprint' or 'challenge')
  * @param  info              - Context and application specific information (e.g., 'Domain manager' or 'Domain ID')
  * @param  infoLen			      - Size of the info
  * @param  algo              - The algorithm used to sign
  * @retval return 0 means OK
  */
int32_t csp_deriveKey(char* baseKeyID, char* derivedKeyID, unsigned char* salt, unsigned char* info, uint16_t infoLen,uint16_t algo)
{
	CK_OBJECT_CLASS sekClass = CKO_SECRET_KEY;
	CK_RV rv;
	CK_KEY_TYPE kt = CKK_GENERIC_SECRET;
	CK_OBJECT_HANDLE derived;
	CK_OBJECT_HANDLE arrayObj[10];
	CK_BBOOL b1 = FALSE;
	CK_BBOOL b2 = FALSE;
	CK_ULONG pulObjCount;
	CK_ATTRIBUTE template[] =
	{
			{CKA_CLASS, &sekClass,sizeof(sekClass)},
			{CKA_KEY_TYPE, &kt,sizeof(kt)},
			{CKA_LABEL, derivedKeyID, strlen(derivedKeyID)}
	};
	CK_HKDF_PARAMS params = {b1, b2,(CK_MECHANISM_TYPE)0,CKF_HKDF_SALT_DATA,salt,SALT_LEN,(CK_OBJECT_HANDLE)0,info, infoLen};

	CK_MECHANISM mechanism = { CKM_HKDF_DERIVE, &params, sizeof(params) };

	CK_ATTRIBUTE baseKeyTemplate[] =
	{
			{CKA_CLASS,   &sekClass,  sizeof(sekClass)},
			{CKA_LABEL,   baseKeyID,  strlen(baseKeyID)},
	};

	CK_ULONG ulObjectCount = 2;
	rv = P11(C_FindObjectsInit)(hSession, baseKeyTemplate, ulObjectCount);
	if (rv != CKR_OK)
	{
		PRINT_APP("C_FindObjectsInit error: 0x%08X\n", rv);
		goto exit;
	}

	CK_ULONG ulMaxObjectCount = 100;
	rv = P11(C_FindObjects)(hSession, arrayObj, ulMaxObjectCount, &pulObjCount);
	if (rv != CKR_OK)
	{
		PRINT_APP("C_FindObjects error: 0x%08X\n", rv);
		goto exit;
	}
	else
	{
		if (pulObjCount != 1)
		{
			PRINT_APP("C_FindObjects (bae key key), expected 1 key but was:: %d", pulObjCount);
			goto exit;
		}
	}

	rv = P11(C_FindObjectsFinal)(hSession);
	if (rv != CKR_OK)
	{
		PRINT_APP("C_FindObjectsFinal error: 0x%08X\n", rv);
		goto exit;
	}


	rv = P11(C_DeriveKey)(hSession, &mechanism, arrayObj[0], template, sizeof(template) / sizeof(CK_ATTRIBUTE), &derived);
	if (rv != CKR_OK)
	{
		PRINT_APP("Error on C_DeriveKey: 0x%08X\n", rv);
		goto exit;
	}

exit:
	return rv;
}

/**
 * @brief Generate random buffer
 * @param randomBuffer reference to generated random data (needs to be allocated beforehand)
 * @param randomBufferLen Byte length of requested data
 * @retval return 0 means OK
 */
int32_t csp_generateRandom(unsigned char *randomBuffer, uint16_t randomBufferLen)
{
	int32_t offset = 0;
	size_t l_s = randomBufferLen;
	CK_RV rv;
	CK_SESSION_HANDLE session = hSession;
	CK_BYTE_PTR data = randomBuffer;

	while (l_s > MAX_RANDOM_LENGTH)
	{
		rv = P11(C_GenerateRandom)(session, randomBuffer, MAX_RANDOM_LENGTH);
		if (rv != CKR_OK)
		{
			/* error while getting random */
			break;
		}
		randomBuffer += MAX_RANDOM_LENGTH;
		l_s -= MAX_RANDOM_LENGTH;
	}

	if (rv != CKR_OK)
	{
		if (l_s > 0)
		{
			rv = P11(C_GenerateRandom)(session, randomBuffer, l_s);
			if (rv != CKR_OK)
			{
				/* error while getting random */
				PRINT_APP("PKCS11 random failed %x\n\r", rv);
			}

		}
	}

	return rv;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
