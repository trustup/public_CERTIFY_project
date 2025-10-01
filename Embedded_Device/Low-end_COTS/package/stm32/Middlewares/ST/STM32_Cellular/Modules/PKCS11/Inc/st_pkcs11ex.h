/**
  ******************************************************************************
  * @file    st_pkcs11ex.h
  * @author  MCD Application Team
  * @brief   Include file for PKCS #11
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

#ifndef ST_PKCS11EX_H
#define ST_PKCS11EX_H

#ifdef WIN32
#pragma pack(push, cryptoki, 1)
#endif /* WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

#define CK_PTR *

#ifdef WIN32
#define CK_DEFINE_FUNCTION(returnType, name) \
  returnType __declspec(dllexport) name
#define CK_DECLARE_FUNCTION(returnType, name) \
  returnType __declspec(dllexport) name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) \
  returnType __declspec(dllexport) (* name)
#define CK_CALLBACK_FUNCTION(returnType, name) \
  returnType (* name)
#else
#define CK_DEFINE_FUNCTION(returnType, name) \
  returnType name
#define CK_DECLARE_FUNCTION(returnType, name) \
  returnType name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) \
  returnType (* name)
#define CK_CALLBACK_FUNCTION(returnType, name) \
  returnType (* name)
#endif /* WIN32 */

#ifndef NULL_PTR
#define NULL_PTR 0
#endif /* !NULL_PTR */

/* FreeRTOS headers are included in order to avoid warning during compilation */
/* FreeRTOS is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "pkcs11.h"
/*cstat +MISRAC2012-* */

#ifdef __cplusplus
}
#endif

#ifdef WIN32
#pragma pack(pop, cryptoki)
#endif /* WIN32 */

#endif /* ST_PKCS11EX_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
