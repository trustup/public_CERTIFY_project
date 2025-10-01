/**
  ******************************************************************************
  * @file    st_p11.c
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
#include "st_p11.h"
#include "st_util.h"
#include "st_comm_layer.h"
#include <time.h>
/* Indicates that no PKCS #11 operation is underway for the given session. */
/* #define PKCS11_NO_OPERATION       ( ( CK_MECHANISM_TYPE ) 0xFFFFFFFFF ) */
/**
  * @brief Helper definitions.
  */

/*
    * Check if module is Initialized.
*/
#define PKCS11_MODULE_IS_INITIALIZED            \
  (((xP11Context.xIsInitialized == CK_TRUE) ? (bool)CK_TRUE : (bool)CK_FALSE))

/*
    * Check if Session is open.
*/
#define PKCS11_SESSION_IS_OPEN(xSessionHandle)  \
  (((((P11SessionPtr_t)(xSessionHandle))->xOpened) == CK_TRUE) ? CKR_OK : CKR_SESSION_CLOSED)

/*
    * Check if is a valid session.
*/
#define PKCS11_SESSION_IS_VALID(xSessionHandle) \
  (((xSessionHandle) != (CK_ULONG)NULL) ? PKCS11_SESSION_IS_OPEN((xSessionHandle)) : CKR_SESSION_HANDLE_INVALID)

#define PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSessionHandle) \
  ((PKCS11_MODULE_IS_INITIALIZED) ? PKCS11_SESSION_IS_VALID((xSessionHandle)) : CKR_CRYPTOKI_NOT_INITIALIZED)

/**
  * @brief Maps an opaque caller session handle into its internal state structure.
  */
static P11SessionPtr_t prvSessionPointerFromHandle(CK_SESSION_HANDLE xSession)
{
  return (P11SessionPtr_t) xSession;
}
/*-------------------------------------------------------------*/

/*
 * PKCS#11 module implementation.
 */

/*
  * Cryptoki provides the following general-purpose functions.
*/

/**
  * @brief Initialize the PKCS #11 module for use.
  *
  * @note C_Initialize is not thread-safe.
  *
  * C_Initialize should be called (and allowed to return) before any additional PKCS #11 operations are invoked.
  *
  * In this implementation, all input arguments are ignored.
  *
  * @param[in] pvInitArgs    This parameter is ignored.
  *
  * @return  CKR_OK if successful.
  *              CKR_CRYPTOKI_ALREADY_INITIALIZED if C_Initialize was previously called.
  *              CKR_FUNCTION_FAILED if it's impossible to read the Certificates
  *              CKR_GENERAL_ERROR if it's impossible to Select the Applet
  *              All other errors indicate that the PKCS #11 module is not ready to be used.
  */
CK_DECLARE_FUNCTION(CK_RV, C_Initialize)(CK_VOID_PTR pvInitArgs)
{
  (void)(pvInitArgs);

  CK_RV xResult;

  if (xP11Context.xIsInitialized != CK_TRUE)
  {
    (void)memset(&xP11Context, 0, sizeof(xP11Context));
    /*
      * Open a new communication
    */
    xResult = InitCommunicationLayer();
    if (xResult == CKR_OK)
    {
      xP11Context.xIsInitialized = (CK_BBOOL)CK_TRUE;
      /*
        * add token objects handle to list of know object
      */
      xResult = LoadTokenObjects();
    }
  }
  else
  {
    xResult = CKR_CRYPTOKI_ALREADY_INITIALIZED; /* */
  }

  return xResult;
}


/**
  * @brief C_Finalize is called to indicate that an application is finished with the Cryptoki library.
  * @note  It should be the last Cryptoki call made by an application.
  *
  * param pvReserved    The pReserved parameter is reserved for future versions;
  *                     for this version, it should be set to NULL_PTR.
  *                     if C_Finalize is called with a non-NULL_PTR value for pReserved,
  *                     it should return the value CKR_ARGUMENTS_BAD.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED, Vendor defined error.
  *
  */
CK_DECLARE_FUNCTION(CK_RV, C_Finalize)(CK_VOID_PTR pvReserved)
{
  CK_RV xResult = CKR_OK;

  if (pvReserved != NULL)
  {
    xResult = CKR_ARGUMENTS_BAD;
  }

  if (xResult == CKR_OK)
  {
    if (xP11Context.xIsInitialized == CK_FALSE)
    {
      xResult = CKR_CRYPTOKI_NOT_INITIALIZED;
    }
    xP11Context.xIsInitialized = (CK_BBOOL)CK_FALSE;
    xP11session.xOpened = (CK_BBOOL)CK_FALSE;
    xP11session.ephemeralKeyGenerated = (CK_BBOOL)CK_FALSE;
  }
  /*
    * Close the communication
  */
  xResult =  CloseCommunication();
  return xResult;
}

/**
  * @brief C_GetFunctionList obtains a pointer to the PKCS #11 module's list
  * of function pointers.
  *
  * All other PKCS #11 functions should be invoked using the returned function list.
  *
  * param[in] ppxFunctionList       Pointer to the location where pointer to function list will be placed.
  *
  * @return CKR_OK if successful or a specific error code.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetFunctionList)(CK_FUNCTION_LIST_PTR_PTR ppxFunctionList)
{
  CK_RV xResult = CKR_OK;
  /**
    * @brief PKCS#11 interface functions implemented by this Cryptoki module.
  */
  static CK_FUNCTION_LIST prvP11FunctionList =
  {
    { CRYPTOKI_VERSION_MAJOR, CRYPTOKI_VERSION_MINOR },
    C_Initialize,
    C_Finalize,
    C_GetInfo,
    C_GetFunctionList,
    C_GetSlotList,
    C_GetSlotInfo,
    C_GetTokenInfo,
    C_GetMechanismList,
    C_GetMechanismInfo,
    NULL,     /*C_InitToken,*/
    NULL,     /*C_InitPIN*/
    NULL,     /*C_SetPIN*/
    C_OpenSession,
    C_CloseSession,
    NULL,     /*C_CloseAllSessions*/
    NULL,     /*C_GetSessionInfo*/
    NULL,     /*C_GetOperationState*/
    NULL,     /*C_SetOperationState*/
    NULL,   /*C_Login*/
    NULL,     /*C_Logout*/
    C_CreateObject,
    NULL,     /*C_CopyObject*/
	NULL,	  /*C_DestroyObject*/
    NULL,     /*C_GetObjectSize*/
    C_GetAttributeValue,
    NULL,     /*C_SetAttributeValue*/
    C_FindObjectsInit,
    C_FindObjects,
    C_FindObjectsFinal,
	C_EncryptInit,
	C_Encrypt,
    NULL,     /*C_EncryptUpdate*/
    NULL,     /*C_EncryptFinal*/
	C_DecryptInit,
	C_Decrypt,
    NULL,     /*C_DecryptUpdate*/
    NULL,     /*C_DecryptFinal*/
    NULL,     /*C_DigestInit,*/
    NULL,     /*C_Digest*/
    NULL,     /*C_DigestUpdate,*/
    NULL,     /* C_DigestKey*/
    NULL,     /*C_DigestFinal,*/
    C_SignInit,
    C_Sign,
    NULL,     /*C_SignUpdate*/
    NULL,     /*C_SignFinal*/
    NULL,     /*C_SignRecoverInit*/
    NULL,     /*C_SignRecover*/
	NULL,     /*C_VerifyInit*/
	NULL,     /*C_Verify**/
    NULL,     /*C_VerifyUpdate*/
    NULL,     /*C_VerifyFinal*/
    NULL,     /*C_VerifyRecoverInit*/
    NULL,     /*C_VerifyRecover*/
    NULL,     /*C_DigestEncryptUpdate*/
    NULL,     /*C_DecryptDigestUpdate*/
    NULL,     /*C_SignEncryptUpdate*/
    NULL,     /*C_DecryptVerifyUpdate*/
    NULL,     /*C_GenerateKey*/
	NULL,     /*C_GenerateKeyPair*/
    NULL,     /*C_WrapKey*/
    NULL,     /*C_UnwrapKey*/
    C_DeriveKey,
    NULL,     /*C_SeedRandom*/
    C_GenerateRandom,
    NULL,     /*C_GetFunctionStatus*/
    NULL,     /*C_CancelFunction*/
    NULL      /*C_WaitForSlotEvent*/
  };


  if (NULL == ppxFunctionList)
  {
    xResult = CKR_ARGUMENTS_BAD;
  }
  else
  {
    *ppxFunctionList = &prvP11FunctionList;
  }

  return xResult;
}
/*-----------------------------------------------------------------------------------------*/

/*
  * Cryptoki provides the following functions for session management:
*/

/**
  * @brief C_OpenSession start a PKCS#11 session for a cryptographic command sequence.
  *
  * @note PKCS #11 module must have been previously initialized with a call to C_Initialize()
  *       before calling C_OpenSession().
  *
  * param[in]  xSlotID         This parameter is unused.
  * param[in]  xFlags          Session flags - CKF_SERIAL_SESSION is a mandatory flag.
  * param[in]  pvApplication   This parameter is unused.
  * param[in]  xNotify         This parameter is unused.
  * param[in]  pxSession       Pointer to the location that receives the created session's handle.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_PARALLEL_NOT_SUPPORTED, CKR_FUNCTION_FAILED,
  *         CKR_SLOT_ID_INVALID
  */
CK_DECLARE_FUNCTION(CK_RV, C_OpenSession)(CK_SLOT_ID xSlotID,
                                          CK_FLAGS xFlags,
                                          CK_VOID_PTR pvApplication,
                                          CK_NOTIFY xNotify,
                                          CK_SESSION_HANDLE_PTR pxSession)
{
  CK_RV xResult = CKR_OK;
  if (xSlotID != (uint32_t)1) /* only slot 1 is present*/
  {
    xResult = CKR_SLOT_ID_INVALID;
  }
  if (xResult == CKR_OK)
  {
    /*
      * Check that the PKCS #11
      * module is initialized
    */
    if (PKCS11_MODULE_IS_INITIALIZED != CK_TRUE)
    {
      xResult = CKR_CRYPTOKI_NOT_INITIALIZED;
    }
  }
  if (xResult == CKR_OK)
  {
    /*
      * Check that the Session is not already opened.
    */
    if (xP11session.xOpened == CK_TRUE)
    {
      xResult = CKR_FUNCTION_FAILED;
    }
  }
  if (xResult == CKR_OK)
  {
    /*
      * Check arguments.
    */
    if (pxSession == NULL)
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }
  if (xResult == CKR_OK)
  {
    /*
      * For legacy reasons, the CKF_SERIAL_SESSION bit must always be set;
      * if a call to C_OpenSession does not have this bit set,
      * the call should return unsuccessfully with the error code CKR_SESSION_PARALLEL_NOT_SUPPORTED.
    */
    if ((uint32_t)0 == (CKF_SERIAL_SESSION & xFlags))
    {
      xResult = CKR_SESSION_PARALLEL_NOT_SUPPORTED;
    }
  }
  /*
    * Sets the context.
  */
  if (xResult == CKR_OK)
  {
    (void)memset(&xP11session, 0, sizeof(xP11session));
    xP11session.ulState = (0u != (xFlags & CKF_RW_SESSION)) ? CKS_RW_PUBLIC_SESSION : CKS_RO_PUBLIC_SESSION;
    xP11session.xOpened = (CK_BBOOL)CK_TRUE;
    xP11session.xLogged = (CK_BBOOL)CK_FALSE;
    *pxSession = (CK_ULONG) &xP11session;
  }

  if (xResult != CKR_OK)
  {
    /*
      *clean up memory
    */
    (void)memset(&xP11session, 0, sizeof(xP11session));
  }

  return xResult;
}

/**
  * @brief Terminates a session and release all resources.
  *
  * @param[in]   xSession        The session handle to be terminated.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_CloseSession)(CK_SESSION_HANDLE xSession)
{
  /*
    * Check that the PKCS #11 module is initialized,
    * if the session is open and valid.
  */

  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    xResult = removeSessionObjectFromList(); /* Delete the handle of session object from list */
    xP11session.xOpened = (CK_BBOOL)CK_FALSE;/* Close Session */
    (void)memset(secretLabel, 0x00, MAX_LABEL_LENGTH); /* Clean up memory */
    (void)memset(secretValue, 0x00, SECRET_KEY_LENGTH);  /* Clean up memory */
  }

  return xResult;
}


/**
  * @brief This login operation is not implemented.
  *
  * @return CKR_OK or a specific error code.
  */
/*
CK_DECLARE_FUNCTION( CK_RV, C_Login )( CK_SESSION_HANDLE hSession,
                                       CK_USER_TYPE userType,
                                       CK_UTF8CHAR_PTR pPin,
                                       CK_ULONG ulPinLen )
{
  ( void ) hSession;
  ( void ) userType;
  ( void ) pPin;
  ( void ) ulPinLen;

  CK_RV xResult = CKR_OK;
  P11SessionPtr_t pxSession = prvSessionPointerFromHandle( hSession );
  if (pxSession->xLogged == CK_TRUE)
  {
    xResult = CKR_USER_ALREADY_LOGGED_IN;
  }
  else
  {
    xResult = doLogin(pPin,ulPinLen );
    if (xResult == CKR_OK)
    {
      pxSession->xLogged = CK_TRUE;
    }
  }

  return xResult;
}
*/

/**
  * @brief initializes a search for token objects that match a template.
  * After calling C_FindObjectsInit, the application may call C_FindObjects one or more
  * times to obtain handles for objects matching the template, and then eventually call
  * C_FindObjectsFinal to finish the active search operation
  *
  * @param[in] xSession          The session handle.
  * @param[in] pxTemplate        Points to a search template that specifies the attribute values to match.
  * @param[in] ulCount           The number of attributes in the search template.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_FindObjectsInit)(CK_SESSION_HANDLE xSession,
                                              CK_ATTRIBUTE_PTR pxTemplate,
                                              CK_ULONG ulCount)
{
  CK_BYTE  FindObjectLabel[MAX_LABEL_LENGTH];
  uint32_t ulIndex;
  CK_ATTRIBUTE xAttribute;

  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if (NULL == pxTemplate)
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  if (xResult == CKR_OK)
  {
    (void)memset(FindObjectLabel, 0x00, MAX_LABEL_LENGTH);
    (void)memcpy(pxSession->pxFindObjectLabel, FindObjectLabel, MAX_LABEL_LENGTH);

    /*
      * Search template for label.
    */
    xResult = CKR_ARGUMENTS_BAD;
    for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
      xAttribute = pxTemplate[ ulIndex ];

      if (xAttribute.type == CKA_LABEL)
      {
        (void)memcpy(pxSession->pxFindObjectLabel, xAttribute.pValue, xAttribute.ulValueLen);
        xResult = CKR_OK;
        break;
      }
    }
  }

  /*
    * Clean up memory if there was an error parsing the template.
  */
  if (xResult != CKR_OK)
  {
    (void)memset(pxSession->pxFindObjectLabel, 0x00, MAX_LABEL_LENGTH);
  }

  if (xResult == CKR_OK)
  {
    pxSession->xFindObjectInit = (CK_BBOOL)1;
  }

  return xResult;
}

/**
  * @brief Continues a search for token objects that match a template
  * The search must have been initialized with C_FindObjectsInit.
  *
  * @param[in] xSession                  The session handle.
  * @param[out] pxObject                 The location that receives the list of object handles.
  * @param[in] ulMaxObjectCount          The maximum number of object handles to be returned.
  * @param[out] pulObjectCount           The location that receives the actual number of object handles returned.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OPERATION_NOT_INITIALIZED, CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED..
  */
CK_DECLARE_FUNCTION(CK_RV, C_FindObjects)(CK_SESSION_HANDLE xSession,
                                          CK_OBJECT_HANDLE_PTR pxObject,
                                          CK_ULONG ulMaxObjectCount,
                                          CK_ULONG_PTR pulObjectCount)
{
  P11Object_t p11Obj;

  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if ((NULL == pxObject) || (NULL == pulObjectCount))
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  if (xResult == CKR_OK)
  {
    if (!pxSession->xFindObjectInit)
    {
      xResult = CKR_OPERATION_NOT_INITIALIZED;
    }
  }


  if (xResult == CKR_OK)
  {
    if (0u == ulMaxObjectCount)
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  /* if (xResult == CKR_OK)
   {
     if ((CK_BBOOL)CK_TRUE == pxSession->xFindObjectComplete)
     {
       *pulObjectCount = 0;
       return CKR_OK;
     }
   }*/

  if (xResult == CKR_OK)
  {
    if ((CK_BBOOL)CK_TRUE != pxSession->xFindObjectComplete)
    {
      findObjectInListByLabel((int8_t *)pxSession->pxFindObjectLabel, &p11Obj);
      if (p11Obj.xHandle == CK_INVALID_HANDLE)
      {
        *pulObjectCount = 0;
      }
      else
      {
        *pxObject = p11Obj.xHandle;
        *pulObjectCount = 1;
      }
    }
    else
    {
      *pulObjectCount = 0;
      xResult = CKR_OK;
    }
  }
  return xResult;
}

/**
  * @brief Terminates a search for token objects.
  *
  * @param[in] xSession    The session handle.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_OPERATION_NOT_INITIALIZED, CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED..
  */
CK_DECLARE_FUNCTION(CK_RV, C_FindObjectsFinal)(CK_SESSION_HANDLE xSession)
{
  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if (!pxSession->xFindObjectInit)
    {
      xResult = CKR_OPERATION_NOT_INITIALIZED;
    }
  }

  /* if( xResult == CKR_OK ) { */
  (void)memset(pxSession->pxFindObjectLabel, 0x00, MAX_LABEL_LENGTH); /* clean up memory*/
  pxSession->xFindObjectInit = (CK_BBOOL)0;
  /* } */

  return xResult;
}

/**
  * @brief Obtains the value of one or more attributes of an object.
  *
  * @param[in] xSession                  The session handle.
  * @param[out] xObject                  The object�s handle.
  * @param[out] pxTemplate               The template that specifies which attribute values are to be obtained,
  *                                      and receives the attribute values.
  * @param[in] ulCount                   The number of attributes in the template.
  *
  * @note Object type CKO_DATA isn't supported.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OBJECT_HANDLE_INVALID, CKR_BUFFER_TOO_SMALL,
  *         CKR_ATTRIBUTE_SENSITIVE, CKR_ATTRIBUTE_TYPE_INVALID, CKR_TEMPLATE_INCONSISTENT,
  *         CKR_GENERAL_ERROR, CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED, Vendor defined error.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetAttributeValue)(CK_SESSION_HANDLE xSession,
                                                CK_OBJECT_HANDLE xObject,
                                                CK_ATTRIBUTE_PTR pxTemplate,
                                                CK_ULONG ulCount)
{

  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  CK_ULONG iAttrib;

  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;

  P11Object_t p11Obj;


  CK_BYTE certValue[MAX_CERTIFICATE_LEN];
  (void)memset(certValue, 0, MAX_CERTIFICATE_LEN);

  CK_BYTE DataObjectValue[MAX_DATAOBJECT_LEN];
  (void)memset(DataObjectValue, 0, MAX_DATAOBJECT_LEN);
  uint16_t certificateFileContentLen = 0;
  uint16_t DataObjectFileContentLen = 0;

  if (xResult == CKR_OK)
  {
    if ((NULL == pxTemplate) || ((uint32_t)0 == ulCount))
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  if (xResult == CKR_OK)
  {
    findObjectInListByHandle(xObject, &xHandle, &p11Obj); /* find object by handle*/

    if (xHandle == CK_INVALID_HANDLE)
    {
      xResult = CKR_OBJECT_HANDLE_INVALID;
    }

  }

  if (xResult == CKR_OK)
  {
	  switch (p11Obj.xClass)
	  {
	  case CKO_CERTIFICATE: /* Is a Certificate */
	  {
		  for (iAttrib = 0; (iAttrib < ulCount) && (xResult == CKR_OK); iAttrib++)
		  {
			  switch (pxTemplate[iAttrib].type)
			  {
			  case CKA_CLASS:
			  {
				  break;
			  }
			  case CKA_VALUE:
				  if (certificateFileContentLen == (uint16_t)0)
					  /*
					   * Retrieve Certificate Length only if not already retrieved
					   */
				  {
					  xResult = getCertificateFileContentLength(0x02,
							  &certificateFileContentLen);
				  }

//				  if (xResult == CKR_OK)
//				  {
//					  if (pxTemplate[iAttrib].pValue == NULL)
//					  {
//						  pxTemplate[iAttrib].ulValueLen = (uint32_t)certificateFileContentLen - (uint16_t)4;
//						  xResult = CKR_OK;
//						  //break;
//					  }
//					  else if (pxTemplate[iAttrib].ulValueLen < certificateFileContentLen)
//					  {
//						  xResult = CKR_BUFFER_TOO_SMALL;
//					  }
//					  else
//					  {
//						  /*****************/
//					  }
//				  }

				  if (xResult == CKR_OK)
				  {
					  if (certValue[0] == 0U)
					  {
						  xResult = getCertificateFileContent(pxSession, certValue, &certificateFileContentLen);
					  }
				  }

				  if (xResult == CKR_OK)
				  {
					  (void)memcpy(pxTemplate[iAttrib].pValue, certValue, \
							  (uint32_t)certificateFileContentLen);
					  pxTemplate[iAttrib].ulValueLen = (uint32_t)certificateFileContentLen;
				  }

				  break;
			  case CKA_VALUE_LEN:
			  {
				  break;
			  }
			  default:
				  xResult = CKR_ATTRIBUTE_TYPE_INVALID;
				  break;
			  }
		  }
		  break;
	  }
	  case CKO_DATA: /* Is a Data object */
	  {
		  for (iAttrib = 0; (iAttrib < ulCount) && (xResult == CKR_OK); iAttrib++)
		  {
			  switch (pxTemplate[iAttrib].type)
			  {
			  case CKA_CLASS:
			  {
				  break;
			  }
			  case CKA_VALUE:
				  xResult = getDataObjectLengthAndValue(URL_FILE_TARGET,&DataObjectFileContentLen, DataObjectValue);
				  if (xResult == CKR_OK)
				  {
					  (void)memcpy(pxTemplate[iAttrib].pValue, DataObjectValue, \
							  (uint32_t)DataObjectFileContentLen);
					  pxTemplate[iAttrib].ulValueLen = (uint32_t)DataObjectFileContentLen;
				  }
			  case CKA_VALUE_LEN:
			  {
				  break;
			  }
			  default:
				  xResult = CKR_ATTRIBUTE_TYPE_INVALID;
				  break;
			  }
		  }
		  break;
	  }
	  default:
		  xResult = CKR_TEMPLATE_INCONSISTENT;
		  break;
	  }

  }


  return xResult;
}
/*
  * Cryptoki provides the following functions for signing data
  */

/**
  * @brief Begin creating a digital signature.
  * After calling C_SignInit, the application can either call C_Sign to sign in a single part.
  *
  * @param[in] xSession                      Handle of a valid PKCS #11 session.
  * @param[in] pxMechanism                   Mechanism used to sign.
  *                                          This port supports the following mechanisms:
  *                                           - CKM_RSA_PKCS for RSA signatures
  *                                           - CKM_ECDSA for elliptic curve signatures
  *                                          Note that neither of these mechanisms perform
  *                                          hash operations.
  * @param[in] xKey                          The handle of the private key to be used for
  *                                          signature. Key must be compatible with the
  *                                          mechanism chosen by pxMechanism.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OPERATION_ACTIVE, CKR_KEY_HANDLE_INVALID,
  *         CKR_KEY_TYPE_INCONSISTENT, CKR_MECHANISM_INVALID, CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED.
  */
//CK_DECLARE_FUNCTION(CK_RV, C_SignInit)(CK_SESSION_HANDLE xSession,
//                                       CK_MECHANISM_PTR pxMechanism,
//                                       CK_OBJECT_HANDLE xKey)
//{
//  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
//  P11Object_t p11Obj;
//
//  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  if (xResult == CKR_OK)
//  {
//    if (NULL == pxMechanism)
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//    else if ((CK_ULONG)NULL != pxSession->xSignMechanism)
//    {
//      xResult = CKR_OPERATION_ACTIVE;
//    }
//    else
//    {
//      findObjectInListByHandle(xKey, &xHandle, &p11Obj);
//    }
//  }
//
//  /*
//    * Check the validity of Key handle
//  */
//  if (xResult == CKR_OK)
//  {
//    if (xHandle == CK_INVALID_HANDLE)
//    {
//      xResult = CKR_KEY_HANDLE_INVALID;
//    }
//  }
//
//  /*
//    * Check that a private key was retrieved.
//  */
//  if (xResult == CKR_OK)
//  {
//    if (p11Obj.xClass != CKO_PRIVATE_KEY)
//    {
//      xResult = CKR_KEY_TYPE_INCONSISTENT;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    pxSession->hSignKey = xHandle;
//    if (setPrivateKeyEnvironmentTarget(pxSession, p11Obj.xLabel) == false)
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  /*
//    * Check that the mechanism is compatible, supported.
//  */
//  if (xResult == CKR_OK)
//  {
//    if (pxMechanism->mechanism != CKM_ECDSA)
//    {
//      xResult = CKR_MECHANISM_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    pxSession->m_SignInitialized = (CK_BBOOL)1;
//    pxSession->xSignMechanism = pxMechanism->mechanism;
//  }
//
//  return xResult;
//}

CK_DECLARE_FUNCTION(CK_RV, C_SignInit)(CK_SESSION_HANDLE xSession,
                                       CK_MECHANISM_PTR pxMechanism,
                                       CK_OBJECT_HANDLE xKey)
{
  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
  P11Object_t p11Obj;

  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if (NULL == pxMechanism)
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
    else if ((CK_ULONG)NULL != pxSession->xSignMechanism)
    {
      xResult = CKR_OPERATION_ACTIVE;
    }
    else
    {
      findObjectInListByHandle(xKey, &xHandle, &p11Obj);
    }
  }

  /*
    * Check the validity of Key handle
  */
  if (xResult == CKR_OK)
  {
    if (xHandle == CK_INVALID_HANDLE)
    {
      xResult = CKR_KEY_HANDLE_INVALID;
    }
  }

  /*
    * Check that a secret key was retrieved.
  */
  if (xResult == CKR_OK)
  {
    if (p11Obj.xClass != CKO_SECRET_KEY)
    {
      xResult = CKR_KEY_TYPE_INCONSISTENT;
    }
  }


  if (xResult == CKR_OK)
  {
    pxSession->hSignKey = xHandle;
    if (setSignKeyEnvironmentTarget(pxSession, p11Obj.xLabel) == false)
    {
      xResult = CKR_KEY_HANDLE_INVALID;
    }
  }


  /*
    * Check that the mechanism is compatible, supported.
    * If the mechanism is NULL it's considered as CKM_AES_MAC
  */
  if (xResult == CKR_OK)
  {
	  if (pxMechanism->mechanism == (CK_ULONG)NULL)
	  {
		  pxMechanism->mechanism = CKM_AES_MAC;
	  }

      if (pxMechanism->mechanism != CKM_AES_MAC)
	  {
		  xResult = CKR_MECHANISM_INVALID;
	  }
  }

  if (xResult == CKR_OK)
  {
    pxSession->m_SignInitialized = (CK_BBOOL)1;
    pxSession->xSignMechanism = pxMechanism->mechanism;
  }

  return xResult;
}

/**
  * @brief Performs a digital signature operation.
  * The signing operation must have been initialized with C_SignInit.
  *
  * @param[in] xSession                      Handle of a valid PKCS #11 session.
  * @param[in] pucData                       Data to be signed.
  *                                          Note: Some applications may require this data to
  *                                          be hashed before passing to C_Sign().
  * @param[in] ulDataLen                     Length of pucData, in bytes.
  * @param[out] pucSignature                 Buffer where signature will be placed.
  *                                          Caller is responsible for allocating memory.
  *                                          Providing NULL for this input will cause
  *                                          pulSignatureLen to be updated for length of
  *                                          buffer required.
  * @param[in,out] pulSignatureLen           Length of pucSignature buffer.
  *                                          If pucSignature is non-NULL, pulSignatureLen is
  *                                          updated to contain the actual signature length.
  *                                          If pucSignature is NULL, pulSignatureLen is
  *                                          updated to the buffer length required for signature
  *                                          data.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_BUFFER_TOO_SMALL, CKR_OPERATION_NOT_INITIALIZED,
  *         CKR_DATA_LEN_RANGE ,CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED, Vendor defined error.
  */
//CK_DECLARE_FUNCTION(CK_RV, C_Sign)(CK_SESSION_HANDLE xSession,
//                                   CK_BYTE_PTR pucData,
//                                   CK_ULONG ulDataLen,
//                                   CK_BYTE_PTR pucSignature,
//                                   CK_ULONG_PTR pulSignatureLen)
//{
//  CK_ULONG xSignatureLength;
//  CK_ULONG xExpectedInputLength = 0;
//  CK_BYTE_PTR pxSignatureBuffer = pucSignature;
//  uint8_t ecSignature[PKCS11_ECDSA_P256_SIGNATURE_LENGTH];
//
//  P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  if (xResult == CKR_OK)
//  {
//    if (!pxSessionObj->m_SignInitialized)
//    {
//      xResult = CKR_OPERATION_NOT_INITIALIZED;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if ((ulDataLen == (uint32_t)0) || (NULL == pucData) || (NULL == pulSignatureLen))
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  if (pucSignature != NULL)
//  {
//    if (xResult == CKR_OK)
//    {
//      xSignatureLength = PKCS11_ECDSA_P256_SIGNATURE_LENGTH;
//      xExpectedInputLength = PKCS11_SHA256_DIGEST_LENGTH;
//      pxSignatureBuffer = ecSignature;
//      /*
//        * Calling application is trying to determine length needed for signature buffer.
//      */
//      if (*pulSignatureLen < xSignatureLength) /* The length of data to verify must be 32 byte*/
//      {
//        xResult = CKR_BUFFER_TOO_SMALL;
//      }
//    }
//    if (xResult == CKR_OK)
//    {
//      /*
//        * Check that input data to be signed is the expected length.
//      */
//      if (xExpectedInputLength != ulDataLen)
//      {
//        xResult = CKR_DATA_LEN_RANGE;
//      }
//    }
//    /*
//      * Sign the data.
//    */
//    if (xResult == CKR_OK)
//    {
//      xResult = ComputeSignature(pxSessionObj, pucData,
//                                 (CK_BYTE)ulDataLen,
//                                 pxSignatureBuffer,
//                                 pulSignatureLen);
//      if (xResult == (uint32_t)0)
//      {
//        (void)memcpy(pucSignature, pxSignatureBuffer, *pulSignatureLen);
//      }
//    }
//    /*
//      * Complete the operation in the context.
//    */
//    pxSessionObj->xSignMechanism = (CK_ULONG)NULL;
//    pxSessionObj->targetKeyParameter = 0x00;
//    pxSessionObj->m_SignInitialized = (CK_BBOOL)0;
//  }
//  else
//  {
//    *pulSignatureLen = PKCS11_ECDSA_P256_SIGNATURE_LENGTH;
//    xResult = CKR_OK;
//  }
//
//
//  return xResult;
//}
  CK_DECLARE_FUNCTION(CK_RV, C_Sign)(CK_SESSION_HANDLE xSession,
                                     CK_BYTE_PTR pucData,
                                     CK_ULONG ulDataLen,
                                     CK_BYTE_PTR pucSignature,
                                     CK_ULONG_PTR pulSignatureLen)
  {
    CK_ULONG xSignatureLength;
    CK_ULONG xExpectedInputLength = 0;
    CK_BYTE_PTR pxSignatureBuffer = pucSignature;
    uint8_t signature[32]; // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 32

    P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
    /*
        * Check that the PKCS #11 module is initialized,
        * if the session is open and valid.
    */
    CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

    if (xResult == CKR_OK)
    {
      if (!pxSessionObj->m_SignInitialized)
      {
        xResult = CKR_OPERATION_NOT_INITIALIZED;
      }
    }

    if (xResult == CKR_OK)
    {
      if ((ulDataLen == (uint32_t)0) || (NULL == pucData) || (NULL == pulSignatureLen))
      {
        xResult = CKR_ARGUMENTS_BAD;
      }
    }

    if (pucSignature != NULL)
    {
      if (xResult == CKR_OK)
      {
        xSignatureLength = 32; // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 32
        xExpectedInputLength = PKCS11_SHA256_DIGEST_LENGTH;
        pxSignatureBuffer = signature;
        /*
          * Calling application is trying to determine length needed for signature buffer.
        */
        if (*pulSignatureLen < xSignatureLength) /* The length of data to verify must be 32 byte*/
        {
          xResult = CKR_BUFFER_TOO_SMALL;
        }
      }
      if (xResult == CKR_OK)
      {
        /*
          * Check that input data to be signed is the expected length.
        */
        if (xExpectedInputLength != ulDataLen)
        {
          xResult = CKR_DATA_LEN_RANGE;
        }
      }
      /*
        * Sign the data.
      */
      if (xResult == CKR_OK)
      {
        xResult = ComputeSignature(pxSessionObj, pucData,
                                   (CK_BYTE)ulDataLen,
                                   pxSignatureBuffer,
                                   pulSignatureLen);
        if (xResult == (uint32_t)0)
        {
          (void)memcpy(pucSignature, pxSignatureBuffer, *pulSignatureLen);
        }
      }
      /*
        * Complete the operation in the context.
      */
      pxSessionObj->xSignMechanism = (CK_ULONG)NULL;
      pxSessionObj->targetKeyParameter = 0x00;
      pxSessionObj->m_SignInitialized = (CK_BBOOL)0;
    }
    else
    {
      *pulSignatureLen = 32; // *********** ATTENZIONE: PER MISRA QUI CI VUOLE UNA COSTANTE AL POSTO DI 32
      xResult = CKR_OK;
    }


    return xResult;
  }
/*-----------------------------------------------------------------------------------------*/

/**
  * @brief Generates a challenge (random bytes).
  *
  * @param xSession[in]          Handle of a valid PKCS #11 session.
  * @param pucRandomData[out]    The location that receives the random data.
  *                              It is the responsiblity of the application to allocate
  *                              this memory.
  * @param ulRandomLength[in]    Length of data (in bytes) to be generated.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED, Vendor defined error.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GenerateRandom)(CK_SESSION_HANDLE xSession,
                                             CK_BYTE_PTR pucRandomData,
                                             CK_ULONG ulRandomLen)
{
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if ((NULL == pucRandomData) ||
        (ulRandomLen == (uint32_t)0) || (ulRandomLen > (uint32_t)MAX_RANDOM_LENGTH))
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  if (xResult == CKR_OK)
  {
    xResult = GetChallenge(pucRandomData, ulRandomLen); /*generate random data*/
  }

  return xResult;
}

/**
  * @brief Generate a new public-private key pair.
  *
  * This port only supports generating elliptic curve P-256
  * key pairs.
  *
  * @param[in] xSession                      Handle of a valid PKCS #11 session.
  * @param[in] pxMechanism                   Pointer to a mechanism. At this time,
  *                                          CKM_EC_KEY_PAIR_GEN is the only supported mechanism.
  * @param[in] pxPublicKeyTemplate           Pointer to a list of attributes that the generated
  *                                          public key should possess.
  *                                          Public key template must have the following attributes:
  *                                          - CKA_LABEL
  *                                              - Label should be no longer than #pkcs11configMAX_LABEL_LENGTH
  *                                              and must be supported by port's PKCS #11 PAL.
  *                                          - CKA_EC_PARAMS
  *                                              - Must equal pkcs11DER_ENCODED_OID_P256.
  *                                              Only P-256 keys are supported.
  *                                          - CKA_VERIFY
  *                                              - Must be set to true.  Only public keys used
  *                                              for verification are supported.
  *                                          Public key templates may have the following attributes:
  *                                          - CKA_KEY_TYPE
  *                                              - Must be set to CKK_EC. Only elliptic curve key
  *                                              generation is supported.
  *                                          - CKA_TOKEN
  *                                              - Must be set to CK_TRUE.
  * @param[in] ulPublicKeyAttributeCount     Number of attributes in pxPublicKeyTemplate.
  * @param[in] pxPrivateKeyTemplate          Pointer to a list of attributes that the generated
  *                                          private key should possess.
  *                                          Private key template must have the following attributes:
  *                                          - CKA_LABEL
  *                                              - Label should be no longer than #pkcs11configMAX_LABEL_LENGTH
  *                                              and must be supported by port's PKCS #11 PAL.
  *                                          - CKA_PRIVATE
  *                                              - Must be set to true.
  *                                          - CKA_SIGN
  *                                              - Must be set to true.  Only private keys used
  *                                              for signing are supported.
  *                                          Private key template may have the following attributes:
  *                                          - CKA_KEY_TYPE
  *                                              - Must be set to CKK_EC. Only elliptic curve key
  *                                              generation is supported.
  *                                          - CKA_TOKEN
  *                                              - Must be set to CK_TRUE.
  *
  * @param[in] ulPrivateKeyAttributeCount    Number of attributes in pxPrivateKeyTemplate.
  * @param[out] pxPublicKey                  Pointer to the handle of the public key to be created.
  * @param[out] pxPrivateKey                 Pointer to the handle of the private key to be created.
  *
  * @note Not all attributes specified by the PKCS #11 standard are supported.
  * @note CKA_LOCAL attribute is not supported.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_MECHANISM_INVALID, CKR_TEMPLATE_INCONSISTENT,
  *         CKR_FUNCTION_FAILED, CKR_ATTRIBUTE_VALUE_INVALID ,CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED, Vendor defined error.
  */
//CK_DECLARE_FUNCTION(CK_RV, C_GenerateKeyPair)(CK_SESSION_HANDLE xSession,
//                                              CK_MECHANISM_PTR pxMechanism,
//                                              CK_ATTRIBUTE_PTR pxPublicKeyTemplate,
//                                              CK_ULONG ulPublicKeyAttributeCount,
//                                              CK_ATTRIBUTE_PTR pxPrivateKeyTemplate,
//                                              CK_ULONG ulPrivateKeyAttributeCount,
//                                              CK_OBJECT_HANDLE_PTR pxPublicKey,
//                                              CK_OBJECT_HANDLE_PTR pxPrivateKey)
//{
//  uint16_t pubkeyFid = 0x00;
//  P11Object_t p11Obj;
//
//  CK_ATTRIBUTE pxPrivateLabelAttribute;
//  CK_ATTRIBUTE pxPublicLabelAttribute;
//
//  P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
//
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  static CK_BYTE ephemeralBSO            = 0x17;
//  static uint16_t pubkeyEphemeralFid     = 0x8017;
//
//  if (xResult == CKR_OK)
//  {
//    if ((pxPublicKeyTemplate == NULL) || (pxPrivateKeyTemplate == NULL)
//        || (pxPublicKey == NULL) || (pxPrivateKey == NULL) || (pxMechanism == NULL))
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (CKM_EC_KEY_PAIR_GEN != pxMechanism->mechanism)
//    {
//      xResult = CKR_MECHANISM_INVALID;
//    }
//  }
//
//  bool isToken = (CK_BBOOL)0;
//  if (xResult == CKR_OK)
//  {
//    /* Checks that the private key template and  public key template*/
//    xResult = checkGenerateKeyPairPrivatePublicTemplates(pxPrivateKeyTemplate, ulPrivateKeyAttributeCount,
//                                                         pxPublicKeyTemplate, ulPublicKeyAttributeCount,
//                                                         &pxPrivateLabelAttribute, &pxPublicLabelAttribute, &isToken);
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (isToken)
//    {
//      if (setCompleteKeyEnvironmentTarget(pxSessionObj, pxPrivateLabelAttribute.pValue, &pubkeyFid) == false)
//      {
//        xResult = CKR_FUNCTION_FAILED;
//      }
//
//      if (xResult == CKR_OK)
//      {
//        findObjectInListByLabel(pxPrivateLabelAttribute.pValue, &p11Obj); /* find object by Label*/
//        if (p11Obj.xHandle == (CK_ULONG)NULL)
//        {
//          xResult = CKR_FUNCTION_FAILED;
//        }
//        else
//        {
//          *pxPrivateKey = p11Obj.xHandle;
//        }
//
//        findObjectInListByLabel(pxPublicLabelAttribute.pValue, &p11Obj); /* find object by Label*/
//        if (p11Obj.xHandle == (CK_ULONG)NULL)
//        {
//          xResult = CKR_FUNCTION_FAILED;
//        }
//        else
//        {
//          *pxPublicKey = p11Obj.xHandle;
//        }
//      }
//    }
//    else
//    {
//      /* Is Ephemeral */
//      if (pxSessionObj->ephemeralKeyGenerated) /* || pxSessionObj->ephemeralSecretKeyGenerated) */
//      {
//        xResult = CKR_FUNCTION_FAILED;
//      }
//      else
//      {
//        pxSessionObj->targetKeyParameter = (CK_BYTE)ephemeralBSO;
//        pubkeyFid = (uint16_t)pubkeyEphemeralFid;
//      }
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    xResult = GenerateKeyPair(pxSessionObj->targetKeyParameter, pubkeyFid);  /* Generate Key pair*/
//    if ((xResult == CKR_OK) && (!isToken))
//    {
//
//      /*
//        * Set attribute for the private key object
//      */
//      P11Object_t privateObjectToAdd;
//      privateObjectToAdd.xClass      = CKO_PRIVATE_KEY;
//      privateObjectToAdd.xDerive     = true;
//      (void)memcpy(&privateObjectToAdd.xLabel, &pxPrivateLabelAttribute.pValue, sizeof(CK_VOID_PTR));
//      privateObjectToAdd.xSign       = false;
//      privateObjectToAdd.xToken      = false;
//
//      /*
//        * Set attribute for the public key object
//      */
//      P11Object_t publicObjectToAdd;
//      publicObjectToAdd.xClass      = CKO_PUBLIC_KEY;
//      publicObjectToAdd.xDerive     = false;
//      (void)memcpy(&publicObjectToAdd.xLabel, &pxPublicLabelAttribute.pValue, sizeof(CK_VOID_PTR));
//      publicObjectToAdd.xSign       = false;
//      publicObjectToAdd.xToken      = false;
//      publicObjectToAdd.xVerify     = false;
//
//      /*
//        * add the private key handle and public key handle to list of know object
//      */
//
//      srand(generateRandom());
//      xResult = addObjectToList(privateObjectToAdd, pxPrivateKey); /* add object to list*/
//
//      if (xResult == CKR_OK)
//      {
//        xResult = addObjectToList(publicObjectToAdd, pxPublicKey); /* add object to list*/
//      }
//
//      if (xResult == CKR_OK)
//      {
//        pxSessionObj->ephemeralKeyGenerated = (CK_BBOOL)1;
//      }
//    }
//  }
//
//  return xResult;
//}

/**
  * @brief  C_GetInfo returns general information about Cryptoki.
  *
  * @param  pInfo     location that receives information
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetInfo)(CK_INFO_PTR pInfo)
{
  CK_RV rv = CKR_OK;
  if (pInfo != NULL)
  {
    if (PKCS11_MODULE_IS_INITIALIZED == CK_TRUE)
    {
      pInfo->cryptokiVersion.major = CK_VER_MAJ;
      pInfo->cryptokiVersion.minor = CK_VER_MIN;
      (void)memset(pInfo->manufacturerID, 0x00, sizeof(pInfo->manufacturerID));
      (void)memcpy(pInfo->manufacturerID, MANUFACTURER_ID, strlen(MANUFACTURER_ID));
      (void)memset(pInfo->libraryDescription, 0x00, sizeof(pInfo->libraryDescription));
      (void)memcpy(pInfo->libraryDescription, LIBRARYDESCRIPTION, strlen(LIBRARYDESCRIPTION));
      pInfo->libraryVersion.major = LIB_VER_MAJ;
      pInfo->libraryVersion.minor = LIB_VER_MIN;
      pInfo->flags = CKINFO_FLAGS;
    }
    else
    {
      rv =  CKR_CRYPTOKI_NOT_INITIALIZED;
    }
  }
  else
  {
    rv = CKR_ARGUMENTS_BAD;
  }
  return rv;
}

/*
  * Cryptoki provides the following functions for slot and token management.
*/

/**
  * @brief  C_GetSlotInfo obtains information about a particular slot in the system
  *
  * @param  slotID     the ID of the slot
  * @param  pInfo     receives the slot information
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_SLOT_ID_INVALID, CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED.
  *
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetSlotInfo)(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pSlotInfo)
{
  CK_RV rv = CKR_OK;
  /*
    * Check that slotID value is allowed
  */
  if (slotID == (uint32_t)1)
  {
    /*
      * Check that the PKCS #11 module is initialized
    */
    if (PKCS11_MODULE_IS_INITIALIZED == CK_TRUE)
    {
      if (pSlotInfo != NULL)
      {
        (void)memset(pSlotInfo->manufacturerID, 0x00, sizeof(pSlotInfo->manufacturerID));
        (void)memcpy(pSlotInfo->manufacturerID, "ST Microelectronics", 19);
        (void)memset(pSlotInfo->slotDescription, 0x00, sizeof(pSlotInfo->slotDescription));
        (void)memcpy(pSlotInfo->slotDescription, "STM32", 5);
        pSlotInfo->hardwareVersion.major = 0;
        pSlotInfo->hardwareVersion.minor = 0;
        pSlotInfo->firmwareVersion.major = 0;
        pSlotInfo->firmwareVersion.minor = 0;
        pSlotInfo->flags = CKF_TOKEN_PRESENT | CKF_HW_SLOT;
      }
      else
      {
        rv = CKR_ARGUMENTS_BAD;
      }
    }
    else
    {
      rv = CKR_CRYPTOKI_NOT_INITIALIZED;
    }
  }
  else
  {
    rv =  CKR_SLOT_ID_INVALID;
  }
  return rv;
}

/**
  * @brief  C_GetSlotList obtains a list of slots in the system.
  *
  * @param  tokenPresent     only slots with tokens
  * @param  pSlotList     receives array of slot IDs
  * @param  pulCount      receives number of slots
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_BUFFER_TOO_SMALL, CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetSlotList)(CK_BBOOL tokenPresent, CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR puCount)
{
  /*********************************************/
  /*
    * SETTING ONLY 1 SLOT
  */
  CK_ULONG m_Slots = 1;
  /*********************************************/
  CK_RV rv = CKR_OK;
  /*
    * Check that the PKCS #11 module is initialized
  */
  if (PKCS11_MODULE_IS_INITIALIZED == CK_TRUE)
  {
    if (puCount != NULL)
    {
      if (pSlotList != NULL)
      {
        CK_ULONG i;
        for (i = 0; i < ((m_Slots <= *puCount) ? m_Slots : *puCount); i++)
        {
          pSlotList[i] = i + (uint32_t)1;
        }
        *puCount = m_Slots;
        if (i < m_Slots)
        {
          rv =  CKR_BUFFER_TOO_SMALL;
        }
      }
      else
      {
        *puCount = m_Slots;
        rv =  CKR_OK;
      }
    }
    else
    {
      rv = CKR_ARGUMENTS_BAD;
    }
  }
  else
  {
    rv = CKR_CRYPTOKI_NOT_INITIALIZED;
  }

  return rv;

}

/**
  * @brief  C_GetTokenInfo obtains information about a particular token in the slot
  *
  * @param  slotID     the ID of the slot
  * @param  pInfo     receives the token information
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_SLOT_ID_INVALID, CKR_ARGUMENTS_BAD, CKR_CRYPTOKI_NOT_INITIALIZED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetTokenInfo)(CK_SLOT_ID slotID, CK_TOKEN_INFO_PTR pInfo)
{

  static char model[] = "ST33";
  static char manufacturerID[] = "ST Microelectronics";
  static char label[] = "JSign";
  CK_RV rv = CKR_OK;
  /*
    * Check that slotID value is allowed
  */
  if (slotID == (uint32_t)1)
  {
    /*
      * Check that the PKCS #11 module is initialized
    */
    if (PKCS11_MODULE_IS_INITIALIZED == CK_TRUE)
    {
      /*
        * Check that pInfo is not NULL
      */
      if (pInfo != NULL)
      {
        (void)memset(pInfo, 0, sizeof(pInfo[0]));
        (void)memset(pInfo->label, 0x00, sizeof(pInfo->label));
        (void)memset(pInfo->manufacturerID, 0x00, sizeof(pInfo->manufacturerID));
        (void)memset(pInfo->model, 0x00, sizeof(pInfo->model));
        (void)memset(pInfo->serialNumber, 0x00, sizeof(pInfo->serialNumber));
        (void)memcpy(pInfo->label, label, strlen(label));
        (void)memcpy(pInfo->manufacturerID, manufacturerID, strlen(manufacturerID));
        (void)memcpy(pInfo->model, model, strlen(model));
        (void)memset(pInfo->serialNumber, 0x00, sizeof(pInfo->serialNumber));
        pInfo->flags = CKF_RNG | CKF_TOKEN_INITIALIZED;
        pInfo->ulMaxSessionCount = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulSessionCount = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulRwSessionCount = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulMaxRwSessionCount = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulMaxPinLen = 0;
        pInfo->ulMinPinLen = 0;
        pInfo->ulTotalPrivateMemory = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulFreePrivateMemory = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulFreePrivateMemory = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulTotalPublicMemory = CK_UNAVAILABLE_INFORMATION;
        pInfo->ulFreePublicMemory = CK_UNAVAILABLE_INFORMATION;
        pInfo->hardwareVersion.major = 1;
        pInfo->hardwareVersion.minor = 1;
        pInfo->firmwareVersion.major = 1;
        pInfo->firmwareVersion.minor = 0; /* for library version 0.3.0, relevant
                                            current firmware version is 1.02 */
      }
      else
      {
        rv = CKR_ARGUMENTS_BAD;
      }
    }
    else
    {
      rv = CKR_CRYPTOKI_NOT_INITIALIZED;
    }
  }
  else
  {
    rv = CKR_SLOT_ID_INVALID;
  }
  return rv;
}

/**
  * @brief  C_GetMechanismList obtain a list of mechanism types supported by a token
  *
  * @param  slotID        the ID of the slot
  * @param  pMechanismList   receives mechanism types array
  * @param  pCount        points to the location that receives the number of mechanisms.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_BUFFER_TOO_SMALL, CKR_SLOT_ID_INVALID, CKR_ARGUMENTS_BAD,
  *         CKR_CRYPTOKI_NOT_INITIALIZED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetMechanismList)(

  CK_SLOT_ID            slotID,
  CK_MECHANISM_TYPE_PTR pMechanismList,
  CK_ULONG_PTR          pCount
)
{
  CK_RV rv = CKR_OK;
  /*
    * Check that the PKCS #11 module is initialized
  */
  if (PKCS11_MODULE_IS_INITIALIZED == CK_TRUE)
  {
    if (pCount != NULL_PTR)
    {
      if (slotID == (uint32_t)1)
      {
        MECHANISM_PTR_t pMech = m_MechList;
        pMech->type = CKM_EC_KEY_PAIR_GEN;
        pMech->info.ulMinKeySize = KEY_SIZE;
        pMech->info.ulMaxKeySize = KEY_SIZE;
        pMech->info.flags = CKF_HW | CKF_GENERATE_KEY_PAIR;
        pMech++;
        pMech->type = CKM_ECDSA;
        pMech->info.ulMinKeySize = KEY_SIZE;
        pMech->info.ulMaxKeySize = KEY_SIZE;
        pMech->info.flags = CKF_HW | CKF_SIGN | CKF_VERIFY;
        pMech++;
        pMech->type = CKM_ECDH1_DERIVE;
        pMech->info.ulMinKeySize = KEY_SIZE;
        pMech->info.ulMaxKeySize = KEY_SIZE;
        pMech->info.flags = CKF_HW | CKF_DERIVE;
        pMech++;
        pMech->type = CKM_HKDF_DERIVE;
        pMech->info.ulMinKeySize = KEY_SIZE;
        pMech->info.ulMaxKeySize = KEY_SIZE;
        pMech->info.flags = CKF_HW | CKF_DERIVE;
        pMech++;
        pMech->type = CKM_ECDH1_COFACTOR_DERIVE;
        pMech->info.ulMinKeySize = KEY_SIZE;
        pMech->info.ulMaxKeySize = KEY_SIZE;
        pMech->info.flags = CKF_HW | CKF_DERIVE;
        if (pMechanismList != NULL_PTR)
        {
          if (*pCount < (uint32_t)MECH_NUMBER)
          {
            rv = CKR_BUFFER_TOO_SMALL;
          }
          else
          {
            for (CK_ULONG i = (uint32_t)0; i < (uint32_t)MECH_NUMBER; i++)
            {
              pMechanismList[i] = m_MechList[i].type;
            }
          }
        }
        else
        {
          rv = CKR_OK;
        }
        /*
          * If pMechanismList is NULL_PTR, then all that C_GetMechanismList does is return
          * (in *pCount) the number of mechanisms, without actually returning a list of
          * mechanisms.
        */
        *pCount = MECH_NUMBER;
      }
      else
      {
        rv = CKR_SLOT_ID_INVALID;
      }
    }
    else
    {
      rv = CKR_ARGUMENTS_BAD;
    }
  }
  else
  {
    rv  = CKR_CRYPTOKI_NOT_INITIALIZED;
  }

  return rv;
}

/**
  * @brief  C_GetMechanismInfo obtains information about a particular mechanism possibly supported by a token
  *
  * @param  slotID        the ID of the slot
  * @param  type          type of mechanism
  * @param  pCount        points to the location that receives the mechanism information.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_MECHANISM_INVALID, CKR_SLOT_ID_INVALID, CKR_ARGUMENTS_BAD,
  *         CKR_CRYPTOKI_NOT_INITIALIZED.
  */
CK_DECLARE_FUNCTION(CK_RV, C_GetMechanismInfo)(

  CK_SLOT_ID            slotID,
  CK_MECHANISM_TYPE     type,
  CK_MECHANISM_INFO_PTR pInfo
)
{

  CK_RV rv = CKR_MECHANISM_INVALID;
  /*
    * Check that the PKCS #11 module is initialized
  */
  if (PKCS11_MODULE_IS_INITIALIZED == CK_TRUE)
  {
    if (pInfo != NULL_PTR)
    {
      if (slotID == (uint32_t)1)
      {
        for (CK_ULONG i = (uint32_t)0; i < (uint32_t)MECH_NUMBER; i++)
        {
          if (type == m_MechList[i].type)
          {
            pInfo->ulMinKeySize = m_MechList[i].info.ulMinKeySize;
            pInfo->ulMaxKeySize = m_MechList[i].info.ulMaxKeySize;
            pInfo->flags = m_MechList[i].info.flags;
            rv = CKR_OK;
            break;
          }
        }
      }
      else
      {
        rv =  CKR_SLOT_ID_INVALID;
      }
    }
    else
    {
      rv =  CKR_ARGUMENTS_BAD;
    }
  }
  else
  {
    rv = CKR_CRYPTOKI_NOT_INITIALIZED;
  }
  return rv;
}

/**
  * @brief  C_DestroyObject destroys an object.
  *
  * @param  xSession      Handle of a valid PKCS #11 session.
  * @param  hObject       The object's handle.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ATTRIBUTE_VALUE_INVALID, CKR_OBJECT_HANDLE_INVALID, CKR_ACTION_PROHIBITED,
  *         CKR_FUNCTION_FAILED, CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_IN,VALID,
  *         CKR_SESSION_CLOSED, Vendor defined error.
  */

//CK_DECLARE_FUNCTION(CK_RV, C_DestroyObject)(
//  CK_SESSION_HANDLE xSession,  /* the session's handle */
//  CK_OBJECT_HANDLE  hObject    /* the object's handle */
//)
//{
//  P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
//  P11Object_t p11Obj;
//  if (xResult == CKR_OK)
//  {
//    findObjectInListByHandle(hObject, &xHandle, &p11Obj);  /* find object by handle*/
//    if (xHandle == CK_INVALID_HANDLE)
//    {
//      xResult = CKR_OBJECT_HANDLE_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (p11Obj.xClass != CKO_CERTIFICATE)
//    {
//      xResult = CKR_ACTION_PROHIBITED;
//    }
//    else
//    {
//      if (setEnvironmentTarget(pxSessionObj, p11Obj.xLabel) == false)
//      {
//        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    /*
//      * Set the legth of certificate to 0
//    */
//    xResult = DeleteCertificateValue(pxSessionObj);
//  }
//
//  if (xResult == CKR_OK)
//  {
//    /*
//      * Set the handle of token object to CK_INVALID_HANDLE
//    */
//    xResult = removeObjectFromList(&hObject);
//  }
//
//  return xResult;
//}

/**
  * Cryptoki provides the following functions for managing objects.
  * @brief  C_CreateObject creates a new object.
  *
  * @param  xSession              Handle of a valid PKCS #11 session.
  * @param  pTemplate             Points to the object�s template.
  * @param  ulCount               Is the number of attributes in the template
  * @param  phObject              Points to the location that receives the new object�s handle.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_ATTRIBUTE_VALUE_INVALID, CKR_TEMPLATE_INCOMPLETE,
  *         CKR_FUNCTION_FAILED, CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED, Vendor defined error.
  */

CK_DECLARE_FUNCTION(CK_RV, C_CreateObject)(
  CK_SESSION_HANDLE xSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount,
  CK_OBJECT_HANDLE_PTR phObject
)
{
  /* int32_t ret = 0; */
  int32_t lTmpPos;
  CK_ULONG labelSize = 0;
  CK_ULONG valueSize = 0;
  CK_ULONG  pulPosition = 0;
  CK_BBOOL *isToken;
  CK_ULONG *keyType;
  P11Object_t p11Obj;
  CK_BYTE *pbLabel =  __NULL;
  CK_BYTE *pbID =  __NULL;
  CK_ULONG *pObjClass = __NULL;
  CK_BYTE *pbValue  = __NULL;
  CK_BYTE *pclassValue = __NULL;
  CK_ULONG classValue;

  P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if ((phObject == NULL_PTR) || (pTemplate == NULL) || (ulCount == (uint32_t)0))
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  if (xResult == CKR_OK)
  {
/*    lTmpPos = GetAttributePos(CKA_TOKEN, pTemplate, ulCount);
    if (lTmpPos >= 0)
    {
      isToken = (CK_BBOOL *)pTemplate[lTmpPos].pValue;
      if ((*isToken) == FALSE)
      {
        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
      }
    }
*/
//    if (xResult == CKR_OK)
//    {
      /*
        * CKA_CLASS is Mandatory
      */
      pclassValue =  GetAttribute(CKA_CLASS, pTemplate, ulCount, NULL, &pulPosition);
      if (pclassValue == __NULL)
      {
        xResult = CKR_TEMPLATE_INCOMPLETE;
      }
      else
      {
        classValue = (CK_ULONG)(*pclassValue);
        pObjClass = &classValue;
      }
//    }

    if (xResult == CKR_OK)
    {
      if (*pObjClass == CKO_CERTIFICATE)
      {
        /*
          * CKA_LABEL is Mandatory
        */
        pbLabel = GetAttribute(CKA_LABEL, pTemplate, ulCount, &labelSize, &pulPosition);
        if (pbLabel == __NULL)
        {
          xResult = CKR_TEMPLATE_INCOMPLETE;
        }

//        if (xResult == CKR_OK)
//        {
//          /*
//           * Allowed Values for Certificates are MAIN_CERTIFICATE,
//           */
//          if (setEnvironmentTarget(pxSessionObj, (int8_t *)pbLabel) == false)
//          {
//            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//          }
//        }
        if (xResult == CKR_OK)
        {
          /*
            * CKA_VALUE is Mandatory
          */
          pbValue = GetAttribute(CKA_VALUE, pTemplate, ulCount, &valueSize, &pulPosition);
          if (pbValue == __NULL)
          {
            xResult = CKR_TEMPLATE_INCOMPLETE;
          }
        }

        /*
           ret = checkCertStructure(pbValue, valueSize);
           if(ret < 0)
           {
             return CKR_ATTRIBUTE_VALUE_INVALID;
           }
         */
        if (xResult == CKR_OK)
        {
//          xResult = WriteCertificateValue(pxSessionObj, pbValue, valueSize);  /* Write certificate value*/
          xResult = UpdateFileValue(pxSessionObj, CERTIFICATE_FILE_TARGET, pbValue, valueSize);  /* Write certificate value*/
          if(xResult == CKR_OK){
        	  p11Obj.xClass = CKO_CERTIFICATE;
        	  p11Obj.xLabel = MAIN_CERTIFICATE;
        	  p11Obj.xDerive = FALSE;
          }
        }
      }
      else  if (*pObjClass == CKO_SECRET_KEY)
      {
    	/*
    	* CKA_LABEL is Mandatory, only MAIN_SECRET_KEY is allowed
    	*/
    	  pbLabel = GetAttribute(CKA_LABEL, pTemplate, ulCount, &labelSize, &pulPosition);
    	  if (pbLabel == __NULL)
    	  {
    		  xResult = CKR_TEMPLATE_INCOMPLETE;
    	  }
    	  if (strcmp((CCHAR *)pbLabel, MAIN_SECRET_KEY) != 0){
    		  xResult = CKR_ATTRIBUTE_VALUE_INVALID;
    	  }
    	  if (xResult == CKR_OK)
    	  {
    		  /*
    		   * CKA_VALUE is Mandatory
    		   */
    		  pbValue = GetAttribute(CKA_VALUE, pTemplate, ulCount, &valueSize, &pulPosition);
    		  if (pbValue == __NULL)
    		  {
    			  xResult = CKR_TEMPLATE_INCOMPLETE;
    		  }
    	  }
    	  if (xResult == CKR_OK)
    	  {
    		  if (valueSize != (uint32_t)SECRET_KEY_LENGTH)
    		  {
    			  xResult = CKR_ARGUMENTS_BAD;
    		  }
    	  }
    	  if (xResult == CKR_OK)
    	  {
    		  xResult = UpdateKey(pbValue, (CK_BYTE)valueSize);  /* Update key value*/
    		  if(xResult == CKR_OK){
    			  p11Obj.xClass = CKO_SECRET_KEY;
    			  p11Obj.xLabel = MAIN_SECRET_KEY;
    			  p11Obj.xDerive = TRUE;
    		  }
    	  }
      }else  if (*pObjClass == CKO_DATA)
      {
    	  /*
    	   * CKA_LABEL is Mandatory
    	   */
    	  pbLabel = GetAttribute(CKA_LABEL, pTemplate, ulCount, &labelSize, &pulPosition);
    	  if (pbLabel == __NULL)
    	  {
    		  xResult = CKR_TEMPLATE_INCOMPLETE;
    	  }
    	  if (strcmp((CCHAR *)pbLabel, DATA_OBJECT) != 0){
    		  xResult = CKR_ATTRIBUTE_VALUE_INVALID;
    	  }
    	  if (xResult == CKR_OK)
    	  {
    		  /*
    		   * CKA_VALUE is Mandatory
    		   */
    		  pbValue = GetAttribute(CKA_VALUE, pTemplate, ulCount, &valueSize, &pulPosition);
    		  if (pbValue == __NULL)
    		  {
    			  xResult = CKR_TEMPLATE_INCOMPLETE;
    		  }
    	  }

    	  if (xResult == CKR_OK)
    	  {
    		  xResult = UpdateFileValue(pxSessionObj, URL_FILE_TARGET, pbValue, valueSize);  /* Write DATA OBJECT value*/
    		  if(xResult == CKR_OK){
    			  p11Obj.xClass = CKO_DATA;
    			  p11Obj.xLabel = DATA_OBJECT;
    			  p11Obj.xDerive = FALSE;
    		  }
    	  }
      }

      else
      {
        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
      }
    }
    if (xResult == CKR_OK)
    {
      /* findObjectInListByLabel((char *)pbLabel, &p11Obj); */
      srand(generateRandom());
      /*
        * add the object handle to list of know object
      */
      //xResult = addObjectToList(p11Obj, phObject);
      updateHandleForObject((int8_t *)pbLabel, labelSize, &p11Obj);
      if (p11Obj.xHandle != (uint32_t)0)
      {
        *phObject = p11Obj.xHandle;
      }
      else
      {
        xResult = CKR_FUNCTION_FAILED;
      }
    }
  }

  return xResult;
}

/**
  * @brief  C_DeriveKey derives a key from a base key, creating a new key object.
  *
  * @param  xSession              Handle of a valid PKCS #11 session.
  * @param  pMechanism            Points to a structure that specifies the key derivation mechanism.
  * @param  hBaseKey              Is the handle of the base key.
  * @param  pTemplate             Points to the template for the new key.
  * @param  ulAttributeCount      Is the number of attributes in the template
  * @param  phKey                 Points to the location that receives the handle of the derived key.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_MECHANISM_INVALID, CKR_MECHANISM_PARAM_INVALID
  *         CKR_KEY_HANDLE_INVALID, CKR_KEY_TYPE_INCONSISTENT, CKR_ATTRIBUTE_VALUE_INVALID,
  *         CKR_TEMPLATE_INCOMPLETE, CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED, Vendor defined error.
  */
//CK_DECLARE_FUNCTION(CK_RV, C_DeriveKey)
//(
//  CK_SESSION_HANDLE    xSession,
//  CK_MECHANISM_PTR     pMechanism,
//  CK_OBJECT_HANDLE     hBaseKey,
//  CK_ATTRIBUTE_PTR     pTemplate,
//  CK_ULONG             ulAttributeCount,
//  CK_OBJECT_HANDLE_PTR phKey
//)
//{
//  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
//  P11Object_t p11Obj;
//  CK_ULONG_PTR cls;
//  CK_ULONG_PTR pKeyType;
//  CK_ULONG pValueLen;
//  CK_BBOOL *isToken;
//  CK_BYTE pSecret[33];
//  int32_t lTmpPos;
//  CK_ECDH1_DERIVE_PARAMS  params;
//  static CK_BYTE defaultLabelSecret[]    = "EphSecretK";
//
//  MECHANISM_t Mechanism = { pMechanism->mechanism, 0, 0, CKF_DERIVE};
//
//  P11Object_t secretToAdd;
//
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  (void)memset(pSecret, 0, 33);
//
//  /*********************************************/
//  /*                CHECKS                     */
//  /*********************************************/
//  if (xResult == CKR_OK)
//  {
//    if ((phKey == __NULL) || (pMechanism == __NULL) || (pTemplate == __NULL) || (ulAttributeCount == (uint32_t)0))
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if ((Mechanism.type != CKM_ECDH1_DERIVE) && (Mechanism.type != CKM_ECDH1_COFACTOR_DERIVE))
//    {
//      xResult = CKR_MECHANISM_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if ((Mechanism.info.flags & CKF_DERIVE) == (uint32_t)0)
//    {
//      xResult = CKR_MECHANISM_PARAM_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    findObjectInListByHandle(hBaseKey, &xHandle, &p11Obj);
//    if (xHandle == CK_INVALID_HANDLE)
//    {
//      xResult = CKR_KEY_HANDLE_INVALID;
//    }
//  }
//
//  /*
//    * The base hKey must have:
//    * CKA_CLASS = CKO_PRIVATE_KEY,
//    * CKA_TOKEN = FALSE,
//    * CKA_DERIVE = TRUE
//  */
//  if (xResult == CKR_OK)
//  {
//    if (p11Obj.xClass != CKO_PRIVATE_KEY)
//    {
//      xResult = CKR_KEY_TYPE_INCONSISTENT;
//    }
//  }
//  if (xResult == CKR_OK)
//  {
//    if ((p11Obj.xToken != (CK_BYTE)false) || (p11Obj.xDerive == (CK_BYTE)false))
//    {
//      xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    /*
//      * Get public key value from mechnism paramter
//    */
//    xResult = ParseECParam(pMechanism->pParameter, &params);
//  }
//
//  /*
//    * CKA_CLASS must be present in the Template, it must be CKO_SECRET_KEY
//  */
//  if (xResult == CKR_OK)
//  {
//    lTmpPos = GetAttributePos(CKA_CLASS, pTemplate, ulAttributeCount);
//    if (lTmpPos >= 0)
//    {
//      cls = (CK_ULONG_PTR)pTemplate[lTmpPos].pValue;
//      if (*cls != CKO_SECRET_KEY)
//      {
//        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//    }
//    else
//    {
//      xResult = CKR_TEMPLATE_INCOMPLETE;
//    }
//  }
//
//  /*
//    * CKA_KEY_TYPE must be present in the Template, it must be CKK_GENERIC_SECRET
//  */
//  if (xResult == CKR_OK)
//  {
//    lTmpPos = GetAttributePos(CKA_KEY_TYPE, pTemplate, ulAttributeCount);
//    if (lTmpPos >= 0)
//    {
//      pKeyType = (CK_ULONG_PTR)pTemplate[lTmpPos].pValue;
//      if (*pKeyType != CKK_GENERIC_SECRET)
//      {
//        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//    }
//    else
//    {
//      xResult = CKR_TEMPLATE_INCOMPLETE;
//    }
//  }
//
//  /*
//    * CKA_VALUE_LEN must be present in the Template, it must be 32
//  */
//  if (xResult == CKR_OK)
//  {
//    lTmpPos = GetAttributePos(CKA_VALUE_LEN, pTemplate, ulAttributeCount);
//    if (lTmpPos >= 0)
//    {
//      pValueLen = *(CK_ULONG *)pTemplate[lTmpPos].pValue;
//      if (pValueLen != (uint32_t)32)
//      {
//        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//    }
//    else
//    {
//      xResult = CKR_TEMPLATE_INCOMPLETE;
//    }
//  }
//
//  /*
//    * if CKA_TOKEN is present in the Template, it must be FALSE
//  */
//  if (xResult == CKR_OK)
//  {
//    lTmpPos = GetAttributePos(CKA_TOKEN, pTemplate, ulAttributeCount);
//    if (lTmpPos >= 0)
//    {
//      isToken = (CK_BBOOL *)pTemplate[lTmpPos].pValue;
//      if (*isToken == TRUE)
//      {
//        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//    }
//  }
//  /************** END OF CHECKS ***************************************/
//  /* P11Object_t p11Obj; */
//  if (xResult == CKR_OK)
//  {
//    CK_ULONG labelSize = 0;
//    CK_ULONG pulPosition = 0;
//    char pbLabel[MAX_LABEL_LENGTH + 1];
//
//    (void)memset(pbLabel, 0, MAX_LABEL_LENGTH + 1);
//
//    secretToAdd.xLabel = (int8_t *)&pbLabel[0];
//    secretToAdd.xValue = &pSecret[0];
//    (void)memset(secretLabel, 0x00, MAX_LABEL_LENGTH);
//    CK_BYTE *labelDerive = GetAttribute(CKA_LABEL, pTemplate, ulAttributeCount, &labelSize, &pulPosition);
//    if (labelDerive != __NULL)
//    {
//      /*
//        * Check that an Object with such given Label is not already existing
//        */
//      findObjectInListByLabel(pTemplate[pulPosition].pValue, &p11Obj);
//      if (p11Obj.xHandle != (CK_ULONG)NULL)
//      {
//        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
//      }
//      else
//      {
//        (void)memcpy(secretLabel, labelDerive, labelSize);
//      }
//    }
//    else
//    {
//
//      (void)memcpy(secretLabel, defaultLabelSecret, sizeof(defaultLabelSecret));
//
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    (void)memset(secretValue, 0x00, SECRET_KEY_LENGTH);
//    xResult = GenerateSecret(params, secretValue, (int32_t *)(&secretToAdd.xValueLen));
//  }
//
//  if (xResult == CKR_OK)
//  {
//    /*
//      * Set attribute of the secret key
//      */
//    secretToAdd.xClass      = CKO_SECRET_KEY;
//    secretToAdd.xLabel      = (int8_t *)secretLabel;
//    secretToAdd.xDerive     = false;
//    secretToAdd.xSign       = false;
//    secretToAdd.xToken      = false;
//    secretToAdd.xValue      = secretValue;
//    srand(generateRandom());
//    /*
//      * add the private key handle and public key handle to list of know object
//      */
//    xResult = addObjectToList(secretToAdd, phKey);
//    /* xP11session.ephemeralSecretKeyGenerated = CK_FALSE; */
//
//  }
//  else if (xResult == CUSTOM_FUNCTION_FAILED)
//  {
//    xResult = CKR_FUNCTION_FAILED;
//  }
//  else
//  {
//    /*****************/
//  }
//  xP11session.ephemeralKeyGenerated = (CK_BBOOL) CK_FALSE;
//  /*
//    * Set the handle of token object to CK_INVALID_HANDLE
//  */
//
//  removeSessionKeyPairsFromList();
//
//  return xResult;
//}

CK_DECLARE_FUNCTION(CK_RV, C_DeriveKey)
(
  CK_SESSION_HANDLE    xSession,
  CK_MECHANISM_PTR     pMechanism,
  CK_OBJECT_HANDLE     hBaseKey,
  CK_ATTRIBUTE_PTR     pTemplate,
  CK_ULONG             ulAttributeCount,
  CK_OBJECT_HANDLE_PTR phKey
)
{
  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
  P11Object_t p11Obj;
  CK_ULONG_PTR cls;
  CK_ULONG_PTR pKeyType;
  CK_ULONG pValueLen;
  CK_BBOOL *isToken;
  CK_BYTE pSecret[33];
  int32_t lTmpPos;
  CK_ECDH1_DERIVE_PARAMS  params;
  CK_HKDF_PARAMS  parameters;
  CK_BYTE *labelDerive = NULL;
  char * pSecretLabel = &secretLabel[0];

  CK_ULONG labelSize	= 0;
  CK_ULONG pulPosition	= 0;

  MECHANISM_t Mechanism = { pMechanism->mechanism, 0, 0, CKF_DERIVE};

  P11Object_t secretToAdd;
  int8_t appo[MAX_LABEL_LENGTH];
  secretToAdd.xLabel = &appo;

  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  (void)memset(pSecret, 0, 33);

  /*********************************************/
  /*                CHECKS                     */
  /*********************************************/
  if (xResult == CKR_OK)
  {
    if ((phKey == __NULL) || (pMechanism == __NULL) || (pTemplate == __NULL) || (ulAttributeCount == (uint32_t)0))
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
  }

  if (xResult == CKR_OK)
  {
    if ((Mechanism.type != CKM_HKDF_DERIVE))
    {
      xResult = CKR_MECHANISM_INVALID;
    }
  }

  if (xResult == CKR_OK)
  {
    if ((Mechanism.info.flags & CKF_DERIVE) == (uint32_t)0)
    {
      xResult = CKR_MECHANISM_PARAM_INVALID;
    }
  }

  if (xResult == CKR_OK)
  {
    findObjectInListByHandle(hBaseKey, &xHandle, &p11Obj);
    if (xHandle == CK_INVALID_HANDLE)
    {
      xResult = CKR_KEY_HANDLE_INVALID;
    }
  }

  /*
    * The base hKey must have:
    * CKA_CLASS = CKO_SECRET_KEY,
    * CKA_DERIVE = TRUE
  */
  if (xResult == CKR_OK)
  {
	if (p11Obj.xClass == (CK_ULONG)NULL)
	{
		p11Obj.xClass = CKO_SECRET_KEY;
	}
	else if (p11Obj.xClass != CKO_SECRET_KEY)
    {
      xResult = CKR_KEY_TYPE_INCONSISTENT;
    }
  }

  if (xResult == CKR_OK)
  {
    if ((p11Obj.xDerive == (CK_BYTE)false))
    {
      xResult = CKR_ATTRIBUTE_VALUE_INVALID;
    }
  }

  if (xResult == CKR_OK)
  {
    /*
      * Get public key value from mechnism paramter
    */
    xResult = ParseHKDFParam(pMechanism->pParameter, &parameters);
  }

  /*
    * CKA_CLASS must be present in the Template, it must be CKO_SECRET_KEY
  */
  if (xResult == CKR_OK)
  {
    lTmpPos = GetAttributePos(CKA_CLASS, pTemplate, ulAttributeCount);
    if (lTmpPos >= 0)
    {
      cls = (CK_ULONG_PTR)pTemplate[lTmpPos].pValue;
      if (*cls != CKO_SECRET_KEY)
      {
        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
      }
    }
    else
    {
      xResult = CKR_TEMPLATE_INCOMPLETE;
    }
  }

  /*
   * CKA_KEY_TYPE must be present in the Template, it must be CKK_GENERIC_SECRET or CKK_HKDF
   */
  if (xResult == CKR_OK)
  {
    lTmpPos = GetAttributePos(CKA_KEY_TYPE, pTemplate, ulAttributeCount);
    if (lTmpPos >= 0)
    {
      pKeyType = (CK_ULONG_PTR)pTemplate[lTmpPos].pValue;
      if ((*pKeyType != CKK_GENERIC_SECRET) && (*pKeyType != CKK_HKDF))
      {
        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
      }
    }
    else
    {
      xResult = CKR_TEMPLATE_INCOMPLETE;
    }
  }

  if (xResult == CKR_OK)
  {
	  (void)memset(pSecretLabel, 0x00, MAX_LABEL_LENGTH);
	  pSecretLabel = GetAttribute(CKA_LABEL, pTemplate, ulAttributeCount, &labelSize, &pulPosition);

	  /* 1. CKA_LABEL must be present in the template */
	  if (pSecretLabel == __NULL)
	  {
		  xResult = CKR_TEMPLATE_INCOMPLETE;
	  }

	  /* 2. CKA_LABEL in the Template cannot be equal to p11Object.xLabel */
	  if (xResult == CKR_OK)
	  {
		  if (strcmp((CCHAR *)p11Obj.xLabel, (CCHAR *)pSecretLabel) == 0)
		  {
			  xResult = CKR_ATTRIBUTE_VALUE_INVALID;
		  }
	  }

	  /* 3. If CKA_LABEL in the Template is MSK then p11Object.xLabel must be PSK */
	  if (xResult == CKR_OK)
	  {
	      if (strcmp((CCHAR *)p11Obj.xLabel, MAIN_SECRET_KEY) == 0)
	      {
	    	  if (strcmp((CCHAR *)pSecretLabel, FIRST_DERIVED_KEY) != 0)
	    	  {
	    		  xResult = CKR_ATTRIBUTE_VALUE_INVALID;
	    	  }
	      }
	  }
  }
  /************** END OF CHECKS ***************************************/

  if (xResult == CKR_OK)
  {
	  (void)memcpy(secretToAdd.xLabel, pSecretLabel, MAX_LABEL_LENGTH); // cambiare con la lunghezza della label pSecretLabel
	  secretToAdd.xValue = &pSecret[0];
	  (void)memset(secretValue, 0x00, SECRET_KEY_LENGTH);
	  xResult = DeriveSecret(parameters, p11Obj.xLabel, pSecretLabel, secretValue, (int32_t *)(&secretToAdd.xValueLen));
  }

  if (xResult == CKR_OK)
  {
    /*
      * Set attribute of the secret key
      */
    secretToAdd.xClass      = CKO_SECRET_KEY;
    secretToAdd.xLabel      = (int8_t *)pSecretLabel;
    secretToAdd.xDerive     = true;
    secretToAdd.xSign       = true;
    secretToAdd.xToken      = true;
    srand(generateRandom());

    findObjectInListByLabel(secretToAdd.xLabel, &p11Obj);
    if (p11Obj.xHandle == CK_INVALID_HANDLE)
    {
    	updateHandleForObject(secretToAdd.xLabel, strlen(secretToAdd.xLabel), &p11Obj);
    	//xResult = addObjectToList(secretToAdd, phKey);
    }
  }
  else if (xResult == CUSTOM_FUNCTION_FAILED)
  {
    xResult = CKR_FUNCTION_FAILED;
  }
  else
  {
    /*****************/
  }

  return xResult;
}

/*
  *Cryptoki provides the following functions for encrypting data.
  */

/**
  * @brief Begin creating a data ciphered.
  * After calling C_EncryptInit, the application can either call C_Encrypt to cipher in a single part.
  *
  * @param[in] xSession                      Handle of a valid PKCS #11 session.
  * @param[in] pxMechanism                   Mechanism used to sign.
  *                                          This port supports the following mechanisms:
  *                                           - CKM_AES_CBC for AES Encrypt
  * @param[in] xKey                          The handle of the private key to be used for
  *                                          encrypt. Key must be compatible with the
  *                                          mechanism chosen by pxMechanism.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OPERATION_ACTIVE, CKR_KEY_HANDLE_INVALID,
  *         CKR_KEY_TYPE_INCONSISTENT, CKR_MECHANISM_INVALID, CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED.
  */

CK_DECLARE_FUNCTION(CK_RV, C_EncryptInit)(CK_SESSION_HANDLE xSession,
                                       CK_MECHANISM_PTR pxMechanism,
                                       CK_OBJECT_HANDLE xKey)
{
  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
  P11Object_t p11Obj;

  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if (NULL == pxMechanism)
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
    else if ((CK_ULONG)NULL != pxSession->EncDecMechanism)
    {
      xResult = CKR_OPERATION_ACTIVE;
    }
    else
    {
      findObjectInListByHandle(xKey, &xHandle, &p11Obj);
    }
  }

  /*
    * Check the validity of Key handle
  */
  if (xResult == CKR_OK)
  {
    if (xHandle == CK_INVALID_HANDLE)
    {
      xResult = CKR_KEY_HANDLE_INVALID;
    }
  }

  /*
    * Check that a secret key was retrieved.
  */
  if (xResult == CKR_OK)
  {
    if (p11Obj.xClass != CKO_SECRET_KEY)
    {
      xResult = CKR_KEY_TYPE_INCONSISTENT;
    }
  }


  if (xResult == CKR_OK)
  {
	  pxSession->hEncDecKey = xHandle;
	  if (strcmp((CCHAR *)p11Obj.xLabel, ENCDEC_DERIVED_KEY) == 0)
	  {
		  pxSession->targetKeyParameter = (CK_BYTE)0x10;
	  }
	  else
	  {
		  xResult = CKR_KEY_HANDLE_INVALID;
	  }
  }


  /*
    * Check that the mechanism is compatible, supported.
    * If the mechanism is NULL it's considered as CKM_AES_CBC
  */
  if (xResult == CKR_OK)
  {
	  if (pxMechanism->mechanism == (CK_ULONG)NULL)
	  {
		  pxMechanism->mechanism = CKM_AES_CBC;
	  }

      if (pxMechanism->mechanism != CKM_AES_CBC)
	  {
		  xResult = CKR_MECHANISM_INVALID;
	  }
  }

  if (xResult == CKR_OK)
  {
    pxSession->m_EncryptInitialized = (CK_BBOOL)1;
    pxSession->EncDecMechanism = pxMechanism->mechanism;
  }

  return xResult;
}

/**
  * @brief Performs a data enrypting operation.
  * The encrypting operation must have been initialized with C_EncryptInit.
  *
  * @param[in] xSession                      Handle of a valid PKCS #11 session.
  * @param[in] pData                         the plaintext data.
  * @param[in] ulDataLen                     Length of pData, in bytes.
  * @param[out] pEncryptedData               Buffer where receives encrypted data.
  * @param[out] puEncryptedDataLen           Length of pEncryptedData buffer.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_BUFFER_TOO_SMALL, CKR_OPERATION_NOT_INITIALIZED,
  *         CKR_DATA_LEN_RANGE ,CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED, Vendor defined error.
  */
CK_DECLARE_FUNCTION(CK_RV, C_Encrypt)(CK_SESSION_HANDLE xSession,
                                     CK_BYTE_PTR pData,
                                     CK_ULONG uDataLen,
                                     CK_BYTE_PTR pEncryptedData,
                                     CK_ULONG_PTR puEncryptedDataLen)
  {
    CK_ULONG xExpectedInputLength = 0;
    CK_BYTE_PTR pxCipherBuffer = pEncryptedData;
    uint8_t chiper[64];

    P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
    /*
        * Check that the PKCS #11 module is initialized,
        * if the session is open and valid.
    */
    CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

    if (xResult == CKR_OK)
    {
      if (!pxSessionObj->m_EncryptInitialized)
      {
        xResult = CKR_OPERATION_NOT_INITIALIZED;
      }
    }

    if (xResult == CKR_OK)
    {
      if ((uDataLen == (uint32_t)0) || (NULL == pData) || (NULL == puEncryptedDataLen))
      {
        xResult = CKR_ARGUMENTS_BAD;
      }
    }


    if (xResult == CKR_OK)
    {
    	xExpectedInputLength = 0x00;/*da definire la len dei dati in ingresso*/
    	pxCipherBuffer = chiper;

    	if (*puEncryptedDataLen < 64) /*da definire la len dei dati in uscita*/
    	{
    		xResult = CKR_BUFFER_TOO_SMALL;
    	}
    }
    if (xResult == CKR_OK)
    {
    	/*
    	 * Check that input data to be signed is the expected length.
    	 */
    	if (xExpectedInputLength != uDataLen)
    	{
    		xResult = CKR_DATA_LEN_RANGE;
    	}
    }
    /*
     * Sign the data.
     */
    if (xResult == CKR_OK)
    {
    	xResult = encrypt(pxSessionObj, pData,
    			(CK_BYTE)uDataLen,
				pxCipherBuffer,
				puEncryptedDataLen);
    	if (xResult == (uint32_t)0)
    	{
    		(void)memcpy(pEncryptedData, pxCipherBuffer, *puEncryptedDataLen);
    	}
    }
    /*
     * Complete the operation in the context.
     */
    pxSessionObj->EncDecMechanism = (CK_ULONG)NULL;
    pxSessionObj->targetKeyParameter = 0x00;
    pxSessionObj->m_EncryptInitialized = (CK_BBOOL)0;

    return xResult;
  }

/*
  *Cryptoki provides the following functions for decrypting data.
  */

/**
  * @brief Begin creating a data deciphered.
  * After calling C_DecryptInit, the application can either call C_Decrypt to decipher in a single part.
  *
  * @param[in] xSession                      Handle of a valid PKCS #11 session.
  * @param[in] pxMechanism                   Mechanism used to sign.
  *                                          This port supports the following mechanisms:
  *                                           - CKM_AES_CBC for AES Decrypt
  * @param[in] xKey                          The handle of the private key to be used for
  *                                          encrypt. Key must be compatible with the
  *                                          mechanism chosen by pxMechanism.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OPERATION_ACTIVE, CKR_KEY_HANDLE_INVALID,
  *         CKR_KEY_TYPE_INCONSISTENT, CKR_MECHANISM_INVALID, CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED.
  */

CK_DECLARE_FUNCTION(CK_RV, C_DecryptInit)(CK_SESSION_HANDLE xSession,
                                       CK_MECHANISM_PTR pxMechanism,
                                       CK_OBJECT_HANDLE xKey)
{
  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
  P11Object_t p11Obj;

  P11SessionPtr_t pxSession = prvSessionPointerFromHandle(xSession);
  /*
      * Check that the PKCS #11 module is initialized,
      * if the session is open and valid.
  */
  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

  if (xResult == CKR_OK)
  {
    if (NULL == pxMechanism)
    {
      xResult = CKR_ARGUMENTS_BAD;
    }
    else if ((CK_ULONG)NULL != pxSession->EncDecMechanism)
    {
      xResult = CKR_OPERATION_ACTIVE;
    }
    else
    {
      findObjectInListByHandle(xKey, &xHandle, &p11Obj);
    }
  }

  /*
    * Check the validity of Key handle
  */
  if (xResult == CKR_OK)
  {
    if (xHandle == CK_INVALID_HANDLE)
    {
      xResult = CKR_KEY_HANDLE_INVALID;
    }
  }

  /*
    * Check that a secret key was retrieved.
  */
  if (xResult == CKR_OK)
  {
    if (p11Obj.xClass != CKO_SECRET_KEY)
    {
      xResult = CKR_KEY_TYPE_INCONSISTENT;
    }
  }


  if (xResult == CKR_OK)
  {
	  pxSession->hEncDecKey = xHandle;
	  if (strcmp((CCHAR *)p11Obj.xLabel, ENCDEC_DERIVED_KEY) == 0)
	  {
		  pxSession->targetKeyParameter = (CK_BYTE)0x10;
	  }
	  else
	  {
		  xResult = CKR_KEY_HANDLE_INVALID;
	  }
  }


  /*
    * Check that the mechanism is compatible, supported.
    * If the mechanism is NULL it's considered as CKM_AES_CBC
  */
  if (xResult == CKR_OK)
  {
	  if (pxMechanism->mechanism == (CK_ULONG)NULL)
	  {
		  pxMechanism->mechanism = CKM_AES_CBC;
	  }

      if (pxMechanism->mechanism != CKM_AES_CBC)
	  {
		  xResult = CKR_MECHANISM_INVALID;
	  }
  }

  if (xResult == CKR_OK)
  {
    pxSession->m_DecryptInitialized = (CK_BBOOL)1;
    pxSession->EncDecMechanism = pxMechanism->mechanism;
  }

  return xResult;
}


/**
  * @brief Performs a data decrypting operation.
  * The decrypting operation must have been initialized with C_DecryptInit.
  *
  * @param[in] xSession                     Handle of a valid PKCS #11 session.
  * @param[in] pEncryptedData               the encrypted data.
  * @param[in] puEncryptedDataLen           Length of pEncryptedData, in bytes.
  * @param[out] pData               		Buffer where receives decrypted data.
  * @param[out] ulDataLen           		Length of pData buffer.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_BUFFER_TOO_SMALL, CKR_OPERATION_NOT_INITIALIZED,
  *         CKR_DATA_LEN_RANGE ,CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED, Vendor defined error.
  */
CK_DECLARE_FUNCTION(CK_RV, C_Decrypt)(CK_SESSION_HANDLE xSession,
                                     CK_BYTE_PTR pEncryptedData,
                                     CK_ULONG puEncryptedDataLen,
                                     CK_BYTE_PTR pData,
                                     CK_ULONG_PTR uDataLen)
  {
    CK_ULONG xExpectedInputLength = 0;
    CK_BYTE_PTR pxDecipherBuffer = pData;
    uint8_t dechiper[256];

    P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
    /*
        * Check that the PKCS #11 module is initialized,
        * if the session is open and valid.
    */
    CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);

    if (xResult == CKR_OK)
    {
      if (!pxSessionObj->m_DecryptInitialized)
      {
        xResult = CKR_OPERATION_NOT_INITIALIZED;
      }
    }

    if (xResult == CKR_OK)
    {
      if ((puEncryptedDataLen == (uint32_t)0) || (NULL == pEncryptedData) || (NULL == uDataLen))
      {
        xResult = CKR_ARGUMENTS_BAD;
      }
    }


    if (xResult == CKR_OK)
    {
    	xExpectedInputLength = 0x00;/*da definire la len dei dati in ingresso*/
    	pxDecipherBuffer = dechiper;

    	if (*uDataLen < 256) /*da definire la len dei dati in uscita*/
    	{
    		xResult = CKR_BUFFER_TOO_SMALL;
    	}
    }
    if (xResult == CKR_OK)
    {
    	/*
    	 * Check that input data to be signed is the expected length.
    	 */
    	if (xExpectedInputLength != puEncryptedDataLen)
    	{
    		xResult = CKR_DATA_LEN_RANGE;
    	}
    }
    /*
     * Sign the data.
     */
    if (xResult == CKR_OK)
    {
    	xResult = decrypt(pxSessionObj, pData,
    			(CK_BYTE)uDataLen,
				pxDecipherBuffer,
				uDataLen);
    	if (xResult == (uint32_t)0)
    	{
    		(void)memcpy(pData, pxDecipherBuffer, *uDataLen);
    	}
    }
    /*
     * Complete the operation in the context.
     */
    pxSessionObj->EncDecMechanism = (CK_ULONG)NULL;
    pxSessionObj->targetKeyParameter = 0x00;
    pxSessionObj->m_DecryptInitialized = (CK_BBOOL)0;

    return xResult;
  }




/*
  *Cryptoki provides the following functions for verifying signatures on data.
  */

/**
  * @brief Initializes a verification operation.
  * After calling C_VerifyInit, the application can either call C_Verify to verify a signature
  * on data in a single part.
  *
  * @param[in] xSession       Handle of a valid PKCS #11 session.
  * @param[in] pxMechanism    Points to the structure that specifies the verification mechanism
  * @param[in] xKey           is the handle of the verification key.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OPERATION_ACTIVE, CKR_MECHANISM_INVALID
  *         CKR_KEY_HANDLE_INVALID, CKR_KEY_FUNCTION_NOT_PERMITTED,
  *         CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_INVALID,
  *         CKR_SESSION_CLOSED.
  */
//CK_DECLARE_FUNCTION(CK_RV, C_VerifyInit)
//(
//  CK_SESSION_HANDLE xSession,
//  CK_MECHANISM_PTR  pxMechanism,
//  CK_OBJECT_HANDLE  hKey
//)
//{
//  P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  CK_OBJECT_HANDLE xHandle = CK_INVALID_HANDLE;
//  P11Object_t p11Obj;
//  MECHANISM_t Mechanism = { pxMechanism->mechanism, 0, 0, CKF_VERIFY};
//
//  if (xResult == CKR_OK)
//  {
//    if ((hKey == (uint32_t)0) || (pxMechanism == __NULL))
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (pxSessionObj->m_VerifyInitialized == 1)
//    {
//      xResult = CKR_OPERATION_ACTIVE;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (Mechanism.type != CKM_ECDSA)
//    {
//      xResult = CKR_MECHANISM_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    findObjectInListByHandle(hKey, &xHandle, &p11Obj);
//    if (xHandle == CK_INVALID_HANDLE)
//    {
//      xResult = CKR_KEY_HANDLE_INVALID;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (p11Obj.xClass != CKO_PUBLIC_KEY)
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (setPublicKeyEnvironmentTarget(pxSessionObj, p11Obj.xLabel) == false)
//    {
//      xResult = CKR_KEY_FUNCTION_NOT_PERMITTED;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    pxSessionObj->m_VerifyInitialized = (CK_BBOOL)1;
//  }
//
//  return xResult;
//}
/**
  * @brief  C_Verify verifies a signature in a single-part operation.
  * The verification operation must have been initialized with C_VerifyInit
  *
  * @param  xSession              Handle of a valid PKCS #11 session.
  * @param  pData                 Points to the original data.
  * @param  ulDataLen             Is the length of the data.
  * @param  pSignature            Points to the signature.
  * @param  ulSignatureLent       Is the length of the signature
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_OPERATION_NOT_INITIALIZED, CKR_DATA_LEN_RANGE,
  *         CKR_SIGNATURE_LEN_RANGE, CKR_SIGNATURE_INVALID, CKR_FUNCTION_FAILED,
  *         CKR_CRYPTOKI_NOT_INITIALIZED, CKR_SESSION_HANDLE_IN,VALID,
  *         CKR_SESSION_CLOSED.
  */
//CK_DECLARE_FUNCTION(CK_RV, C_Verify)
//(
//  CK_SESSION_HANDLE xSession,
//  CK_BYTE_PTR pData,
//  CK_ULONG ulDataLen,
//  CK_BYTE_PTR pSignature,
//  CK_ULONG ulSignatureLen
//)
//{
//  P11SessionPtr_t pxSessionObj = prvSessionPointerFromHandle(xSession);
//  /*
//      * Check that the PKCS #11 module is initialized,
//      * if the session is open and valid.
//  */
//  CK_RV xResult = PKCS11_SESSION_VALID_AND_MODULE_INITIALIZED(xSession);
//
//  if (xResult == CKR_OK)
//  {
//    if (!pxSessionObj->m_VerifyInitialized)
//    {
//      xResult = CKR_OPERATION_NOT_INITIALIZED;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if (ulDataLen != (uint32_t)PKCS11_SHA256_DIGEST_LENGTH) /* The length of data to verify must be 32 byte*/
//    {
//      xResult = CKR_DATA_LEN_RANGE;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if ((pData == __NULL) || (pSignature == __NULL))
//    {
//      xResult = CKR_ARGUMENTS_BAD;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    if ((ulSignatureLen != (uint32_t)PKCS11_ECDSA_P256_SIGNATURE_LENGTH)
//        && (ulSignatureLen != (uint32_t)(PKCS11_ECDSA_P256_SIGNATURE_LENGTH - 1))
//        && (ulSignatureLen != (uint32_t)(PKCS11_ECDSA_P256_SIGNATURE_LENGTH - 2)))
//    {
//      xResult = CKR_SIGNATURE_LEN_RANGE;
//    }
//  }
//
//  if (xResult == CKR_OK)
//  {
//    xResult = ExecuteVerifySignature(pxSessionObj, pData, (CK_BYTE) ulDataLen, pSignature, ulSignatureLen);
//    if (xResult == DS_VERIFICATION_FAILED)
//    {
//      xResult = CKR_SIGNATURE_INVALID;
//    }
//    else if (xResult != (uint32_t)0)
//    {
//      xResult = CKR_FUNCTION_FAILED;
//    }
//    else
//    {
//      /***************/
//    }
//  }
//
//  pxSessionObj->m_VerifyInitialized = (CK_BBOOL)0;
//
//  return xResult;
//}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
