/**
  ******************************************************************************
  * @file    st_comm_layer.c
  * @author  MCD Application Team
  * @brief   Header for st_comm_layer.c module
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


#ifndef ST_COMM_LAYER_H
#define ST_COMM_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "com_err.h"

/* Exported constants --------------------------------------------------------*/
/* Max. signature length is 72 bytes + SW(2) + '\0' (144 characters + 4 + 1) */
#define MAX_BUFFER_LENGTH_FOR_SIGNATURE         150
/* Max. cipher response buffer length is 242 bytes + SW(2) + '\0' (484 characters + 4 + 1) */
#define MAX_BUFFER_LENGTH_FOR_ENCDEC     250


/* Max. Challenge length is 128 bytes + SW(2) + '\0'  (256 characters + 4 + 1) */
#define MAX_BUFFER_LENGTH_FOR_CHALLENGE         261
/* Max. Challenge length is 77 bytes + SW(2) + '\0' (154 characters + 4 + 1)  */
#define MAX_BUFFER_LENGTH_FOR_GENERATE_SECRET   159
#define CKR_STM32_CUSTOM_ATTRIBUTE              (CKR_VENDOR_DEFINED | 0x08000000UL)
#define CKR_NOT_STATUS_WORD                     (CKR_STM32_CUSTOM_ATTRIBUTE + 16U)

#define SETTING_EC256_BSO_KEY_SE_HI_BYTE        (0x30)
#define SETTING_EC256_BSO_KEY_SE_LO_BYTE        (0x33)

#define UPDATE_BINARY_BLOCK_SIZE        240U
#define READ_BINARY_BLOCK_SIZE          250U

#define RESP_BUFFER_READ_RECORD_SIZE       171U
#define RESP_BUFFER_READ_BINARY_SIZE       511U
#define SIGN_BUFFER_LENGTH                  75U    /* 75 = 5*2 (command length) + 32*2 (digest length) + '\0' */
#define MAX_ENC_BUFFER_LENGTH              239U
#define MAX_DEC_BUFFER_LENGTH              240U

#define PUBLIC_KEY_SEND_BUFFER_LEN      ((PUBLIC_KEY_LEN_NO_TAGLEN*2) + 10 + 1)
#define PRIVATE_KEY_SEND_BUFFER_LEN      ((PRIVATE_KEY_LEN*2) + 10 + 1)
#define SECRET_KEY_GEN_BUFFER_LEN       (SECRET_KEY_LENGTH + 4 + 1) /* 32 (length of the Secret) + 4 (SW) + 1 */

#define MAX_BYTES_FOR_CHALLENGE         128U /*  Maximum number of bytes to be asked for GetChallenge */

#define URL_FILE_TARGET						0x01
#define CERTIFICATE_FILE_TARGET				0x02
#define LEN_RESP							32

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  This function initializes the communication with the token.
  * @note   This function must be called only once, before using any other com_ese function.
  * @param  -
  * @retval bool - true/false if init is ok/nok
  */
CK_ULONG InitCommunicationLayer(void);

/**
  * @brief  This function closes an ESE session.
  * @note   This function closes an ESE session and release the ese handle.
  * @param  -
  * @retval 0 if successful or an error code (see note)
  * @note   Possible returned value:\n
  *         - COM_ERR_OK           : ese handle release OK
  *         - COM_ERR_DESCRIPTOR   : ese handle parameter NOK
  *         - COM_ERR_STATE        : a command is already in progress and its answer is not yet received
  */
CK_ULONG CloseCommunication(void);

/**
  * @brief This function performs a digital signature operation.
  * @param[in] pxSessionObj                  Handle of a valid PKCS#11 session
  * @param[in] pDataToSign                   Data to sign
  * @param[in] DataToSignLen                 Length in bytes of pDataToSign
  * @param[out] pSignature                   Buffer that receives the signature
  * @param[in,out] pulSignatureLen           Length in byte of pSignature buffer
  * @return 0 if successful or a specific error code.
  */
CK_ULONG ComputeSignature(P11SessionPtr_t pxSessionObj, CK_BYTE *pDataToSign, CK_BYTE DataToSignLen,
                          CK_BYTE *pSignature, CK_ULONG_PTR puSignatureLen);


/**
  * @brief Performs a data enrypting operation.
  * @param[in] pxSessionObj                  Handle of a valid PKCS #11 session.
  * @param[in] pDataToEncrypt                the plaintext data.
  * @param[in] DataToEncryptLen              Plaintext data max len = 239 byte.
  * @param[out] pEncryptedData               Buffer where receives encrypted data.
  * @param[out] puEncryptedDataLen           Length of pEncryptedData buffer.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_BUFFER_TOO_SMALL, CKR_OPERATION_NOT_INITIALIZED,
  *         CKR_DATA_LEN_RANGE ,CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED, Vendor defined error.
  */
CK_ULONG encrypt(P11SessionPtr_t pxSessionObj, CK_BYTE *pDataToEncrypt, CK_BYTE DataToEncryptLen,
                          CK_BYTE *pEncryptedData, CK_ULONG_PTR puEncryptedDataLen);

/**
  * @brief Performs a data decrypting operation.
  * @param[in] xSession                     Handle of a valid PKCS #11 session.
  * @param[in] pEncryptedData               the encrypted data.
  * @param[in] puEncryptedDataLen           Length of pEncryptedData, in bytes.
  * @param[out] pData               		Buffer where receives decrypted data.
  * @param[out] uDataLen           			Length of pData buffer.
  *
  * @return CKR_OK if successful or the following error codes:
  *         CKR_ARGUMENTS_BAD, CKR_BUFFER_TOO_SMALL, CKR_OPERATION_NOT_INITIALIZED,
  *         CKR_DATA_LEN_RANGE ,CKR_CRYPTOKI_NOT_INITIALIZED,
  *         CKR_SESSION_HANDLE_INVALID, CKR_SESSION_CLOSED, Vendor defined error.
  */

CK_ULONG decrypt(P11SessionPtr_t pxSessionObj, CK_BYTE *pEncryptedData, CK_BYTE pEncryptedDataLen,
                          CK_BYTE *pData, CK_ULONG_PTR puDataLen);


/**
  * @brief This function allows selecting a file (EF or DF) on the token, by path.
  * @param[in] path                          Path of the file (EF or DF) to select
  * @param[in] ulFidsInPath                  Number of FIDs in the path
  * @return 0 if successful or a specific error code.
  */
static CK_ULONG PathSelect(CK_BYTE *path, CK_ULONG ulFidsInPath);

/**
  * @brief This function allows selecting a file (EF or DF) on the token, by path.
  * @param[in] path                          Path of the file (EF or DF)
  * @param[in] ulFidsInPath                  Number of FIDs in the path
  * @return 0 if successful or a specific error code.
  */
CK_ULONG PathSelectWORD(uint16_t *path, CK_ULONG ulFidsInPath);

/**
  * @brief This function allows selecting a file (EF or DF) on the token.
  * @param[in] fid                           File identifier
  * @param[in] SelOpt                        Selection options
  * @return 0 if successful or a specific error code.
  */
static CK_ULONG SelectEx(uint16_t fid, uint32_t *SelOpt);

/**
  * @brief The function reads all or part of the data in a transparent file on the token.
  * @param[in] Offset                        Read offset position
  * @param[in] len                           Number of bytes to read
  * @param[out] pBuffer                      Buffer that receives the read data
  * @return 0 if successful or a specific error code.
  */
CK_ULONG ReadBinary(uint16_t Offset, uint16_t len, CK_BYTE_PTR pBuffer);

/**
  * @brief The function updates a transparent data file on the token.
  * @param[in] Data                          Data to update
  * @param[in] DataLen                       Number of bytes to update
  * @param[in] Offset                        Update offset position
  * @return 0 if successful or a specific error code.
  */
CK_ULONG UpdateBinary(CK_BYTE *Data, uint16_t DataLen, uint16_t Offset);

/**
  * @brief This function reads the contents of a linear fixed file (EF) on the token.
  * @param[in] access_mode                   Mode to access to the file
  * @param[in] rec                           Number of the record to read
  * @param[out] pData                        Buffer that receives the read data
  * @param[in] DataLen                       Number of bytes to read
  * @return 0 if successful or a specific error code.
  */
CK_ULONG ReadRecord(CK_BYTE access_mode, CK_BYTE rec, CK_BYTE *pData, CK_BYTE *DataLen);

/**
  * @brief This function generates a new public-private key pair.
  * This functions only supports generating elliptic curve P-256 key pairs.
  * @param[in] BsoId                             Secure object identifier
  * @param[in] PubKeyFid                         Public key identifier
  * @return return 0 if successful or a specific error code.
  */
CK_ULONG GenerateKeyPair(CK_BYTE BsoId, uint16_t PubKeyFid);

/**
  * @brief This function is not implemented (RFU).
  * All inputs to this function are ignored, and calling this
  * function always returns 0 (operation completed successfully).
  *
  * @return CKR_OK.
  */
/* unsigned long doLogin(unsigned char *pPin, unsigned long ulPinLen); */

/**
  * @brief This function generates a challenge (random bytes).
  * @param pChallenge[out]       Buffer that receives the challenge (random data)
  * @param pulChallenge[in]      Length in bytes of the random data
  *
  * @return 0 if successful or a specific error code.
  */
CK_ULONG GetChallenge(CK_BYTE *pChallenge, CK_ULONG pulChallenge);

/**
  * @brief This function generates a derived key.
  * @param pParams[in]           Mechanism parameters
  * @param pSecret[out]          Buffer that receives the derived key.
  * @param pulSecret[out]        Length in bytes of the erived key.
  * @return 0 if successful or a specific error code.
  */
CK_ULONG GenerateSecret(CK_ECDH1_DERIVE_PARAMS  pParams, CK_BYTE *pSecret, int32_t *pulSecret);

/**
  * @brief This function performs a verify signature operation. The Public Key is considered as already selected
  * @param[in] pxSessionObj                  Handle of a valid PKCS#11 session
  * @param[in] pDataToVerify                 Data to verify
  * @param[in] DataToVerifyLen               Length in bytes of pDataToVerify
  * @param[in] pSignature                    Buffer that contains the signature to verify
  * @param[in] pulSignatureLen               Length in bytes of pSignature buffer
  * @return 0 if successful or a specific error code.
  */
CK_ULONG VerifySignature(const CK_BYTE *pDataToVerify, CK_BYTE DataToVerifyLen, CK_BYTE *pSignature,
                         CK_ULONG puSignatureLen);

/**
  * @brief This function updates the Public Key related to the given BSO with a given value.
  * @param[in] keyValue   The value of the new Key.
  * @param[in] keyLen     The length of the new Key.
  * @return 0 if successful or a specific error code.
  */
CK_ULONG UpdateKey(CK_BYTE *keyValue, CK_BYTE keyLen);

/**
  * @brief This function selects the Public Key related to the given BSO.
  * @param[in] SelectedBSO      The given BSO.
  * @return 0 if successful or a specific error code.
  */
CK_ULONG SelectPublicKey(CK_BYTE SelectedBSO);

/**
  * @brief This function executes the verification of a digital signature, first selecting the given Public Key.
  * @param[in] pxSessionObj                  Handle of a valid PKCS#11 session
  * @param[in] pDataToVerify                 Data to verify
  * @param[in] DataToVerifyLen               Length in bytes of pDataToVerify
  * @param[in] pSignature                    Buffer that contains the signature to verify
  * @param[in] pulSignatureLen               Length in bytes of pSignature buffer
  * @return 0 if successful or a specific error code.
  */
CK_ULONG ExecuteVerifySignature(P11SessionPtr_t pxSessionObj, const CK_BYTE *pDataToVerify, CK_BYTE DataToVerifyLen,
                                CK_BYTE *pSignature, CK_ULONG puSignatureLen);

CK_ULONG composePKCS11Return(int32_t ret);

CK_ULONG DeriveSecret(CK_HKDF_PARAMS pParams, int8_t * baseKeyLabel, char * derivedKeyLabel, CK_BYTE *pSecret, int32_t *pulSecret);

CK_ULONG retrieveSecretKeysPresence(CK_BYTE *pResponse, CK_BYTE_PTR pResponseLen);

#ifdef __cplusplus
}
#endif

#endif /* ST_COMM_LAYER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
