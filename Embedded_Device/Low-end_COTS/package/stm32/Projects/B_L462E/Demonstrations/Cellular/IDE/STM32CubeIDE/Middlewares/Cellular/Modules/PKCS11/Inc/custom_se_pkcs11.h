/**
  ******************************************************************************
  * @file    custom_se_pkcs11.h
  * @author
  * @brief   Header for custom_se_pkcs11.c module
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CUSTOM_SE_PKCS11_H
#define CUSTOM_SE_PKCS11_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include <stdint.h>
#include "st_p11.h"

#define HMAC		  0x0001
#define AES128		  0x0002
#define AES_256_CBC	  0x0003
#define CMAC		  0x0004

/* Exported functions ------------------------------------------------------- */

static char* pSKID = "PSK";
static char* mSKID = "MSK";
static char* EDKID = "EDK";
static CK_BYTE signature[32];
/**
  * @brief  Library initialization
  * @note   This function initializes the communication with the Secure Element (SE). First function to call.
  * @param  -
  * @retval return 0 means OK
  */
int32_t csp_initialize();


/**
  * @brief  Import the AES-256 pre-shared key (PSK) into the SE
  * @note   A PSK is a 32-byte array in hexadecimal format.
  * @param  pSKValue - PSK value
  * @retval return 0 means OK
  */
int32_t csp_installPSK(unsigned char* pSKValue);


/**
  * @brief  Import the MUD file URL into the SE
  * @note   The URL of a MUD file is a string like https://www.example.com/yourmudfile.json.
  * @param  mUDuRLValue - Value of the MUD file URL
  * @retval return 0 means OK
  */
int32_t csp_installMudURL(char* mUDuRLValue);


/**
  * @brief  Get the URL value of the MUD file
  * @param  mUDuRLValue - The location that receives the MUD file URL
  * @retval return 0 means OK
  */
int32_t csp_getMuDFileURL(char* mUDuRLValue);


/**
  * @brief  Import the Idenetity Certificate in the SE
  * @note   An Identity Certificate is a byte array in hexadecimal format.
  * @param  certificateValue - Value of the Identity Certificate
  * @param  certificateValueLen - Size of Identity Certificate
  * @retval return 0 means OK
  */
int32_t csp_installCertificate(unsigned char* certificateValue, uint16_t certificateValueLen);


/**
  * @brief  Get the value of the Identity Certificate
  * @param  certificateValue - The location that receives the value of the Identity Certificate
  * @param  certificateValueLen - The locateion taht receives the Identity Certificate size
  * @retval return 0 means OK
  */
int32_t csp_getCertificateValue(unsigned char* certificateValue, uint16_t * certificateValueLen);


/**
  * @brief  Derive a key from a base key using the algorithm specified by the parameter "algo"
  * @note   baseKeyID and derivedKeyID admitted values: "PSK", "MSK", "PSK_1", "PSK_2", "PSK_3", "PSK_4", "PSK_5", "PSK_6", "PSK_7", "PSK_8".
  * @note   algo admitted values: HMAC, AES128
  * @note   values for baseKeyID: "PSK_[1-8]" are only valid for "HMAC"
  * @param  baseKeyID         - Key ID of the base key
  * @param  derivedKeyID      - Key ID of the derived key
  * @param  salt              - Salt value (a 32-byte (16 for AES128) array in hexadecimal format -> e.g., 'fingerprint' or 'challenge')
  * @param  info              - Context and application specific information (e.g., 'Domain manager' or 'Domain ID')
  * @param  infoLen	      - Size of the info
  * @param  algo              - The algorithm used to sign
  * @retval return 0 means OK
  */
int32_t csp_deriveKey(char* baseKeyID, char* derivedKeyID, unsigned char* salt, unsigned char* info, uint16_t infoLen, uint16_t algo);


/**
  * @brief  Calculate signature using the algorithm specified by the parameter "algo"
  * @note   signatureKeyID admitted values: "PSK", "MSK", "PSK_1", "PSK_2", "PSK_3", "PSK_4", "PSK_5", "PSK_6", "PSK_7", "PSK_8".
  * @note   algo admitted values: HMAC, CMAC
  * @note   values for baseKeyID: "PSK_[1-8]" are only valid for "HMAC"
  * @param  signatureKeyID    - Key ID of the signature key
  * @param  dataToSign        - Data to sign array in hexadecimal format)
  * @param  dataToSignLen     - Size of data
  * @param  signature         - The location that receives the signature
  * @param  algo              - The algorithm used to sign
  * @retval return 0 means OK
  */
int32_t csp_sign(char* signatureKeyID, unsigned char* dataToSign, uint16_t dataToSignLen, unsigned char* signature, uint16_t algo);


/**
  * @brief  Encrypt data
  * @param  encriptionKeyId	- Key ID of the encryption key
  * @param  dataToEncrypt	- Data to encrypt
  * @param  dataToEncryptLen	- size of data to encrypt
  * @param  encryptedData	- The location that receives the encrypted data
  * @param  algo		- The algorithm used to encrypt
  * @retval return 0 means OK
  */
int32_t csp_encryptData(char* encryptionKeyID, unsigned char* dataToEncrypt, uint16_t dataToEncryptLen, unsigned char* encryptedData, uint16_t algo);


/**
  * @brief  Decrypt data
  * @param  decriptionKeyId	- Key ID of the decryption key
  * @param  dataToDecrypt	- Data to encrypt
  * @param  dataToDecryptLen	- Size of data to decrypt
  * @param  decryptedData	- The location that receives the decrypted data
  * @param  decryptedDataLen	- Size of decrypted data
  * @param  algo		- The algorithm used to encrypt
  * @retval return 0 means OK
  */
int32_t csp_decryptData(char* decryptionKeyID, unsigned char* dataToDecrypt, uint16_t dataToDecryptLen, unsigned char* decryptedData, uint16_t * decryptedDataLen, uint16_t algo);


/**
 * @brief Generate random buffer
 * @param randomBuffer the location to generated random data (needs to be allocated beforehand) 
 * @param randomBufferLen Byte length of requested data
 * @retval return 0 means OK
 */
int32_t csp_generateRandom(unsigned char * randomBuffer, uint16_t randomBufferLen);


/**
  * @brief  Library finalization
  * @note   This function terminates the communication with the Secure Element (SE). Last function to call.
  * @param  -
  * @retval return 0 means OK
  */
int32_t csp_terminate();




#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_SE_PKCS11_H */


/************************ (C) COPYRIGHT STMicroelectronics ***** END OF FILE ****/

