/**
  ******************************************************************************
  * @file    st_util.c
  * @author  MCD Application Team
  * @brief   This file provides the functions that xxx.
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "st_util.h"
#include "st_p11.h"
#include "st_comm_layer.h"

typedef struct
{
  int8_t *object_label;
  /* CK_ULONG  object_class; */
  CK_BYTE object_class;
} object_desc_t;

/*
  * brief max number of token object
*/
#define OBJECT_DESC_NB (uint8_t)12

/**
  * @brief Retrieves the token objects from the config file.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV LoadTokenObjects()
{

  static const object_desc_t object_desc[OBJECT_DESC_NB] =
  {
    {FIRST_DERIVED_KEY,   4},
	{PMK_1,				  4},
	{PMK_2,				  4},
	{PMK_3,				  4},
	{PMK_4,				  4},
	{PMK_5,				  4},
	{PMK_6,				  4},
	{PMK_7,				  4},
	{ENC_DEC_KEY, 		  4},
	{MAIN_SECRET_KEY,     4},
	{DATA_OBJECT,         0},
    {MAIN_CERTIFICATE,    1}

  };

  CK_BBOOL secretKeysPresence[OBJECT_DESC_NB];
  (void)memset(secretKeysPresence, 0, OBJECT_DESC_NB);

  uint16_t certValueLen 	= 0;
  uint16_t dataObjectLen 	= 0;
  CK_BYTE dataObjectValue[MAX_DATAOBJECT_LEN];
  (void)memset(dataObjectValue, 0, MAX_DATAOBJECT_LEN);

  CK_BYTE keyResponseStatusLen	= 0;
  CK_BYTE keyResponseStatus[MAX_SECKEYRESPONSE_LEN];
  (void)memset(keyResponseStatus, 0, MAX_SECKEYRESPONSE_LEN);

  /*
    * get length of the certificate
  */
  CK_RV rv = getCertificateFileContentLength((CK_BYTE)0x02, &certValueLen); /* Certificate */
  if (rv == CKR_OK)
  {
	certValueLen -= (uint16_t)4;
	/*
	  * get length and value of the URL
	*/
	rv = getDataObjectLengthAndValue((CK_BYTE)0x01, &dataObjectLen, &dataObjectValue[0]);
	if (rv == CKR_OK)
	{
		/*
		  * check if the Secret Keys are present
		*/
		rv = retrieveSecretKeysPresence(&keyResponseStatus[0], &keyResponseStatusLen);
	}
  }

  if (rv == CKR_OK)
  {
    srand(generateRandom());
    for (uint8_t i = 0; i < OBJECT_DESC_NB; i++)
    {
      if (object_desc[i].object_class == CKO_CERTIFICATE)
      {
        /*
          * To decide xhandle value for Certificates it must be checked if length != 0
        */
//        if (strcmp((CCHAR *)object_desc[i].object_label, MAIN_CERTIFICATE) == 0)
//        {
          xP11Context.xObjectList.xObjects[i].xHandle = (uint32_t)((bool)(certValueLen == 0x0000U) ? 0 : rand());
          xP11Context.xObjectList.xObjects[i].xSign = CK_FALSE;

//        }
      }
      else if (object_desc[i].object_class == CKO_DATA)
      {
        xP11Context.xObjectList.xObjects[i].xHandle = (uint32_t)((bool)(dataObjectLen == 0x0000U) ? 0 : rand());
        xP11Context.xObjectList.xObjects[i].xSign = CK_FALSE;

      }
      else if (object_desc[i].object_class == CKO_SECRET_KEY)
      {
        xP11Context.xObjectList.xObjects[i].xHandle = (uint32_t)((bool)(keyResponseStatus[i] == 0x0000U) ? 0 : rand());
        xP11Context.xObjectList.xObjects[i].xSign = CK_TRUE;
        xP11Context.xObjectList.xObjects[i].xDerive = CK_TRUE;
      }
      else
      {
        xP11Context.xObjectList.xObjects[i].xHandle = (uint32_t) rand();
      }

      xP11Context.xObjectList.xObjects[i].xLabel = object_desc[i].object_label;
      xP11Context.xObjectList.xObjects[i].xClass = object_desc[i].object_class;

    }
  }

  return rv;
}

/**
  * @brief Finds, by Label, a P11 objects in the list of P11 objects.
  *
  * @param[in] objectLabel           Points to the Label.
  * @param[out] p11Obj                The location that contains the list of to the pkcs#11 objects
  *
  */
void findObjectInListByLabel(int8_t *objectLabel, P11Object_t *p11Obj)
{
  (*p11Obj).xHandle = (CK_ULONG)NULL;
  for (uint8_t i = 0; i < NUM_OBJECTS; i++)
  {
    if (xP11Context.xObjectList.xObjects[i].xLabel != NULL)
    {
      if (strcmp((CCHAR *)xP11Context.xObjectList.xObjects[i].xLabel, (CCHAR *)objectLabel) == 0)
      {
        *p11Obj = xP11Context.xObjectList.xObjects[i];
        break;
      }
    }
  }

}

/**
  * @brief Adds a P11 object to the list of P11 objects.
  *
  * @param[out] handle                Object Handle to add.
  * @param[in] objectToAdd           List of to the pkcs#11 objects
  *
  * @return CKR_OK if successful or error code.
  *
  */
//CK_RV addObjectToList(P11Object_t objectToAdd, CK_OBJECT_HANDLE *handle)
//{
//  CK_RV rv = CKR_OK;
//  /* verify that object is already present */
////  findObjectInListByLabel(objectToAdd.xLabel, &objectToAdd); /*Find object by label*/
////  if (objectToAdd.xHandle != (CK_ULONG)NULL)
////  {
////    if (objectToAdd.xToken == (uint8_t)CK_TRUE)
////    {
////      rv = CKR_ATTRIBUTE_VALUE_INVALID;
////    }
////    else
////    {
////      /* if the object handle is present, then set the handle to CK_INVALID_HANDLE */
////      rv = removeObjectFromList(&objectToAdd.xHandle); /*Delete object from Object List*/
////    }
////  }
////  if (rv == CKR_OK)
////  {
//    /*
//      * if it isn't there, add it in the first free slot
//    */
//    for (uint8_t i = 0; i < NUM_OBJECTS; i++)
//    {
//      if (strcmp(xP11Context.xObjectList.xObjects[i].xLabel,objectToAdd->xLabel) == 0)
//      {
////        xP11Context.xObjectList.xObjects[i].xLabel      = objectToAdd.xLabel;
////        xP11Context.xObjectList.xObjects[i].xClass      = objectToAdd.xClass;
////        xP11Context.xObjectList.xObjects[i].xToken      = objectToAdd.xToken;
////        xP11Context.xObjectList.xObjects[i].xDerive     = objectToAdd.xDerive;
////        xP11Context.xObjectList.xObjects[i].xSign       = objectToAdd.xSign;
////        xP11Context.xObjectList.xObjects[i].xVerify     = objectToAdd.xVerify;
////        xP11Context.xObjectList.xObjects[i].xValue      = objectToAdd.xValue;
////        xP11Context.xObjectList.xObjects[i].xValueLen   = objectToAdd.xValueLen;
//        xP11Context.xObjectList.xObjects[i].xHandle = (uint32_t)rand();
//        *handle = xP11Context.xObjectList.xObjects[i].xHandle;
//        rv = CKR_OK;
//        break;
//      }
//      else
//      {
//        rv = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//    }
// // }
//
//  return rv;
//}

/**
  * @brief Removes a P11 object to the list of P11 objects.
  *
  * @param[in] pHandle             Object Handle to add.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV removeObjectFromList(CK_OBJECT_HANDLE *pHandle)
{
  CK_RV rv = CKR_OK;
  for (uint8_t i = 0; i < NUM_OBJECTS; i++)
  {
    if (xP11Context.xObjectList.xObjects[i].xHandle == *pHandle)
    {
      /* xP11Context.xObjectList.xObjects[i].xLabel = NULL;*/
      xP11Context.xObjectList.xObjects[i].xHandle = CK_INVALID_HANDLE;
      rv = CKR_OK;
      break;
    }
    else
    {
      rv = CKR_OBJECT_HANDLE_INVALID;
    }
  }
  /*return error because the object isn't present. (Shouldn't happen)*/
  return rv;
}

/**
  * @brief Removes session objects from the list of objects.
  *
  * @return CKR_OK
  *
  */
CK_RV removeSessionObjectFromList()
{
  for (uint8_t i = 0; i < NUM_OBJECTS; i++)
  {
    if (xP11Context.xObjectList.xObjects[i].xToken == (uint8_t)CK_FALSE)
    {
      /* reset to default value of the pkcs#11 objects attributes */
      xP11Context.xObjectList.xObjects[i].xLabel  = NULL;
      xP11Context.xObjectList.xObjects[i].xClass  = 0xffffffffU;
      xP11Context.xObjectList.xObjects[i].xToken  = 0xff;
      xP11Context.xObjectList.xObjects[i].xDerive = 0;
      xP11Context.xObjectList.xObjects[i].xSign   = 0;
      xP11Context.xObjectList.xObjects[i].xHandle = 0;
    }
  }

  return CKR_OK;
}

/**
  * @brief Removes session Key Pairs from the list of objects.
  *
  */
void removeSessionKeyPairsFromList()
{
  for (uint8_t i = 0; i < NUM_OBJECTS; i++)
  {
    if ((xP11Context.xObjectList.xObjects[i].xToken == CK_FALSE) \
        && ((xP11Context.xObjectList.xObjects[i].xClass == (uint8_t)CKO_PRIVATE_KEY)\
            || (xP11Context.xObjectList.xObjects[i].xClass == (uint8_t)CKO_PUBLIC_KEY)))
    {
      /* reset to default value of the pkcs#11 objects attributes */
      xP11Context.xObjectList.xObjects[i].xLabel  = NULL;
      xP11Context.xObjectList.xObjects[i].xClass  = 0xffffffffU;
      xP11Context.xObjectList.xObjects[i].xToken  = 0xff;
      xP11Context.xObjectList.xObjects[i].xDerive = 0;
      xP11Context.xObjectList.xObjects[i].xSign   = 0;
      xP11Context.xObjectList.xObjects[i].xHandle = 0;
    }
  }
}

/**
  * @brief Finds, by handle, a P11 objects in the list of P11 objects.
  *
  * @param[in] xObject           Object handles.
  * @param[out] xHandle          The location that receives the list of object handles.
  * @param[out] p11Obj           The location that contains the list of to the pkcs#11 objects.
  *
  */

void findObjectInListByHandle(CK_OBJECT_HANDLE xObject, CK_OBJECT_HANDLE_PTR xHandle, P11Object_t *p11Obj)
{
  *xHandle = (CK_ULONG)NULL;
  for (uint8_t i = 0; i < NUM_OBJECTS; i++)
  {
    if (xP11Context.xObjectList.xObjects[i].xLabel != NULL)
    {
      if (xP11Context.xObjectList.xObjects[i].xHandle == xObject)/* find object by handle value*/
      {
        *xHandle = xP11Context.xObjectList.xObjects[i].xHandle;
        *p11Obj = xP11Context.xObjectList.xObjects[i];
        break;
      }
    }
  }

}

/**
  * @brief Update the object handle .
  *
  * @param[in] xLabel                Points to the Label.
  * @param[in] labelSize             Length of label.
  * @param[in] p11Obj                The location that contains the list of to the pkcs#11 objects.
  *
  */

void updateHandleForObject(int8_t *xLabel, CK_ULONG labelSize, P11Object_t *p11Obj)
{
  p11Obj->xHandle = 0;
  for (uint8_t i = 0; i < NUM_OBJECTS; i++)
  {
    if (memcmp(xP11Context.xObjectList.xObjects[i].xLabel, xLabel, labelSize) == 0)
    {
      xP11Context.xObjectList.xObjects[i].xHandle = (uint32_t)rand(); /*assign a random number to handle*/
      *p11Obj = xP11Context.xObjectList.xObjects[i];
      break;
    }
  }

}

/**
  * @brief Converts an hex string into a byte array.
  *
  * @param[in] hexstr           Points to hex string.
  * @param[out] charstr         Points to byte array.
  * @param[in] len              Length of hex string
  *
  */
void hexstr_to_char(uint8_t *hexstr, uint8_t *charstr, uint32_t len)
{
  uint32_t final_len = len / (uint32_t)2;
  uint32_t i = 0;
  uint32_t j;

  for (j = 0; j < final_len; j++)
  {
    charstr[j] = ((((hexstr[i] % (uint8_t)32) + (uint8_t)9) % (uint8_t)25) * (uint8_t)16) + \
                 (((hexstr[i + (uint8_t)1] % (uint8_t)32) + (uint8_t)9) % (uint8_t)25);
    i += (uint32_t)2;
  }
  charstr[final_len] = (uint8_t)'\0';

}

/**
  * @brief Gets data from a TLV structure.
  *
  * @param[in] wTag             Label that is used for search in the TLV structure.
  * @param[in] pTlv             Pointer to TLV structure.
  * @param[in] ulTlvSize        Size of TLV structure.
  * @param[out] pData           Pointer to buffer that contains the data extracted from structure.
  * @param[out] pulData         Size of data extracted from structure.
  *
  * @return TRUE if successful or false in case of any error.
  *
  */
bool TLV_Get(uint16_t wTag, const CK_BYTE *pTlv, CK_ULONG ulTlvSize, const CK_BYTE **pData, CK_ULONG *pulData)
{
  *pData = NULL;
  *pulData = 0;
  uint16_t dataLen;
  uint16_t dataLenLen;
  CK_BYTE len;
  CK_ULONG index1 = 0;
  bool ret = false;
  while (index1 < ulTlvSize)
  {
    if (ulTlvSize > (uint32_t)255)
    {

      len = pTlv[index1 + (uint32_t)1];
      if ((int16_t)len == 0x82)
      {
        dataLen = ((uint16_t)pTlv[index1 + (uint16_t)2] << 8) | ((uint16_t)pTlv[index1 + (uint16_t)3]);
        dataLenLen = 3;
      }
      else if ((int16_t)len == 0x81)
      {
        dataLen = pTlv[index1 + (uint32_t)2];
        dataLenLen = 2;
      }
      else
      {
        dataLen = len;
        dataLenLen = 1;
      }

      if (pTlv[index1] == wTag)
      {
        if ((index1 + dataLenLen + dataLen) > ulTlvSize)
        {
          ret = false; /* invalid TLV struct... */
          index1 = ulTlvSize;
          continue;
        }

        *pulData = dataLen;
        *pData = pTlv;
        *pData += (uint32_t)1 + index1 + dataLenLen;
        ret = true;
        index1 = ulTlvSize;
        continue;
      }
      else
      {
        if ((index1 + dataLenLen + dataLen) > ulTlvSize)
        {
          ret = false;
          index1 = ulTlvSize;
          continue;
        }
        else
        {
          index1 += (CK_ULONG) 0x0001U + dataLenLen + dataLen; /* jump L and V */
        }
      }
    }
    else
    {
      if (pTlv[index1] == wTag)
      {
        if ((index1 + (uint32_t)1 + pTlv[index1 + (uint32_t)1]) > ulTlvSize)
        {
          ret = false; /* invalid TLV struct... */
          index1 = ulTlvSize;
          continue;
        }

        *pulData = pTlv[index1 + (uint32_t)1];
        *pData = pTlv;
        *pData += (index1 + (uint32_t)2);
        ret = true;
        index1 = ulTlvSize;
        continue;
      }
      else
      {
        if ((index1 + (uint32_t)1 + pTlv[index1 + (uint32_t)1]) > ulTlvSize)
        {
          ret = false;
          index1 = ulTlvSize;
          continue;
        }
        else
        {
          index1 += (CK_ULONG)2U + pTlv[index1 + (uint32_t)1]; /* jump L and V */
        }
      }
    }
  }

  return ret;
}

/**
  * @brief Validates a TLV (Tag Len Value) structure
  *
  * @param[in] pTlv             Pointer to TLV structure.
  * @param[in] ulTlvSize        Size of TLV structure.
  *
  * @return TRUE if successful or false in case of any error.
  *
  */
bool TLV_Validate(const CK_BYTE *pTlv, CK_ULONG ulTlvSize)
{
  uint16_t dataLen;
  uint16_t dataLenLen;
  CK_BYTE len;
  bool ret = true;;
  if ((pTlv == NULL) || (ulTlvSize < (uint32_t)2))
  {
    ret = false;
  }

  uint16_t index1 = 0;

  /*
    * 87 57 value
  */
  /*
  * T L V if L<127 (X)
  */
  /*
  * T 81 L V if L>=128 and L<256
  */
  /*
  * T 82 LL LL if L >= 256  (X)
  */
  while ((index1 < ulTlvSize) && ret)
  {
    if (ulTlvSize > (uint32_t) 255)
    {
      len = pTlv[index1 + (uint32_t)1];
      if ((int16_t)len == 0x82)
      {
        dataLen = ((uint16_t)pTlv[index1 + (uint16_t)2] << 8) | (uint16_t)pTlv[index1 + (uint16_t)3];
        dataLenLen = 3;
      }
      else if ((uint16_t)len == 0x81U)
      {
        dataLen = pTlv[index1 + (uint32_t)2];
        dataLenLen = 2;
      }
      else
      {
        dataLen = len;
        dataLenLen = 1;
      }

      if ((index1 + dataLenLen + dataLen) > (uint16_t)ulTlvSize)
      {
        ret = false; /* TLV invalid */
        continue;
      }
      index1 += 0x0001U + dataLenLen + dataLen; /* jump L and V */
    }
    else
    {
      if ((index1 + (uint32_t)1 + pTlv[index1 + (uint32_t)1]) > ulTlvSize)
      {
        ret = false; /* TLV invalid */
        continue;
      }
      index1 += (uint16_t)2U + pTlv[index1 + (uint32_t)1];  /* jump L and V */
    }
  }

  return ret;
}

/**
  * @brief Returns the value of a public key.
  *
  * @param[in] pxSessionObj            Points to PKCS#11 session structure.
  * @param[out] publicKeyValue         Pointer to buffer that contains the public Key.
  * @param[out] publicKeyValueLen      Length of publicKeyValue buffer
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV getPublicKeyValue(P11SessionPtr_t pxSessionObj, CK_BYTE_PTR publicKeyValue, CK_ULONG_PTR publicKeyValueLen)
{
  uint16_t pTargetDF[10];
  CK_RV lRes;                   /*lRes*/
  CK_ULONG ulPath = 0;          /*ulPath*/
  CK_BYTE pTmpBuf[70];          /*pTmpBuf*/
  (void)memset(pTmpBuf, 0, 70);
  CK_BYTE cbValueLen = PUBLIC_KEY_VALUE_LEN;  /*cbValueLen*/

  if (pxSessionObj->targetKeyParameter == 0x00U)
  {
    lRes = CKR_FUNCTION_FAILED;            /*CKR_FUNCTION_FAILED*/
  }
  else
  {
    pTargetDF[ulPath] = 0x8000U | (uint16_t)pxSessionObj->targetKeyParameter;
    ulPath++;
    lRes = PathSelectWORD(pTargetDF, ulPath); /* Select file that contains the  public key */
    if (lRes == CKR_OK)
    {
      lRes = ReadRecord(0x04, 0x01, pTmpBuf, &cbValueLen);
      if (lRes == CKR_OK)
      {
        if (pTmpBuf[1] != 0x41U) /* tag 41 means that is a public key */
        {
          lRes = CKR_GENERAL_ERROR;
        }
        else
        {
          *publicKeyValueLen = (uint32_t)cbValueLen - 2U;
          (void)memcpy(publicKeyValue, &pTmpBuf[2], cbValueLen); /*Copy public key value in publicKeyValue*/
        }
      }
    }
  }

  return lRes;
}
/**
  * @brief Returns the Lenght of a certificate.
  *
  * @param[in] targetFile            ID of file that contains the certifciate.
  * @param[out] fileContentLen        Length of certificate.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV getCertificateFileContentLength(CK_BYTE targetFile, uint16_t *fileContentLen)
{
  uint16_t pTargetDF[] = { 0x2F02 };
  CK_RV lRes;
  CK_ULONG ulPath = 0;
  CK_BYTE pTmpBuf[6];
  (void)memset(pTmpBuf, 0, 6);

//  pTargetDF[ulPath] = 0x2F00U | targetFile;
//  ulPath++;
  lRes = PathSelectWORD(pTargetDF, 1);//ulPath); /* Select file that contains the certificate*/
  if (lRes == CKR_OK)
  {
    /*
      * read first 4 bytes to get Certificate Length
    */
    lRes = ReadBinary(0, 4, pTmpBuf);
    if (lRes == CKR_OK)
    {
      /*
        * Byte 1 and 2 indicate that is a certificate byte 1 = 0x30 e byte 2 = 0x82
        * Byte 3 and 4 indicate Certificate Length
        * Global Length is Certificate Length + 4
        * Byte 3 and 4 indicate Certificate Length
      */
      *fileContentLen = ((uint16_t)pTmpBuf[2] * (uint16_t)256) + (uint16_t)pTmpBuf[3] + (uint16_t)4;
    }
  }

  return lRes;
}

CK_RV getDataObjectLengthAndValue(CK_BYTE targetFile, uint16_t *dataObjectLen, CK_BYTE_PTR pDataObjectValue)
{
	uint16_t length	= 0;
	uint16_t pTargetDF[] = { 0x2F01 };
	CK_RV lRes;
	CK_ULONG ulPath = 0;
	CK_BYTE pTmpBuf[MAX_DATAOBJECT_LEN]; // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 150
//	pTargetDF[ulPath] = 0x2F00U | targetFile;
//	ulPath++;
	lRes = PathSelectWORD(pTargetDF, 1);//ulPath); /* Select file that contains the Data Object*/
	if (lRes == CKR_OK)
	{
		(void)memset(pTmpBuf, '\0', MAX_DATAOBJECT_LEN);
		lRes = ReadBinary(0, MAX_DATAOBJECT_LEN, pTmpBuf);
		if (lRes == CKR_OK)
		{
			for (int i = 0; pTmpBuf[i] != '\0'; i++)
			{
				length++;
			}

			*dataObjectLen = length;
            (void)memcpy(pDataObjectValue, pTmpBuf, *dataObjectLen);
		}
	}

	return lRes;
}


/**
  * @brief Returns the content of a certificate.
  *
  * @param[in] pxSessionObj              Points to PKCS#11 session structure.
  * @param[out] certValue                Pointer to buffer that contains the certificate.
  * @param[out] certValueLen             Length of certificate.
  *
  * @return CKR_OK if successful or error code.
  *
  */

CK_RV getCertificateFileContent(P11SessionPtr_t pxSessionObj, CK_BYTE_PTR certValue, uint16_t *certValueLen)
{
  uint16_t pTargetDF[] = { 0x2F02 };
  CK_RV lRes ;
  CK_ULONG ulPath = 0;
  CK_BYTE pTmpBuf[MAX_CERTIFICATE_LEN];
//  pTargetDF[ulPath] = 0x2F00U | 0x02;
//  ulPath++;
  lRes = PathSelectWORD(pTargetDF, 1);//ulPath); /* Select file that contains the Certificate */
  if (lRes == CKR_OK)
  {
	  /*
	   * First read 4 bytes to get Certificate Length
	   */
	  (void)memset(pTmpBuf, 0x00, MAX_CERTIFICATE_LEN);
	  lRes = ReadBinary(0, 4, pTmpBuf);
	  if (lRes == CKR_OK)
	  {
		  /*
          Byte 1 and 2 indicate that is a certificate byte 1 = 0x30 e byte 2 = 0x82
		   */
		  if ((pTmpBuf[0] == 0x30U) || ((int16_t)pTmpBuf[1] == 0x82))
		  {
			  /*
			   * Byte 3 and 4 indicate Certificate Length
			   */
			  *certValueLen = ((uint16_t)pTmpBuf[2] * 0x0100U) + (uint16_t)pTmpBuf[3];
			  if (*certValueLen <= MAX_CERTIFICATE_LEN)
			  {
				  (void)memcpy(certValue, pTmpBuf, 4); /* first 4 bytes */
				  lRes = ReadBinary(4, *certValueLen, pTmpBuf); /* Read the file from position 4 */
				  if (lRes == CKR_OK)
				  {
					  CK_BYTE_PTR pcertValue = &certValue[4];
					  (void)memcpy(pcertValue, pTmpBuf, *certValueLen); /* further bytes */
					  *certValueLen += (uint16_t)4; /* set total length */
				  }
			  }
			  else
			  {
				  lRes = CKR_GENERAL_ERROR;
			  }
		  }
		  else
		  {
			  lRes = CKR_GENERAL_ERROR;
		  }
	  }

  }

  return lRes;
}
/**
  * @brief Checks that the private key template and  public key template provided for C_GenerateKeyPair
  *        contains all necessary attributes, and does not contain any invalid attributes.
  *
  * @param[in] pPrivateTemplate                 Pointer to a list of attributes that the generated
  *                                             private key should possess.
  * @param[in] ulPrivateTemplateLength          Number of attributes in pPrivateTemplate.
  * @param[in] pPublicTemplate                  Pointer to a list of attributes that the generated
  *                                             public key should possess.
  * @param[in] ulPublicTemplateLength           Number of attributes in pPublicTemplate.
  * @param[in] pPrivateLabel                    Label of private key.
  * @param[in] pPublicLabel                     Label of public key.
  * @param[out] isToken                         TRUE is token object, FALSE is ephemeral object.
  *
  * @return CKR_OK if successful or error code.
  *
  */

CK_RV checkGenerateKeyPairPrivatePublicTemplates(CK_ATTRIBUTE_PTR pPrivateTemplate, CK_ULONG ulPrivateTemplateLength,
                                                 CK_ATTRIBUTE_PTR pPublicTemplate, CK_ULONG ulPublicTemplateLength,
                                                 CK_ATTRIBUTE_PTR pPrivateLabel, CK_ATTRIBUTE_PTR pPublicLabel,
                                                 bool *isToken)
{
  CK_KEY_TYPE xKeyType;
  CK_BYTE xEcParams[10] = { 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 };
  uint16_t lCompare;

  static CK_ATTRIBUTE defaultPrivateLabelAttribute[] =          /* Template private key */
  {
    {CKA_LABEL, defaultPrivateLabel, sizeof(defaultPrivateLabel) - 1}
  };
  static CK_ATTRIBUTE defaultPublicLabelAttribute[]  =          /* Template ublic key */
  {
    {CKA_LABEL, defaultPublicLabel, sizeof(defaultPublicLabel) - 1}
  };

  CK_ATTRIBUTE xAttribute;
  CK_RV xResult = CKR_OK;
  CK_BBOOL xBool;
  CK_ULONG xTemp;
  int32_t xIndex;

  /*
    * Default for Private Keys is TOKEN
    */
  bool privateIsToken = true;

  /*
    * Default for Public Keys is TOKEN
    */
  bool publicIsToken  = true;

  int32_t privateTemplateLabelIndex      = -1;
  int32_t privateTemplateClassIndex      = -1;
  int32_t privateTemplateSignIndex       = -1;
  int32_t privateTemplateTokenIndex      = -1;
  int32_t privateTemplateDeriveIndex     = -1;
  int32_t privateTemplateKeyTypeIndex    = -1;
  int32_t privateTemplatePrivateIndex    = -1;
  int32_t privateTemplateSensitiveIndex  = -1;
  int32_t privateTemplateECParamsIndex   = -1;

  int32_t publicTemplateLabelIndex       = -1;
  int32_t publicTemplateClassIndex       = -1;
  int32_t publicTemplateKeyTypeIndex     = -1;
  int32_t publicTemplateEcParamsIndex    = -1;
  int32_t publicTemplateVerifyIndex      = -1;
  int32_t publicTemplateTokenIndex       = -1;
  int32_t publicTemplatePrivateIndex     = -1;

  /*
    * Every Attribute must be present in the template only once
    */
  for (xIndex = 0; (xIndex < (int32_t)ulPrivateTemplateLength) && (xResult == CKR_OK); xIndex++)
  {
    xAttribute = pPrivateTemplate[xIndex];

    switch (xAttribute.type)
    {
      case CKA_LABEL:
        if (privateTemplateLabelIndex == -1)
        {
          privateTemplateLabelIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(pPrivateLabel, pPrivateTemplate + xIndex, sizeof(CK_ATTRIBUTE));
        break;

      case CKA_CLASS:
        if (privateTemplateClassIndex == -1)
        {
          privateTemplateClassIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&xTemp, xAttribute.pValue, sizeof(CK_ULONG));

        if (xTemp != CKO_PRIVATE_KEY)
        {
          xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }
        break;

      case CKA_TOKEN:
        if (privateTemplateTokenIndex == -1)
        {
          privateTemplateTokenIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&privateIsToken, xAttribute.pValue, sizeof(CK_BBOOL));

        break;

      case CKA_KEY_TYPE:
        if (privateTemplateKeyTypeIndex == -1)
        {
          privateTemplateKeyTypeIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&xTemp, xAttribute.pValue, sizeof(CK_ULONG));

        if (xTemp != CKK_EC)
        {
          xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }

        break;

      case CKA_PRIVATE:
        if (privateTemplatePrivateIndex == -1)
        {
          privateTemplatePrivateIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&xBool, xAttribute.pValue, sizeof(CK_BBOOL));

        if (xBool != CK_FALSE)
        {
          xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }

        break;

      case CKA_SIGN:
        if (privateTemplateSignIndex == -1)
        {
          privateTemplateSignIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&xBool, xAttribute.pValue, sizeof(CK_BBOOL));

        break;

      case CKA_SENSITIVE:
        if (privateTemplateSensitiveIndex == -1)
        {
          privateTemplateSensitiveIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&xBool, xAttribute.pValue, sizeof(CK_BBOOL));

        if (xBool != CK_TRUE)
        {
          xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }

        break;

      case CKA_DERIVE:
        if (privateTemplateDeriveIndex == -1)
        {
          privateTemplateDeriveIndex = xIndex;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        (void)memcpy(&xBool, xAttribute.pValue, sizeof(CK_BBOOL));

        break;

      case CKA_EC_PARAMS:
        if (privateTemplateECParamsIndex == -1)
        {
          privateTemplateECParamsIndex = 1;
        }
        else
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
          break;
        }

        lCompare = (uint16_t)memcmp(xEcParams, xAttribute.pValue, sizeof(xEcParams));

        if (lCompare != 0x0000U)
        {
          xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }

        break;

      default:
        xResult = CKR_OK;
        break;
    }
  }

  /* check Public Template */
  if (xResult == CKR_OK)
  {
    /*
      * Every Attribute must be present in the template only once
      */
    /*************************************************************/
    for (xIndex = 0; (xIndex < (int32_t)ulPublicTemplateLength) && (xResult == CKR_OK); xIndex++)
    {
      xAttribute = pPublicTemplate[xIndex];

      switch (xAttribute.type)
      {
        case CKA_LABEL:
          if (publicTemplateLabelIndex == -1)
          {
            publicTemplateLabelIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          (void)memcpy(pPublicLabel, pPublicTemplate + xIndex, sizeof(CK_ATTRIBUTE));

          break;

        case CKA_CLASS:
          if (publicTemplateClassIndex == -1)
          {
            publicTemplateClassIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          (void)memcpy(&xTemp, xAttribute.pValue, sizeof(CK_ULONG));

          if (xTemp != CKO_PUBLIC_KEY)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }
          break;

        case CKA_KEY_TYPE:
          if (publicTemplateKeyTypeIndex == -1)
          {
            publicTemplateKeyTypeIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          (void)memcpy(&xKeyType, xAttribute.pValue, sizeof(CK_KEY_TYPE));

          if (xKeyType != CKK_EC)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }

          break;

        case CKA_EC_PARAMS:
          if (publicTemplateEcParamsIndex == -1)
          {
            publicTemplateEcParamsIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          lCompare = (uint16_t) memcmp(xEcParams, xAttribute.pValue, sizeof(xEcParams));

          if (lCompare != 0x0000U)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }

          break;

        case CKA_VERIFY:
          if (publicTemplateVerifyIndex == -1)
          {
            publicTemplateVerifyIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          (void)memcpy(&xBool, xAttribute.pValue, sizeof(CK_BBOOL));

          if (xBool != CK_TRUE)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }

          break;

        case CKA_TOKEN:
          if (publicTemplateTokenIndex == -1)
          {
            publicTemplateTokenIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          (void)memcpy(&publicIsToken, xAttribute.pValue, sizeof(CK_BBOOL));

          break;

        case CKA_PRIVATE:
          if (publicTemplatePrivateIndex == -1)
          {
            publicTemplatePrivateIndex = xIndex;
          }
          else
          {
            xResult = CKR_TEMPLATE_INCONSISTENT;
            break;
          }

          (void)memcpy(&xBool, xAttribute.pValue, sizeof(CK_BBOOL));

          if (xBool != CK_FALSE)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }

          break;

        default:
          xResult = CKR_OK;
          break;
      }
    }
  }

  /************************ CHECKS ********************************/
  /*
    * Must be both TOKEN or both EPHEMERAL
    */
  if (xResult == CKR_OK)
  {
    if (privateIsToken != publicIsToken)
    {
      xResult = CKR_TEMPLATE_INCONSISTENT;
    }
  }


  if (xResult == CKR_OK)
  {
    *isToken = privateIsToken;
    if (privateIsToken)
    {
      /* TOKEN Objects */
      /* Labels are imposed */
      /* handle the case of empty or null string ? */
      /* If CKA_DERIVE is present, it must be FALSE */
      if (privateTemplateDeriveIndex != -1)
      {
        (void)memcpy(&xBool, pPrivateTemplate[privateTemplateDeriveIndex].pValue, sizeof(CK_BBOOL));
        if (xBool != FALSE)
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
        }
      }

      if (xResult == CKR_OK)
      {
        if (strcmp(pPrivateTemplate[privateTemplateLabelIndex].pValue, MAIN_PRIVATE_KEY_IN_KEY_PAIR) == 0)
        {
          if (strcmp(pPublicTemplate[publicTemplateLabelIndex].pValue, MAIN_PUBLIC_KEY_IN_KEY_PAIR) != 0)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }
        }
        else if (strcmp(pPrivateTemplate[privateTemplateLabelIndex].pValue, SECONDARY_PRIVATE_KEY_IN_KEY_PAIR) == 0)
        {
          if (strcmp(pPublicTemplate[publicTemplateLabelIndex].pValue, SECONDARY_PUBLIC_KEY_IN_KEY_PAIR) != 0)
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }
        }
        else
        {
          xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }
      }
    }
    else
    {
      /* EPHEMERALS Objects */
      /* CKA_DERIVE presence is Mandatory */
      /* Must be TRUE */
      if (privateTemplateDeriveIndex == -1)
      {
        xResult = CKR_TEMPLATE_INCOMPLETE;
      }
      else
      {
        (void)memcpy(&xBool, pPrivateTemplate[privateTemplateDeriveIndex].pValue, sizeof(CK_BBOOL));
        if (xBool != CK_TRUE)
        {
          xResult = CKR_TEMPLATE_INCONSISTENT;
        }
        else
        {
          /*
            *   If CKA_SIGN is present, it must be FALSE
            */
          if (privateTemplateSignIndex != -1)
          {
            (void)memcpy(&xBool, pPrivateTemplate[privateTemplateSignIndex].pValue, sizeof(CK_BBOOL));
            if (xBool != CK_FALSE)
            {
              xResult = CKR_TEMPLATE_INCONSISTENT;
            }
          }
        }
      }

      if (xResult == CKR_OK)
      {
        if ((privateTemplateLabelIndex == -1) && (publicTemplateLabelIndex == -1))
        {
          /* No Labels: Default values*/
          (void)memcpy(pPrivateLabel, &defaultPrivateLabelAttribute[0], sizeof(CK_ATTRIBUTE));
          (void)memcpy(pPublicLabel, &defaultPublicLabelAttribute[0], sizeof(CK_ATTRIBUTE));
        }
        else if ((privateTemplateLabelIndex >= 0) && (publicTemplateLabelIndex >= 0))
        {
          /*
            * Both Labels are present in Templates
            */

          /*
            * Check that Length of Labels is allowed
            */
          if ((pPrivateTemplate[privateTemplateLabelIndex].ulValueLen > (uint32_t)MAX_LABEL_LENGTH)
              || (pPublicTemplate[publicTemplateLabelIndex].ulValueLen > (uint32_t) MAX_LABEL_LENGTH))
          {
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
          }

          /*
            * Check that Labels are different for Private and Public
            */
          if (xResult == CKR_OK)
          {
            if (strcmp(pPrivateTemplate[privateTemplateLabelIndex].pValue,
                       pPublicTemplate[publicTemplateLabelIndex].pValue)
                == 0)
            {
              xResult = CKR_TEMPLATE_INCONSISTENT;
            }
          }

          /*
            * Check that Objects with given Labels are not already existing
            */
          if (xResult == CKR_OK)
          {
            P11Object_t p11Obj;

            findObjectInListByLabel(pPrivateTemplate[privateTemplateLabelIndex].pValue, &p11Obj);
            if (p11Obj.xHandle != (CK_ULONG)NULL)
            {
              xResult = CKR_ATTRIBUTE_VALUE_INVALID;
            }

            if (xResult == CKR_OK)
            {
              findObjectInListByLabel(pPublicTemplate[publicTemplateLabelIndex].pValue, &p11Obj);
              if (p11Obj.xHandle != (CK_ULONG)NULL)
              {
                xResult = CKR_ATTRIBUTE_VALUE_INVALID;
              }
            }
          }
        }
        else
        {
          /*
            * Only one Label: Error
            */
          xResult = CKR_TEMPLATE_INCONSISTENT;
        }
      }
    }
  }
  return xResult;
}
/**
  * @brief Return the position of the attribute inside the template
  *
  * @param[in] attributeType            Type of attribute to search inside pTemplate.
  * @param[in] pTemplate                Pointer to a list of attributes of the pkcs#11 object.
  * @param[in] ulAttributeCount         Number of attributes in pTemplate.
  *
  * @return position of attribute.
  *
  */
int32_t GetAttributePos(CK_ATTRIBUTE_TYPE attributeType, const CK_ATTRIBUTE *pTemplate, CK_ULONG ulAttributeCount)
{
  CK_ULONG i;
  int16_t ret = -1;
  if (pTemplate != NULL)
  {
    for (i = 0; i < ulAttributeCount; i++)
    {
      if (attributeType == pTemplate[i].type)
      {
        ret = (int16_t)i;
        break;
      }
    }
  }

  return ret;
}

/**
  * @brief Return tha value of attribute inside the template.
  *
  * @param[in] attributeType            Type of attribute to search inside pTemplate.
  * @param[in] pTemplate                Pointer to a list of attributes of the pkcs#11 object.
  * @param[in] ulAttributeCount         Number of attributes in pTemplate.
  * @param[out] pulAttributeSize        Size of attribute.
  * @param[out] pulPosition             Position of attribute.
  *
  * @return value of attribute.
  *
  */
CK_BYTE *GetAttribute(CK_ATTRIBUTE_TYPE attributeType, const CK_ATTRIBUTE *pTemplate, CK_ULONG ulAttributeCount,
                      CK_ULONG *pulAttributeSize, CK_ULONG *pulPosition)
{
  CK_BYTE *ret = NULL;
  if (pTemplate != NULL)
  {
    for (uint8_t i = 0; i < ulAttributeCount; i++)
    {
      if (attributeType == pTemplate[i].type)
      {
        if (pulAttributeSize != NULL)
        {
          *pulAttributeSize = pTemplate[i].ulValueLen;
        }
        if (pulPosition != NULL)
        {
          *pulPosition = i;
        }
        ret = (CK_BYTE *) pTemplate[i].pValue;
        break;
      }
    }
  }
  return ret;
}

/**
  * @brief Delete the value of certificate from module
  *
  * @param[in] pxSessionObj             Pointer to PKCS#11 session structure.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV DeleteCertificateValue(P11SessionPtr_t pxSessionObj)
{
  uint16_t pTargetDF[10];
  CK_RV lRes;
  CK_ULONG ulPath = 0;
  CK_BYTE pTmpBuf[4] = {0x30, 0x82, 0x00, 0x00};
  if (pxSessionObj->targetCertificateParameter == 0x00U)
  {
    lRes = CKR_FUNCTION_FAILED;
  }
  else
  {
    pTargetDF[ulPath] = 0x9000U | pxSessionObj->targetCertificateParameter;
    ulPath++;
    lRes = PathSelectWORD(pTargetDF, ulPath); /* Select file that contains the Certificate */
    if (lRes == CKR_OK)
    {
      lRes = UpdateBinary(pTmpBuf, 4, 0); /* Clean up byte 3 and byte 4  */
    }
  }
  return lRes;
}

/**
  * @brief Write the value of certificate on module
  *
  * @param[in] pxSessionObj             Pointer to PKCS#11 session structure.
  * @param[in] certValue                Pointer to buffer that contains the value of certificate.
  * @param[in] certValueLen             Length of certificate.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV WriteCertificateValue(P11SessionPtr_t pxSessionObj, CK_BYTE_PTR certValue, CK_ULONG certValueLen)
{
  uint16_t pTargetDF[10];
  CK_RV lRes;
  CK_ULONG ulPath = 0;

  if (pxSessionObj->targetCertificateParameter == 0x00U)
  {
    lRes = CKR_FUNCTION_FAILED;
  }
  else
  {
    pTargetDF[ulPath] = 0x9000U | pxSessionObj->targetCertificateParameter;
    ulPath++;
    lRes = PathSelectWORD(pTargetDF, ulPath); /* Select file that contains the Certificate */
    if (lRes == CKR_OK)
    {
      lRes = (CK_RV)UpdateBinary(certValue, (uint16_t) certValueLen, 0); /*Update the value of Certficate*/
    }
  }

  return lRes;
}

CK_RV UpdateFileValue(P11SessionPtr_t pxSessionObj, CK_BYTE targetFile, CK_BYTE_PTR fileValue, CK_ULONG fileValueLen)
{
	uint16_t pTargetDF[10];
	CK_RV lRes;
	CK_ULONG ulPath = 0;

    pTargetDF[ulPath++] = 0x2F00U | targetFile;
    lRes = PathSelectWORD(pTargetDF, ulPath); /* Select file that contains the CERTIFICATE/DATA_OBJECT */
    if (lRes == CKR_OK)
    {
      lRes = (CK_RV)UpdateBinary(fileValue, (uint16_t) fileValueLen, 0); /*Update the value of File */
    }

	return lRes;
}

/**
  * @brief Read the public key information from mechanism parameters.
  *
  * @param[in] pParameter         Mecahnism paramweter.
  * @param[out] params            Pointer to struct that contains the value of public key.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV ParseECParam(CK_VOID_PTR pParameter, CK_ECDH1_DERIVE_PARAMS *params)
{
  CK_ULONG *pParam = (CK_ULONG *)pParameter;
  (void)memset(params, 0, sizeof(params));
  CK_RV lRes = CKR_OK;
  params->kdf = (CK_EC_KDF_TYPE)(*pParam);
  if (params->kdf != CKD_NULL)
  {
    lRes = CKR_MECHANISM_PARAM_INVALID;
  }
  else
  {
    pParam++;
    params->ulSharedDataLen = *pParam;
    if (params->ulSharedDataLen == 0U)
    {
      pParam++;
      params->pSharedData = (CK_BYTE *)pParam;
      if (*params->pSharedData == (CK_ULONG)NULL)
      {
        pParam++;
        params->ulPublicDataLen = *pParam;
        pParam++;
        if (params->ulPublicDataLen == (uint32_t)PUBLIC_KEY_LEN_NO_TAGLEN)
        {
          (void)memcpy(&params->pPublicData, pParam, 4);
        }
        else
        {
          lRes = CKR_FUNCTION_FAILED;
        }
      }
      else
      {
        lRes = CKR_MECHANISM_PARAM_INVALID;
      }
    }
    else
    {
      lRes = CKR_MECHANISM_PARAM_INVALID;
    }
  }

  return lRes;
}

/**
  * @brief Read the key information from mechanism parameters.
  *
  * @param[in] pParameter         Mecahnism paramweter.
  * @param[out] params            Pointer to struct that contains the settings of the key.
  *
  * @return CKR_OK if successful or error code.
  *
  */
CK_RV ParseHKDFParam(CK_VOID_PTR pParameter, CK_HKDF_PARAMS *params)
{
//	typedef struct CK_HKDF_PARAMS {
//	   CK_BBOOL bExtract;
//	   CK_BBOOL bExpand;
//	   CK_MECHANISM_TYPE prfHashMechanism;
//	   CK_ULONG ulSaltType;
//	   CK_BYTE_PTR pSalt;
//	   CK_ULONG ulSaltLen;
//	   CK_OBJECT_HANDLE hSaltKey;
//	   CK_BYTE_PTR pInfo;
//	   CK_ULONG ulInfoLen;
//	} CK_HKDF_PARAMS;

	CK_BBOOL *pByteParam = (CK_BBOOL *)pParameter;
	(void)memset(params, 0, sizeof(params));
	CK_RV lRes = CKR_OK;

	params->bExtract 			= (CK_BBOOL)(*pByteParam); /* pByteParam points to Extract */

	pByteParam++; /* pByteParam points to Expand */
	params->bExpand 			= (CK_BBOOL)(*pByteParam);
	pByteParam++;
	pByteParam++;
	pByteParam++;
	CK_ULONG *pLongParam 		= (CK_ULONG *)pByteParam;

	//pLongParam++; /* pLongParam points to prfHashMechanism */
	params->prfHashMechanism 	= (CK_MECHANISM_TYPE)(*pLongParam);

	pLongParam++; /* pLongParam points to ulSaltType */
	params->ulSaltType 			= *pLongParam;

	pLongParam++; /* pLongParam points to pSalt */
	params->pSalt 				= (CK_BYTE*)pLongParam;

	pLongParam++; /* pLongParam points to ulSaltLen */
	params->ulSaltLen 			= *pLongParam;
	if (params->ulSaltLen == 32) // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 32
	{
		/* need to go back to point to pSalt */
		(void)memcpy(&params->pSalt, pLongParam-1, params->ulSaltLen);
	}
	else
	{
		lRes = CKR_MECHANISM_PARAM_INVALID;
	}

	if (lRes == CKR_OK)
	{
		pLongParam++; /* pLongParam points to hSaltKey */
		params->hSaltKey 			= (CK_OBJECT_HANDLE)(*pLongParam);

		pLongParam++; /* pLongParam points to pInfo */
		params->pInfo 				= (CK_BYTE*)pLongParam;

		pLongParam++; /* pLongParam points to InfoLen */
		params->ulInfoLen 			= *pLongParam;

		(void)memcpy(&params->pInfo, pLongParam-1, params->ulInfoLen); /* need to go back to point to pInfo */
	}

	return lRes;
}

/**
  * @brief Checks if the response buffer pointed by pbuf_rsp ends with "9000".
  *
  * @param[in] pbuf_rsp           Pointer to buffer taht contains the response.
  * @param[in] ret                Len of response.

  *
  * @return TRUE if successful or false in case of any error
  *
  */
bool is9000(com_char_t *pbuf_rsp, int32_t ret)
{
  bool retValue = false;
  if (((pbuf_rsp[ret - 1]) == 0x30U) && ((pbuf_rsp[ret - 2]) == 0x30U) && ((pbuf_rsp[ret - 3]) == 0x30U)
      && ((pbuf_rsp[ret - 4]) == 0x39U))
  {
    retValue = true; /*Return true if sw is 9000*/
  }
  return retValue;
}
/*
  * @
  */

/**
  * @brief Brief Retriev the following attributes:
  *   - SUBJECT
  *    - ISSUER
  *    - SERIAL NUMBER
  * from certificate value. This function use  mbedtls API.
  *
  * @param[in] attrType           Type of attribute.
  * @param[in] pemBuf             Pointer to buffer taht contains the value of certificate in pem format.
  * @param[in] pemBuflen          Legnth of pemBuf
  * @param[in] bufSize            Legnth of outbuf - 1
  * @param[out] outbuf            Pointer to buffer that contains the value of attribute.
  * @param[out] plenAttr          Sive of attribute.
  *
  * @return return a value > or = 0 if successful or -1 in case of any error.
  *
  */
int32_t getX509Attribute(CK_ATTRIBUTE_TYPE attrType, CK_BYTE *pemBuf, size_t pemBuflen, int16_t bufSize, char *outbuf,
                         int16_t *plenAttr)
{
  int32_t ret;

  mbedtls_x509_crt crt;
  mbedtls_x509_crt *cur = &crt;
  mbedtls_x509_crt_init(&crt); /* Initialize a certificate (chain) */
  /*
    Parse a single DER formatted certificate and add it to the chained list
  */
  ret = mbedtls_x509_crt_parse_der(&crt, pemBuf, pemBuflen);
  if (ret >= 0)
  {
    if (attrType == CKA_SUBJECT) /* get certificate subject*/
    {
      /*
         Store the certificate DN in printable form into buf;
         no more than size characters will be written
      */
      *plenAttr = (int16_t)mbedtls_x509_dn_gets(outbuf, (size_t) bufSize, &cur->subject);
    }
    else if (attrType == CKA_SERIAL_NUMBER) /* get certificate serial number */
    {
      /*
          Store the certificate serial in printable form into buf;
          no more than size characters will be written
      */
      *plenAttr = (int16_t)mbedtls_x509_serial_gets(outbuf, (size_t) bufSize, &cur->serial);
    }
    else if (attrType == CKA_ISSUER) /* get certificate issuer */
    {
      *plenAttr = (int16_t)mbedtls_x509_dn_gets(outbuf, (size_t) bufSize, &cur->issuer);
    }
    else
    {
      ret = -1;
    }
  }
  mbedtls_x509_crt_free(&crt); /* Unallocate all certificate data */
  return ret;
}

/**************************************************************/
/*               general-purpose functions.                   */
/**************************************************************/
/**
  * @brief Set certificate ID in the PKCS#11 session structure.
  *
  * @param[in] pxSessionObj          Pointer to PKCS#11 session structure.
  * @param[in] pLabel                Label of pkcs#11 object .
  *
  * @return TRUE if successful or false in case of any error
  *
  */

bool setEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel)
{
  bool result = true;

  if (strcmp((CCHAR *)pLabel, MAIN_CERTIFICATE) == 0)
  {
    pxSessionObj->targetCertificateParameter = (CK_BYTE)PRIMARY_CERTIFICATE_TARGET_FILE;
  }
  else if (strcmp((CCHAR *)pLabel, SECONDARY_CERTIFICATE) == 0)
  {
    pxSessionObj-> targetCertificateParameter = (CK_BYTE)BACKUP_CERTIFICATE_TARGET_FILE;
  }
  else if (strcmp((CCHAR *)pLabel, THIRD_CERTIFICATE) == 0)
  {
    pxSessionObj-> targetCertificateParameter = (CK_BYTE)THIRD_CERTIFICATE_TARGET_FILE;
  }
  else if (strcmp((CCHAR *)pLabel, FOURTH_CERTIFICATE) == 0)
  {
    pxSessionObj-> targetCertificateParameter = (CK_BYTE)FOURTH_CERTIFICATE_TARGET_FILE;
  }
  else
  {
    result = false;
  }

  return result;
}

/**
  * @brief Set Private key ID in the PKCS#11 session structure.
  *
  * @param[in] pxSessionObj          Pointer to PKCS#11 session structure.
  * @param[in] pLabel                Label of pkcs#11 object .
  *
  * @return TRUE if successful or false in case of any error
  *
  */
bool setPrivateKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel)
{
  bool result = true;

  if (strcmp((CCHAR *)pLabel, MAIN_PRIVATE_KEY_IN_KEY_PAIR) == 0)
  {
    pxSessionObj->targetKeyParameter = (CK_BYTE)MAIN_PRIVATE_KEY_TARGET;
  }
  else if (strcmp((CCHAR *)pLabel, SECONDARY_PRIVATE_KEY_IN_KEY_PAIR) == 0)
  {
    pxSessionObj-> targetKeyParameter = (CK_BYTE)SECONDARY_PRIVATE_KEY_TARGET;
  }
  else if (strcmp((CCHAR *)pLabel, THIRD_PRIVATE_KEY_IN_KEY_PAIR) == 0)
  {
    pxSessionObj-> targetKeyParameter = (CK_BYTE)THIRD_PRIVATE_KEY_TARGET;
  }
  else
  {
    result = false;
  }

  return result;
}

/**
  * @brief Only for Third Private Key, set Private key ID in the PKCS#11 session structure.
  *
  * @param[in] pxSessionObj          Pointer to PKCS#11 session structure.
  * @param[in] pLabel                Label of pkcs#11 object .
  *
  * @return TRUE if successful or false in case of any error
  *
  */
bool setThirdPrivateKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel)
{
  bool result = true;

  if (strcmp((CCHAR *)pLabel, THIRD_PRIVATE_KEY_IN_KEY_PAIR) == 0)
  {
    pxSessionObj-> targetKeyParameter = (CK_BYTE)THIRD_PRIVATE_KEY_TARGET;
  }
  else
  {
    result = false;
  }

  return result;
}

/**
  * @brief Set Public key ID in the PKCS#11 session structure.
  *
  * @param[in] pxSessionObj          Pointer to PKCS#11 session structure.
  * @param[in] pLabel                Label of pkcs#11 object .
  *
  * @return TRUE if successful or false in case of any error
  *
  */
bool setPublicKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel)
{
  bool result = true;

  if (strcmp((CCHAR *)pLabel, MAIN_PUBLIC_KEY) == 0)
  {
    pxSessionObj->targetKeyParameter = MAIN_PUBLIC_KEY_TARGET;
  }
  else if (strcmp((CCHAR *)pLabel, SECONDARY_PUBLIC_KEY) == 0)
  {
    pxSessionObj->targetKeyParameter = SECONDARY_PUBLIC_KEY_TARGET;
  }
  else
  {
    result = false;
  }

  return result;
}

/**
  * @brief Set Public key ID in the PKCS#11 session structure.
  *
  * @param[in] pxSessionObj          Pointer to PKCS#11 session structure.
  * @param[in] pLabel                Label of pkcs#11 object .
  *
  * @return TRUE if successful or false in case of any error
  *
  */
bool setCompleteKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, CK_VOID_PTR pValue, uint16_t *pubkeyFid)
{
  bool result = true;
  if (strcmp(pValue, MAIN_PRIVATE_KEY_IN_KEY_PAIR) == 0)
  {
    pxSessionObj->targetKeyParameter = (CK_BYTE)MAIN_PRIVATE_KEY_TARGET;
    *pubkeyFid = MAIN_PUBLIC_KEY_FID;
  }
  else if (strcmp(pValue, SECONDARY_PRIVATE_KEY_IN_KEY_PAIR) == 0)
  {
    pxSessionObj-> targetKeyParameter = (CK_BYTE)SECONDARY_PRIVATE_KEY_TARGET;
    *pubkeyFid = SECONDARY_PUBLIC_KEY_FID;
  }
  else
  {
    result = false;
  }
  return result;
}

void setGlobalEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel)
{
  if ((strcmp((CCHAR *)pLabel, MAIN_PRIVATE_KEY_IN_KEY_PAIR) == 0)
      || (strcmp((CCHAR *)pLabel, MAIN_PUBLIC_KEY_IN_KEY_PAIR) == 0))
  {
    pxSessionObj->targetKeyParameter = MAIN_PRIVATE_KEY_TARGET;
  }
  else if ((strcmp((CCHAR *)pLabel, SECONDARY_PRIVATE_KEY_IN_KEY_PAIR) == 0)
           || (strcmp((CCHAR *)pLabel, SECONDARY_PUBLIC_KEY_IN_KEY_PAIR) == 0))
  {
    pxSessionObj->targetKeyParameter = SECONDARY_PRIVATE_KEY_TARGET;
  }
  else if ((strcmp((CCHAR *)pLabel, THIRD_PRIVATE_KEY_IN_KEY_PAIR) == 0))
  {
    pxSessionObj->targetKeyParameter = THIRD_PRIVATE_KEY_TARGET;
  }
  else if (strcmp((CCHAR *)pLabel, MAIN_CERTIFICATE) == 0)
  {
    pxSessionObj->targetCertificateParameter = PRIMARY_CERTIFICATE_TARGET_FILE;
  }
  else if (strcmp((CCHAR *)pLabel, SECONDARY_CERTIFICATE) == 0)
  {
    pxSessionObj->targetCertificateParameter = BACKUP_CERTIFICATE_TARGET_FILE;
  }
  else if (strcmp((CCHAR *)pLabel, THIRD_CERTIFICATE) == 0)
  {
    pxSessionObj->targetCertificateParameter = THIRD_CERTIFICATE_TARGET_FILE;
  }
  else if (strcmp((CCHAR *)pLabel, FOURTH_CERTIFICATE) == 0)
  {
    pxSessionObj->targetCertificateParameter = FOURTH_CERTIFICATE_TARGET_FILE;
  }
  else
  {
    /*****************/
  }
}

/**
  * @brief Set the key ID to be used in the PKCS#11 session structure.
  *
  * @param[in] pxSessionObj          Pointer to PKCS#11 session structure.
  * @param[in] pLabel                Label of pkcs#11 object .
  *
  * @return TRUE if successful or false in case of any error
  *
  */
bool setSignKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel)
{
	/* Checks:
	 * 1. first 4 characters of pLabel must be "PMK_" or pLabel must be "MSK"
	 * 2. length of pLabel must be 5
	 * 3. 5° character must be a digit
	 */

	/*
	 * pxSessionObj->targetKeyParameter represents P1 for sign command
	 */
  bool result = true;

  if (strcmp((CCHAR *)pLabel, FIRST_DERIVED_KEY) == 0)
  {
	  pxSessionObj->targetKeyParameter = (CK_BYTE)0;
  }
  else if (strncmp(pLabel, "PMK_", 4) == 0)
  {
	  if (strlen(pLabel) != 5)
	  {
		  result = false;
	  }
	  else
	  {
		  if (isdigit(pLabel[4]) != 0)
		  {
			  result = false;
		  }
		  else
		  {
			  pxSessionObj->targetKeyParameter = (CK_BYTE)pLabel[4]; // last char of pLabel (i.e. PMK_4)
		  }
	  }
  }
  else
  {
	  result = false;
  }

  return result;
}


/**************************************************************/
/*              end of general-purpose functions.                   */
/**************************************************************/

/**
  * @brief Generates a 32-bit random number. Used by SRand function.
  *
  * @return Value of random.
  *
  */
uint32_t generateRandom(void)
{
  uint32_t ret;
  RNG_HandleTypeDef rng_handle = { 0 };
  __HAL_RCC_RNG_CLK_ENABLE(); /* Enable the RNG controller clock */

  rng_handle.Instance = RNG;
  HAL_StatusTypeDef status = HAL_RNG_Init(&rng_handle); /* Initializes the RNG peripheral
                                                          and creates the associated handle.*/
  if (status != HAL_OK)
  {
    ret = 0x00000000U;
  }
  else
  {
    status = HAL_RNG_GenerateRandomNumber(&rng_handle, &ret); /* Generates a 32-bit random number.*/
    if (status != HAL_OK)
    {
      ret = 0x00000000U;
    }
  }
  return ret;
}

/**
  * @brief This function converts a Hexadecimal byte into two ASCII characters.
  *
  * @param[in] byteToConvert     The byte to convert
  * @param[out] firstChar        Buffer that receives the derived key.
  * @param[out] secondChar       Length in bytes of the erived key.
  *
  * @return 0 if successful or -1 in case of any error.
  */
int8_t convertHexByteToAscii(uint8_t byteToConvert, uint8_t *firstChar, uint8_t *secondChar)
{
  int8_t retValue;
  CK_BYTE loNibble = (LO_NIBBLE(byteToConvert)); /* this is the low nibble */
  CK_BYTE hiNibble = (HI_NIBBLE(byteToConvert)); /* this is the high nibble */

  retValue = nibbleToChar(hiNibble, firstChar);
  if (retValue == (int8_t)0)
  {
    /*
      * first conversion OK
      */
    retValue = nibbleToChar(loNibble, secondChar);
  }

  return retValue;
}

/**
  * @brief This function converts a nibble (seen as a byte) into one ASCII character.
  *
  * @param[in] nibble           The vaue of the input nibble
  * @param[out] convertedChar   The converted char.
  *
  * @return 0 if successful or -1 if the input parameter is not equivalent to a nibble (greater than 15).
  */
int8_t nibbleToChar(uint8_t nibble, uint8_t *convertedChar)
{
  int8_t retValue = (int8_t)0;

  if (nibble <= (uint8_t)9)
  {
    *convertedChar = nibble + (uint8_t)48;
  }
  else if ((nibble >= (uint8_t)10) && (nibble <= (uint8_t)15))
  {
    *convertedChar = nibble + (uint8_t)55;
  }
  else
  {
    /*
      * Generic Error: unexpected value for nibbleChar
      */
    retValue = (int8_t)(-1);
  }

  return retValue;
}

/* Check Private Key and adapt size and Value*/
int8_t checkAndAdaptPrivateKeySizeAndValue(CK_ULONG valueSize, CK_BYTE *pbValue)
{
  int8_t retValue = (int8_t)0;
  CK_BYTE *pbSecondValue = pbValue;
  pbSecondValue++;

  if (valueSize == ((CK_ULONG)PRIVATE_KEY_LEN))
  {
    /* Len = 32 */
    if (*pbValue == 0U)
    {
      retValue = -1;
    }
  }
  else if (valueSize == ((CK_ULONG)((CK_ULONG)PRIVATE_KEY_LEN + 1UL)))
  {
    /* Len = 33 */
    if (*pbValue != 0U)
    {
      retValue = -1;
    }
    else
    {
      if (*pbSecondValue == 0U)
      {
        retValue = -1;
      }
    }
  }
  else
  {
    /* Other Lengths */
    retValue = -1;
  }

  return retValue;
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
