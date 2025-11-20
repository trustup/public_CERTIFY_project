#ifndef MAIN_EAPP_H
#define MAIN_EAPP_H

// Keystone SDK includes
#include "eapp_utils.h"
#include "edge_call.h"
#include <syscall.h>
#include "app/syscall.h"
#include "app/string.h"
#include "app/malloc.h"

// Standard C includes
#include "string.h"
#include <stdio.h>

// Project includes
#include "aes.h"
#include "aes256.h"
#include "util.h"
#include "crypto_utils.h"
#include "sha-256.h"
#include "TI_aes_128.h"
#include "aes-cbc-cmac.h"

// Comandos
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

#define SEALING_KEY_BITS_LEN   128

#define KEY_ID_KEY_MATERIAL "identifier1"
#define KEY_ID_MUD_URL "identifier2"
#define KEY_ID_CERTIFICATE "identifier3"

#define AES_MODE_CBC 0
#define AES_MODE_ECB 1

// Definicion de estructura para la sesión de AES
typedef enum {
    AES_TYPE_128,
    AES_TYPE_256
} aes_type_t;

typedef struct {
  struct AES_ctx ctx;     // Contexto de tiny-AES
  struct AES256_ctx ctx256;     // Contexto de tiny-AES
  uint8_t iv[16];         // IV (para modos CBC/CTR)
  uint32_t mode;          // Ej: AES_ENCRYPT/AES_DECRYPT
  uint32_t key_size;      // Tamaño de clave (128/192/256 bits)
  aes_type_t aes_type;    // Tipo de AES (128 o 256)
} aes_session;

// Function declarations

void print_hex(const char* label, const uint8_t* data, size_t len);
void print_label_int(const char* label, size_t value);
void get_random_buffer(uint8_t* buf, size_t len);

key_material_t key_setup(unsigned char* pSKValue, int is_msk);
key_send_t derive_key(unsigned char* key_value, key_type_t key_type);
char* encrypt_save_MudURL(char* mUDuRLValue);
char* encrypt_save_certificate(char* certificate, size_t len_certificate);

void installPSK();
void installMudURL();
void installCertificate();
void getMudURL();
void getCertificate();
void derive_psk_msk();
void sign();
void encrypt();
void decrypt();
void gen_random();

#endif // MAIN_H 