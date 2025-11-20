#ifndef CSP_API_H
#define CSP_API_H

#include <edge_call.h>
#include <keystone.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include "../common/util.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stddef.h>

// Constantes
#define CMD_INSTALL_PSK        1
#define CMD_INSTALL_MUD_URL    2
#define CMD_GET_MUD_URL        3
#define CMD_INSTALL_CERT       4
#define CMD_GET_CERT           5
#define CMD_DERIVE_KEY         6
#define CMD_SIGN               7
#define CMD_ENCRYPT            8
#define CMD_DECRYPT            9
#define CMD_GEN_RANDOM         10
#define CMD_RECONFIGURE        11


#define KEY_MATERIAL_PATH "/etc/myapp/key_material.bin"
#define KEY_MATERIAL_DIR  "/etc/myapp/key_material"
#define MUD_URL_PATH "/etc/myapp/mud_url.bin"
#define CERTIFICATE_PATH "/etc/myapp/certificate.bin"
#define AK_PATH "/etc/myapp/key_material/ak.bin"
#define KDK_PATH "/etc/myapp/key_material/kdk.bin"
#define MSK_PATH "/etc/myapp/key_material/msk.bin"
#define EDK_PATH "/etc/myapp/key_material/edk.bin"

// Nuevas macros para los directorios base
#define BASE_DIR "/etc/myapp"

// Macros para las rutas de los archivos de claves
#define AK_PSK_PATH "/etc/myapp/key_material/ak_psk.bin"
#define KDK_PSK_PATH "/etc/myapp/key_material/kdk_psk.bin"
#define AK_MSK_PATH "/etc/myapp/key_material/ak_msk.bin"
#define KDK_MSK_PATH "/etc/myapp/key_material/kdk_msk.bin"

typedef struct {
    void* data;
    size_t size;
} dynamic_arg_t;

typedef struct {
    dynamic_arg_t args[MAX_ARGS];
    int num_args;
} dynamic_args_t;

extern char* eapp;
extern char* runtime;
extern char* loader;

void run_enclave_with_operation(int test_case);

int32_t csp_installPSK(unsigned char* pSKValue);
int32_t csp_installMudURL(char* mUDuRLValue);
int32_t csp_getMuDFileURL(char* mUDuRLValue);
int32_t csp_installCertificate(unsigned char* certificateValue, uint16_t certificateValueLen);
int32_t csp_getCertificateValue(unsigned char* certificateValue, uint16_t* certificateValueLen);
int32_t csp_deriveKey(char* baseKeyID, char* derivedKeyID, unsigned char* salt, unsigned char* info, uint16_t infoLen, uint16_t algo);
int32_t csp_sign(char* signatureKeyID, unsigned char* dataToSign, uint16_t dataToSignLen, unsigned char* signature);
int32_t csp_encryptData(char* encryptionKeyID, unsigned char* dataToEncrypt, uint16_t dataToEncryptLen, unsigned char* encryptedData, uint16_t algo);
int32_t csp_decryptData(char* decryptionKeyID, unsigned char* dataToDecrypt, uint16_t dataToDecryptLen, unsigned char* decryptedData, uint16_t* decryptedDataLen, uint16_t algo);
int32_t csp_generateRandom(unsigned char * randomBuffer, uint16_t randomBufferLen);
int32_t csp_reconfigure(uint8_t opcode, unsigned char * operationData, uint16_t operationDataLen);

#endif // CSP_API_H 