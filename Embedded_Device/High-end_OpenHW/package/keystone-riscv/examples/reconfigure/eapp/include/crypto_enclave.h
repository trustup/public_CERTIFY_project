#include "main_eapp.h"

int AES_init(const uint8_t *key, aes_session *session, uint32_t key_size_bits, int requires_iv, uint32_t operation_mode, aes_type_t aes_type);
int AES_cipher(aes_session *session, const uint8_t *src, size_t src_sz, uint8_t *dst, int cipher_mode);
int AES_terminate(aes_session *session);

int encrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);
int decrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);
int encrypt_with_sealing_key_aes256(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);
int decrypt_with_sealing_key_aes256(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);
int encrypt_with_sealing_key_aes256_64(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);
int decrypt_with_sealing_key_aes256_64(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output);