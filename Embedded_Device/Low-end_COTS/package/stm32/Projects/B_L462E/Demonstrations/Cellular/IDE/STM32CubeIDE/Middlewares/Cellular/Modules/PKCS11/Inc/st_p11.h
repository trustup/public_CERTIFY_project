/**
  ******************************************************************************
  * @file    st_p11.h
  * @author  MCD Application Team
  * @brief   Header for st_p11.c module
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

#ifndef ST_P11_H
#define ST_P11_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "st_pkcs11ex.h"
#include "st_p11_config.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* Exported constants --------------------------------------------------------*/
#define NUM_OBJECTS             (uint8_t)12


/* Private defines for checking that attribute templates are complete. */
#define PKCS11_ECDSA_P256_SIGNATURE_LENGTH      0x48
#define PKCS11_SHA256_DIGEST_LENGTH             0x20
#define SECRET_KEY_LENGTH                       32
#define MECH_NUMBER                             5
#define PUBLIC_KEY_LEN_NO_TAGLEN                65
#define PRIVATE_KEY_LEN                         32
#define PUBLIC_KEY_LEN                          PUBLIC_KEY_LEN_NO_TAGLEN + 2
#define DS_VERIFICATION_FAILED                  0x88006300U
#define CUSTOM_FUNCTION_FAILED                  0x88006F00U


/* CryptoKI INFO */
#define CK_VER_MAJ         3
#define CK_VER_MIN         0
#define LIB_VER_MAJ        1 /* Next Release */
#define LIB_VER_MIN        13 /* Next Release */
#define CKINFO_FLAGS       0
#define MANUFACTURER_ID    "ST Microelectronics"
#define LIBRARYDESCRIPTION "STM32-ST33 Cryptoki"

/* INITIAL OBJECTS NAMES */
#define MAX_RANDOM_LENGTH                       128
#define KEY_SIZE                                256
#define __NULL                                  (void *)0


typedef const char CCHAR;

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  CK_OBJECT_HANDLE xHandle;
  int8_t *xLabel;
  CK_ULONG xClass;
  CK_BYTE xToken;
  CK_BYTE xSign;
  CK_BYTE xDerive;
  CK_BYTE *xValue;
  CK_ULONG xValueLen;
  CK_BYTE xVerify ;
} P11Object_t;

/*
 * This structure helps maintaining a mapping of all P11 objects in one place.
 */
typedef struct
{
  P11Object_t xObjects[NUM_OBJECTS];
} P11ObjectList_t;

/* PKCS #11 Module Object */
typedef struct P11Struct_t
{
  CK_BBOOL xIsInitialized; /* Indicates whether PKCS #11 module has been initialized with a call to C_Initialize. */
  P11ObjectList_t xObjectList; /* List of PKCS #11 objects that have been found/created since module initialization. */
} P11Struct_t, * P11Context_t;

/**
  * @brief PKCWS#11 session structure.
  */
typedef struct
{
  CK_ULONG ulState;                         /* Stores the session flags. */
  CK_BBOOL xOpened;                         /* Set to CK_TRUE upon opening PKCS #11 session. */
  CK_BYTE targetKeyParameter;               /* Stores the security object to use. */
  CK_BYTE targetCertificateParameter;       /* Stores the certificate to use. */
  /* CK_MECHANISM_TYPE xOperationInProgress;  */    /* Indicates if a digest operation is in progress. */
  CK_BBOOL xFindObjectInit;       /* Indicates wether or not the C_FindObjectsInit function has been performed. */
  CK_BBOOL xFindObjectComplete;   /* Indicates wether or not the find objects has been called. */
  CK_BYTE pxFindObjectLabel[MAX_LABEL_LENGTH]; /* Pointer to the object's label (identifier) to search.
                                                  Should be NULL if no search in progress. */
  uint8_t xFindObjectLabelLength;   /* Length of the object's label (identifier). */
  CK_BBOOL xLogged;                 /* Indicates wether or not the user has logged into the token. */
  CK_MECHANISM_TYPE xSignMechanism;   /* Mechanism of the sign operation in progress. Sets during C_SignInit. */
  CK_MECHANISM_TYPE EncDecMechanism;  /* Mechanism of the encrypt operation in progress. Sets during C_EncryptInit. */
  CK_OBJECT_HANDLE hSignKey;        /* Handle of the signature key. */
  CK_OBJECT_HANDLE hEncDecKey;      /* Handle of the end/dec key. */
  CK_BBOOL m_SignInitialized;       /* Indicates whether or not the C_SignInit function has been performed. */
  CK_BBOOL m_EncryptInitialized;    /* Indicates whether or not the C_EncryptInit function has been performed. */
  CK_BBOOL m_DecryptInitialized;    /* Indicates whether or not the C_DecryptInit function has been performed. */
  CK_BBOOL ephemeralKeyGenerated;   /* Indicates whether or not the C_GenerationKeyPair function has been performed
                                       and there is an active ephemeral Key Pair. */
  CK_BBOOL m_VerifyInitialized;     /* Indicates whether or not the C_VerifyInit function has been performed. */
} P11Session_t, * P11SessionPtr_t;

typedef struct
{
  CK_MECHANISM_TYPE type;
  CK_MECHANISM_INFO info;
} MECHANISM_t, * MECHANISM_PTR_t;

/* External variables --------------------------------------------------------*/

/* The global PKCS #11 module object. */
static P11Struct_t xP11Context;

/* The global PKCS #11 session. */
static P11Session_t xP11session;
static P11SessionPtr_t prvSessionPointerFromHandle(CK_SESSION_HANDLE xSession);

/* Default value of label of ephemeral object */
static CK_BYTE defaultPrivateLabel[]   = "EphPrivK";
static CK_BYTE defaultPublicLabel[]    = "EphPubK";



static MECHANISM_t m_MechList[MECH_NUMBER];

static char secretLabel[MAX_LABEL_LENGTH];
static CK_BYTE secretValue[SECRET_KEY_LENGTH];
uint32_t generateRandom(void);

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* ST_P11_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
