#ifndef MAIN_H
#define MAIN_H

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
#include "util.h"
#include "crypto_utils.h"

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

// Definicion de estructura para la sesión de AES
typedef struct {
  struct AES_ctx ctx;     // Contexto de tiny-AES
  uint8_t iv[16];         // IV (para modos CBC/CTR)
  uint32_t mode;          // Ej: AES_ENCRYPT/AES_DECRYPT
  uint32_t key_size;      // Tamaño de clave (128/192/256 bits)
} aes_session;

// Function declarations
unsigned long ocall_send_mudUrl_string(char ocall_data[64]);
key_material_t installPSK(unsigned char* pSKValue);
unsigned long ocall_send_key_material(key_material_t* keys);
void ocall_wait_for_message(struct edge_data *msg);
unsigned long ocall_print_string(char* string);
unsigned long ocall_send_random(int random_value);
unsigned long ocall_send_certificate_string(char ocall_data[32]);

void print_hex(const char* label, const uint8_t* data, size_t len);
int encrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);
int decrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);

char* installMudURL(char* mUDuRLValue);
char* installCertificate(char* certificate, size_t len_certificate);

int AES_init(const uint8_t *key, aes_session *session, uint32_t key_size_bits, int requires_iv, uint32_t operation_mode);
int AES_cipher(aes_session *session, const uint8_t *src, size_t src_sz, uint8_t *dst);
int AES_terminate(aes_session *session);

#endif // MAIN_H 