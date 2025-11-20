#include "crypto_enclave.h"

int AES_init(const uint8_t *key, aes_session *session, uint32_t key_size_bits, int requires_iv, uint32_t operation_mode, aes_type_t aes_type) {
  session->key_size = key_size_bits;
  session->mode = operation_mode;
  session->aes_type = aes_type;
  memset(session->iv, 0, sizeof(session->iv));

  if (aes_type == AES_TYPE_128) {
        if (requires_iv) {
            AES_init_ctx_iv(&session->ctx, key, session->iv);
         } else {
            AES_init_ctx(&session->ctx, key);
         }
    } else if (aes_type == AES_TYPE_256) {
        if (requires_iv){
          AES256_init_ctx_iv(&session->ctx256, key, session->iv);
        } else {
            AES256_init_ctx(&session->ctx256, key);
        }
    } else {
      return -1;
    }
  return 0;
}

int AES_cipher(aes_session *session, const uint8_t *src, size_t src_sz, uint8_t *dst, int cipher_mode) {
    //aes_type_t aes_type = session->aes_type;
    size_t padded_sz = src_sz;
    uint8_t padded_input[256]; // Asumiendo un máximo de 240 bytes + 16 de padding. Ajusta según necesidad.

    if (session->mode == 0) // Modo cifrado
    {
        // Calcular cuántos bytes de padding se necesitan (PKCS#7)
        size_t padding_len = AES_BLOCKLEN - (src_sz % AES_BLOCKLEN);
        if (padding_len == 0) padding_len = AES_BLOCKLEN;

        padded_sz = src_sz + padding_len;

        // Copiar datos originales
        if (padded_sz > sizeof(padded_input)) {
            return -1; // Error: buffer interno insuficiente
        }

        memcpy(padded_input, src, src_sz);

        // Aplicar padding PKCS#7
        memset(padded_input + src_sz, padding_len, padding_len);

        // Copiar a dst para que tinyAES opere in-place
        memcpy(dst, padded_input, padded_sz);

        if (session->aes_type == AES_TYPE_128) {
          if (cipher_mode == AES_MODE_CBC)
              AES_CBC_encrypt_buffer(&session->ctx, dst, padded_sz);
          else if (cipher_mode == AES_MODE_ECB)
              for (size_t i = 0; i < padded_sz; i += AES_BLOCKLEN)
                  AES_ECB_encrypt(&session->ctx, dst + i);
          else
              return -2;
        } else if (session->aes_type == AES_TYPE_256) {
            if (cipher_mode == AES_MODE_CBC)
                AES256_CBC_encrypt_buffer(&session->ctx256, dst, padded_sz);
            else if (cipher_mode == AES_MODE_ECB)
                for (size_t i = 0; i < padded_sz; i += AES_BLOCKLEN)
                    AES256_ECB_encrypt(&session->ctx256, dst + i);
            else
                return -2;
      }
    }
    else if (session->mode == 1) // Modo descifrado
    {
        if (src_sz % AES_BLOCKLEN != 0)
        {
            return -1; // El tamaño cifrado debe ser múltiplo de bloque
        }

        memcpy(dst, src, src_sz);
        if (session->aes_type == AES_TYPE_128) {
          if (cipher_mode == AES_MODE_CBC)
              AES_CBC_decrypt_buffer(&session->ctx, dst, padded_sz);
          else if (cipher_mode == AES_MODE_ECB)
              for (size_t i = 0; i < padded_sz; i += AES_BLOCKLEN)
                  AES_ECB_decrypt(&session->ctx, dst + i);
          else
              return -2;
        } else if (session->aes_type == AES_TYPE_256) {
            if (cipher_mode == AES_MODE_CBC)
                AES256_CBC_decrypt_buffer(&session->ctx256, dst, padded_sz);
            else if (cipher_mode == AES_MODE_ECB)
                for (size_t i = 0; i < padded_sz; i += AES_BLOCKLEN)
                    AES256_ECB_decrypt(&session->ctx256, dst + i);
            else
                return -2;
        }
        // Remover el padding PKCS#7 después del descifrado
        size_t pad_len = dst[src_sz - 1];
        if (pad_len > 0 && pad_len <= AES_BLOCKLEN)
            src_sz -= pad_len;
    }

    return 0;
}

int AES_terminate(aes_session *session) {
  // aes_type_t aes_type = session->aes_type;
  if (session->aes_type == AES_TYPE_128) {
        memset(&session->ctx, 0, sizeof(session->ctx));
    } else if (session->aes_type == AES_TYPE_256) {
        memset(&session->ctx256, 0, sizeof(session->ctx256));
    }
  memset(session->iv, 0, sizeof(session->iv));
  return 0;
}

int encrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  struct sealing_key key_buffer;

  if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_identifier, strlen(key_identifier)) != 0) {
    ocall_print_string("Error obtaining sealing key");
    return -1;
  }
  print_hex("SealingKey used for encryption", key_buffer.key, 16);
  aes_session seal_sess;
  if (AES_init(key_buffer.key, &seal_sess, SEALING_KEY_BITS_LEN, 1,  0, AES_TYPE_128) == -1) {
    ocall_print_string("Error initializing encryption session with SealingKey");
    return -1;
  }

  if (AES_cipher(&seal_sess, input, input_len, output, AES_MODE_CBC) == -1) {
    ocall_print_string("Error encrypting with SealingKey");
    AES_terminate(&seal_sess);
    return -1;
  }

  AES_terminate(&seal_sess);
  return 0;
}

int decrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  struct sealing_key key_buffer;

  if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_identifier, strlen(key_identifier)) != 0) {
    ocall_print_string("Error obtaining sealing key");
    return -1;
  }
  print_hex("SealingKey used for decryption", key_buffer.key, 16);
  aes_session desseal_sess;
  if (AES_init(key_buffer.key, &desseal_sess, SEALING_KEY_BITS_LEN, 1,  1, AES_TYPE_128) == -1) {
    ocall_print_string("Error initializing decryption session with SealingKey");
    return -1;
  }

  if (AES_cipher(&desseal_sess, input, input_len, output, AES_MODE_CBC) == -1) {
    ocall_print_string("Error decrypting with SealingKey");
    AES_terminate(&desseal_sess);
    return -1;
  }

  AES_terminate(&desseal_sess);
  return 0;
}

int encrypt_with_sealing_key_aes256(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  struct sealing_key key_buffer;

  if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_identifier, strlen(key_identifier)) != 0) {
    ocall_print_string("Error obtaining sealing key");
    return -1;
  }
  print_hex("SealingKey used for encryption AES-256", key_buffer.key, 32);
  aes_session seal_sess;
  // Usar los primeros 32 bytes de la sealing key
  if (AES_init(key_buffer.key, &seal_sess, 256, 1, 0, AES_TYPE_256) == -1) {
    ocall_print_string("Error initializing encryption session AES-256 with SealingKey");
    return -1;
  }

  if (AES_cipher(&seal_sess, input, input_len, output, AES_MODE_CBC) == -1) {
    ocall_print_string("Error encrypting with SealingKey AES-256");
    AES_terminate(&seal_sess);
    return -1;
  }

  AES_terminate(&seal_sess);
  return 0;
}

int decrypt_with_sealing_key_aes256(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  struct sealing_key key_buffer;

  if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_identifier, strlen(key_identifier)) != 0) {
    ocall_print_string("Error obtaining sealing key");
    return -1;
  }
  print_hex("SealingKey used for decryption AES-256", key_buffer.key, 32);
  aes_session desseal_sess;
  // Usar los primeros 32 bytes de la sealing key
  if (AES_init(key_buffer.key, &desseal_sess, 256, 1, 1, AES_TYPE_256) == -1) {
    ocall_print_string("Error initializing decryption session AES-256 with SealingKey");
    return -1;
  }

  if (AES_cipher(&desseal_sess, input, input_len, output, AES_MODE_CBC) == -1) {
    ocall_print_string("Error decrypting with SealingKey AES-256");
    AES_terminate(&desseal_sess);
    return -1;
  }

  AES_terminate(&desseal_sess);
  return 0;
}

// Cifra 64 bytes en dos bloques de 32 bytes cada uno usando AES-256 y concatena el resultado
int encrypt_with_sealing_key_aes256_64(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  if (input_len != 64) {
    ocall_print_string("encrypt_with_sealing_key_aes256_64: input_len must be 64");
    return -1;
  }
  int res = 0;
  // Cifrar los primeros 32 bytes
  res = encrypt_with_sealing_key_aes256(key_identifier, input, 32, output);
  if (res != 0) return res;
  // Cifrar los siguientes 32 bytes
  res = encrypt_with_sealing_key_aes256(key_identifier, input + 32, 32, output + 32);
  return res;
}

// Descifra 64 bytes en dos bloques de 32 bytes cada uno usando AES-256 y concatena el resultado
int decrypt_with_sealing_key_aes256_64(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  if (input_len != 64) {
    ocall_print_string("decrypt_with_sealing_key_aes256_64: input_len must be 64");
    return -1;
  }
  int res = 0;
  // Descifrar los primeros 32 bytes
  res = decrypt_with_sealing_key_aes256(key_identifier, input, 32, output);
  if (res != 0) return res;
  // Descifrar los siguientes 32 bytes
  res = decrypt_with_sealing_key_aes256(key_identifier, input + 32, 32, output + 32);
  return res;
}
