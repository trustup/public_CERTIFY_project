/**
  ******************************************************************************
  * @file    st_util.h
  * @author  MCD Application Team
  * @brief   Header for st_util.c module
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
#ifndef ST_UTIL_H
#define ST_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "st_p11.h"

/* FreeRTOS headers are included in order to avoid warning during compilation */
/* FreeRTOS is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "mbedtls/x509_crt.h"
/*cstat +MISRAC2012-* */

#include "com_common.h"

/* Exported constants --------------------------------------------------------*/
#define PRIMARY_CERTIFICATE_TARGET_FILE   0x16
#define BACKUP_CERTIFICATE_TARGET_FILE    0x17
#define THIRD_CERTIFICATE_TARGET_FILE     0x20
#define FOURTH_CERTIFICATE_TARGET_FILE    0x21

#define MAIN_PRIVATE_KEY_TARGET           0x16
#define SECONDARY_PRIVATE_KEY_TARGET      0x17
#define THIRD_PRIVATE_KEY_TARGET          0x20

#define MAIN_PUBLIC_KEY_FID               0x8016
#define SECONDARY_PUBLIC_KEY_FID          0x8017
#define THIRD_PUBLIC_KEY_FID              0x8020

#define MAIN_PUBLIC_KEY_TARGET            0X40
#define SECONDARY_PUBLIC_KEY_TARGET       0X41

#define PUBLIC_KEY_VALUE_LEN              170

#define HI_BYTE(VALUE_TO_CONVERT) \
  (uint8_t)((((uint16_t)VALUE_TO_CONVERT) & 0xFF00U) >>8U)  /* from short to low byte */

#define LO_BYTE(VALUE_TO_CONVERT) \
  (uint8_t)(((uint16_t)VALUE_TO_CONVERT) & 0x00FFU)  /* from short to high byte*/

#define LO_NIBBLE(VALUE_TO_CONVERT) (((uint8_t)VALUE_TO_CONVERT) & 0x0FU) /* from byte to low nibble */
#define HI_NIBBLE(VALUE_TO_CONVERT) ((((uint8_t)VALUE_TO_CONVERT) >> 4U) & 0x0FU) /* from byte to high nibble */

/* Exported functions ------------------------------------------------------- */

/* Retrieves the token objects from the config file. */
CK_RV LoadTokenObjects(void);
/* Adds a P11 object to the list of P11 objects. */
CK_RV addObjectToList(P11Object_t objectToAdd, CK_OBJECT_HANDLE *handle);
/* Removes a P11 object to the list of P11 objects. */
CK_RV removeObjectFromList(CK_OBJECT_HANDLE *pHandle);
/* Removes session objects from the list of objects. */
CK_RV removeSessionObjectFromList(void);
/* Removes Session Private and Public Keys from the list of objects */
void removeSessionKeyPairsFromList(void);
/* Converts an hex string into a byte array. */
void hexstr_to_char(uint8_t *hexstr, uint8_t *charstr, uint32_t len);

/* Finds, by identifier, a P11 objects in the list of P11 objects. */
void findObjectInListByLabel(int8_t *objectLabel, P11Object_t *p11Obj);
/* Finds, by handle, a P11 objects in the list of P11 objects. */
void findObjectInListByHandle(CK_OBJECT_HANDLE xObject, CK_OBJECT_HANDLE_PTR xHandle, P11Object_t *p11Obj);

/* Validates a TLV (Tag Len Value) structure */
bool TLV_Validate(const CK_BYTE *pTlv, CK_ULONG ulTlvSize);
/* Gets data from a TLV structure. */
bool TLV_Get(uint16_t wTag, const CK_BYTE *pTlv, CK_ULONG ulTlvSize, const CK_BYTE **pData, CK_ULONG *pulData);

/* Returns the value of a public key. */
CK_RV getPublicKeyValue(P11SessionPtr_t pxSessionObj, CK_BYTE_PTR publicKeyValue, CK_ULONG_PTR publicKeyValueLen);

/* Checks if the response buffer pointed by pbuf_rsp ends with "9000". */
bool is9000(com_char_t *pbuf_rsp, int32_t ret);

/* Returns the content of a certificate. */
CK_RV getCertificateFileContent(P11SessionPtr_t pxSessionObj, CK_BYTE_PTR certValue, uint16_t *certValueLen);
/* Returns the Lenght of a certificate. */
CK_RV getCertificateFileContentLength(CK_BYTE targetFile, uint16_t *certValueLen);

int32_t GetAttributePos(CK_ATTRIBUTE_TYPE attributeType, const CK_ATTRIBUTE *pTemplate, CK_ULONG ulAttributeCount);
CK_BYTE *GetAttribute(CK_ATTRIBUTE_TYPE attributeType, const CK_ATTRIBUTE *pTemplate, CK_ULONG ulAttributeCount,
                      CK_ULONG *pulAttributeSize, CK_ULONG *pulPosition);
int32_t getX509Attribute(CK_ATTRIBUTE_TYPE attrType, CK_BYTE *pemBuf, size_t pemBuflen, int16_t bufSize, char *outbuf,
                         int16_t *plenAttr);
CK_RV checkGenerateKeyPairPrivatePublicTemplates(CK_ATTRIBUTE_PTR pPrivateTemplate, CK_ULONG ulPrivateTemplateLength,
                                                 CK_ATTRIBUTE_PTR pPublicTemplate, CK_ULONG ulPublicTemplateLength,
                                                 CK_ATTRIBUTE_PTR pPrivateLabel, CK_ATTRIBUTE_PTR pPublicLabel,
                                                 bool *isToken);
CK_RV DeleteCertificateValue(P11SessionPtr_t pxSessionObj);
CK_RV WriteCertificateValue(P11SessionPtr_t pxSessionObj, CK_BYTE_PTR certValue, CK_ULONG certValueLen);
CK_RV ParseECParam(CK_VOID_PTR pParameter, CK_ECDH1_DERIVE_PARAMS *params);
void updateHandleForObject(int8_t *xLabel, CK_ULONG labelSize, P11Object_t *p11Obj);
bool setEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel);
bool setThirdPrivateKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel);
bool setPrivateKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel);
void setGlobalEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel);
bool setCompleteKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, CK_VOID_PTR pValue, uint16_t *pubkeyFid);
bool setPublicKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel);

int8_t convertHexByteToAscii(uint8_t byteToConvert, uint8_t *firstChar, uint8_t *secondChar);
int8_t nibbleToChar(uint8_t nibble, uint8_t *convertedChar);
int8_t checkAndAdaptPrivateKeySizeAndValue(CK_ULONG valueSize, CK_BYTE *pbValue);

CK_RV getDataObjectLengthAndValue(CK_BYTE targetFile, uint16_t *fileContentLen, CK_BYTE_PTR pDataObjectValue);
CK_RV UpdateFileValue(P11SessionPtr_t pxSessionObj, CK_BYTE targetFile, CK_BYTE_PTR fileValue, CK_ULONG fileValueLen);
bool setSignKeyEnvironmentTarget(P11SessionPtr_t pxSessionObj, int8_t *pLabel);
CK_RV ParseHKDFParam(CK_VOID_PTR pParameter, CK_HKDF_PARAMS *params);

#ifdef __cplusplus
}
#endif

#endif /* ST_UTIL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
