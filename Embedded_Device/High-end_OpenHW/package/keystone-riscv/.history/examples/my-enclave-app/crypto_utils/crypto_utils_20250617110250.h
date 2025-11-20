#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stdint.h>
#include <stddef.h>
// #include "aes.h"

// MD5 Constants
#define MD5_BLOCK_SIZE 64
#define MD5_DIGEST_SIZE 16

// MD5 Context structure
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[MD5_BLOCK_SIZE];
} MD5_CTX;

// MD5 Functions
void md5_init(MD5_CTX *ctx);
void md5_update(MD5_CTX *ctx, const uint8_t *data, size_t len);
void md5_final(uint8_t digest[MD5_DIGEST_SIZE], MD5_CTX *ctx);
void md5(const uint8_t *data, size_t len, uint8_t digest[MD5_DIGEST_SIZE]);

// HMAC-MD5 Functions
void hmac_md5_init(MD5_CTX *ctx, const uint8_t *key, size_t key_len);
void hmac_md5_update(MD5_CTX *ctx, const uint8_t *data, size_t len);
void hmac_md5_final(uint8_t digest[MD5_DIGEST_SIZE], MD5_CTX *ctx, const uint8_t *key, size_t key_len);
void hmac_md5(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t digest[MD5_DIGEST_SIZE]);

// CMAC-AES Functions
// void cmac_aes_init(struct AES_ctx *ctx, const uint8_t *key, size_t key_len);
// void cmac_aes_update(struct AES_ctx *ctx, const uint8_t *data, size_t len, uint8_t *mac);
// void cmac_aes_final(struct AES_ctx *ctx, uint8_t *mac);
void cmac_aes(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t mac[16]);

#endif // CRYPTO_UTILS_H 