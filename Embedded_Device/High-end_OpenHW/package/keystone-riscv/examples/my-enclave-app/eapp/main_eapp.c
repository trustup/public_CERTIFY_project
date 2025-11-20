//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "main_eapp.h"

// Agregar defines para los modos de cifrado al inicio del archivo (después de includes)

void print_hex(const char* label, const uint8_t* data, size_t len) {
  const char hex_chars[] = "0123456789abcdef";
  static char hex_str[512];  // Asegúrate de que sea lo suficientemente grande
  size_t max_len = sizeof(hex_str) - 1;

  size_t pos = 0;

  // Copiar etiqueta
  if (label) {
      while (*label && pos < max_len) {
          hex_str[pos++] = *label++;
      }
      if (pos < max_len) hex_str[pos++] = ':';  // Separador
      if (pos < max_len) hex_str[pos++] = ' ';
  }

  // Convertir datos a hex
  for (size_t i = 0; i < len && pos + 2 < max_len; i++) {
      hex_str[pos++] = hex_chars[(data[i] >> 4) & 0xF];
      hex_str[pos++] = hex_chars[data[i] & 0xF];
  }

  hex_str[pos] = '\0';
  ocall_print_string(hex_str);
}

// Función auxiliar para imprimir enteros sin snprintf/sprintf
void print_label_int(const char* label, size_t value) {
    char buf[64];
    char* p = buf;
    // Copia la etiqueta
    while (*label) *p++ = *label++;
    *p++ = ':';
    *p++ = ' ';
    // Convierte el entero a string (solo positivo)
    char tmp[32];
    int i = 0;
    size_t v = value;
    if (v == 0) {
        *p++ = '0';
    } else {
        while (v > 0 && i < 31) {
            tmp[i++] = '0' + (v % 10);
            v /= 10;
        }
        while (i > 0) *p++ = tmp[--i];
    }
    *p = 0;
    ocall_print_string(buf);
}

void get_random_buffer(uint8_t* buf, size_t len) {
  for (size_t i = 0; i < len; i += sizeof(int)) {
    int r = get_random();
    size_t copy_len = (len - i < sizeof(int)) ? (len - i) : sizeof(int);
    memcpy(buf + i, &r, copy_len);
  }
}

// Criptography Functions
key_material_t key_setup(unsigned char* pSKValue, int is_msk) {
  key_material_t keys;
  keys.type = is_msk ? 1 : 0;
  if (!pSKValue) {
    ocall_print_string("Invalid PSK input");
    return keys;
  }

  char input_block[AES128_BLOCK_SIZE] = { 0 };
  char inter_block[AES128_BLOCK_SIZE] = { 0 };
  char inter_block_b[AES128_BLOCK_SIZE] = { 0 };
  char ak[AES128_BLOCK_SIZE] = { 0 };
  char kdk[AES128_BLOCK_SIZE] = { 0 };

  size_t dst_sz;

  aes_session sess;
  int res = AES_init(pSKValue, &sess, AES128_BLOCK_SIZE * 8, 1, 0, AES_TYPE_128);
  if(res == -1){
    ocall_print_string("Error on init key_setup");
    return keys;
  }
  dst_sz = AES128_BLOCK_SIZE;
  res = AES_cipher(&sess, input_block, AES128_BLOCK_SIZE, inter_block, AES_MODE_ECB);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return keys;
  }

  memmove(inter_block_b,inter_block,AES128_BLOCK_SIZE);
  inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x01;
  // Cifrar para obtener AK
  res = AES_cipher(&sess, inter_block_b, AES128_BLOCK_SIZE, ak, AES_MODE_ECB);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return keys;
  }

  print_hex("AK generado", (uint8_t*)ak, AES128_BLOCK_SIZE);

  memmove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
  inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x02;

  res = AES_cipher(&sess, inter_block_b, AES128_BLOCK_SIZE, kdk, AES_MODE_ECB);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return keys;
  }
  print_hex("KDK generado", (uint8_t*)kdk, AES128_BLOCK_SIZE);

  AES_terminate(&sess);

  unsigned char encrypted_ak[AES128_BLOCK_SIZE];
  unsigned char encrypted_kdk[AES128_BLOCK_SIZE];

  if (encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, (uint8_t*)ak, AES128_BLOCK_SIZE, encrypted_ak) == -1 ||
    encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, (uint8_t*)kdk, AES128_BLOCK_SIZE, encrypted_kdk) == -1) {
    return keys;
  }

  memcpy(keys.ak, encrypted_ak, AES128_BLOCK_SIZE);
  memcpy(keys.kdk, encrypted_kdk, AES128_BLOCK_SIZE);

  print_hex("AK cifrado", (uint8_t*)encrypted_ak, AES128_BLOCK_SIZE);
  print_hex("kdk cifrado", (uint8_t*)encrypted_kdk, AES128_BLOCK_SIZE);

  // Enviar el tipo de material (PSK o MSK) a través de la estructura
  ocall_send_key_material(&keys);

  unsigned char desencrypted_ak[AES128_BLOCK_SIZE];
  unsigned char desencrypted_kdk[AES128_BLOCK_SIZE];

  // Descifrar (debug)
  if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_kdk, AES128_BLOCK_SIZE, desencrypted_kdk) == -1 || 
  decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_ak, AES128_BLOCK_SIZE, desencrypted_ak) == -1 ) {
    return keys;
  }

  print_hex("AK descifrado", (uint8_t*)desencrypted_ak, AES128_BLOCK_SIZE);
  print_hex("kdk descifrado", (uint8_t*)desencrypted_kdk, AES128_BLOCK_SIZE);

  return keys;
}

key_send_t derive_key(unsigned char* key_value, key_type_t key_type){
  key_send_t derived_key;
  if (!key_value) {
    ocall_print_string("Invalid KDK input");
    return derived_key;
  }

  char input_block[AES128_BLOCK_SIZE] = {0};
  // if (key_type != KEY_TYPE_MSK) {
  //   // 94 85 82 e6 e8 54 2d 22 4b 92 7f f4 ed a4 27 38
    
  //   char psk_block[AES128_BLOCK_SIZE] = {0x2a, 0x70, 0x6f, 0x77, 0x95, 0x12, 0x9c, 0x6f, 0x19, 0x8a, 0xaa, 0x1e, 0x5f, 0xe0, 0xfa, 0xef};
  //   memcpy(input_block, psk_block, AES128_BLOCK_SIZE);
  // } else {
  //   // 2a 70 6f 77 95 12 9c 6f 19 8a aa 1e 5f e0 fa ef
  //   char msk_block[AES128_BLOCK_SIZE] = {0x94, 0x85, 0x82, 0xe6, 0xe8, 0x54, 0x2d, 0x22, 0x4b, 0x92, 0x7f, 0xf4, 0xed, 0xa4, 0x27, 0x38};
  //   memcpy(input_block, msk_block, AES128_BLOCK_SIZE);
  // }

  char inter_block[AES128_BLOCK_SIZE] = { 0 };
  char inter_block_b[AES128_BLOCK_SIZE] = { 0 };
  char key[AES128_BLOCK_SIZE * 4] = { 0 };

  size_t dst_sz;

  aes_session sess; 
  int res = AES_init(key_value, &sess, AES128_BLOCK_SIZE * 8, 1, 0, AES_TYPE_128);
  if(res == -1){
    ocall_print_string("Error on init Derive Key");
    return derived_key;
  }
  dst_sz = AES128_BLOCK_SIZE;
  res = AES_cipher(&sess, input_block, AES128_BLOCK_SIZE, inter_block, AES_MODE_CBC);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return derived_key;
  }

  memmove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
  inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x01;

  // Cifrar para obtener EDK derivado
  for (uint8_t i = 0; i < 4; i++) {
    memmove(inter_block_b,inter_block,AES128_BLOCK_SIZE);
    inter_block_b[AES128_BLOCK_SIZE-1] ^= (0x02 + i);
    res = AES_cipher(&sess, inter_block_b, AES128_BLOCK_SIZE, key + AES128_BLOCK_SIZE * i, AES_MODE_ECB);
    if (res == -1) {
      ocall_print_string("AES cipher failed");
      return derived_key;
    }
  }

  print_hex("KEY derivado generado", (uint8_t*)key, AES128_BLOCK_SIZE * 4);

  AES_terminate(&sess);

  unsigned char encrypted_key[AES128_BLOCK_SIZE * 4];
  const char* key_id = (key_type == KEY_TYPE_MSK) ? KEY_ID_MSK : KEY_ID_EDK;
  if (encrypt_with_sealing_key(key_id, (uint8_t*)key, AES128_BLOCK_SIZE * 4, encrypted_key) == -1) {
    return derived_key;
  }

  // Guardar el EDK/MSK cifrado en la estructura
  memcpy(derived_key.key, key, AES128_BLOCK_SIZE * 4);
  derived_key.type = key_type;

  print_hex("KEY derivado cifrado", (uint8_t*)encrypted_key, AES128_BLOCK_SIZE * 4);

  unsigned char desencrypted_key[AES128_BLOCK_SIZE * 4];

  // Descifrar (debug)
  if (decrypt_with_sealing_key(key_id, encrypted_key, AES128_BLOCK_SIZE * 4, desencrypted_key) == -1) {
    return derived_key;
  }

  // print_hex("KEY derivado descifrado", (uint8_t*)desencrypted_key, AES128_BLOCK_SIZE * 4);

  // print_hex("&&&&&&&&&&&&&&&&&&&&&&&&&&&ANTES DE DEVOLVER EL DERIVED_KEY&&&&&&&&&&&&&&&&&&&&", (uint8_t*)derived_key.key, AES128_BLOCK_SIZE*4);

  return derived_key;
}

char* encrypt_save_MudURL(char* mUDuRLValue) {
  // Calcular el tamaño necesario con padding
  unsigned char encrypted_mudUrl[64];
  size_t original_len = strlen(mUDuRLValue);
  print_hex("mudUrl hex sin cifrar", (uint8_t*)mUDuRLValue, strlen(mUDuRLValue));

  encrypt_with_sealing_key(KEY_ID_MUD_URL, mUDuRLValue, 64, encrypted_mudUrl);
  print_hex("mudUrl hex cifrado", (uint8_t*)encrypted_mudUrl, sizeof(encrypted_mudUrl));

  ocall_send_mudUrl_string(encrypted_mudUrl);

  unsigned char decrypted_mudUrl[64];  // Igual al tamaño cifrado

  
  decrypt_with_sealing_key(KEY_ID_MUD_URL, encrypted_mudUrl, 64, decrypted_mudUrl);
  ocall_print_string("mudUrl descifrado:");
  ocall_print_string(decrypted_mudUrl);

  return (char *)encrypted_mudUrl;
}

char* encrypt_save_certificate(char* certificate, size_t len_certificate){
  // Calcular el tamaño necesario con padding
   unsigned char * encrypted_certificate = malloc(len_certificate);
  size_t original_len = strlen(certificate);
  print_hex("certificado hex sin cifrar", (uint8_t*)certificate, strlen(certificate));

  encrypt_with_sealing_key(KEY_ID_CERTIFICATE, certificate, len_certificate, encrypted_certificate);
  print_hex("certificado hex cifrado", (uint8_t*)encrypted_certificate, sizeof(encrypted_certificate));

  ocall_send_certificate_string(encrypted_certificate);

  unsigned char * decrypted_certificate = malloc(len_certificate);

  
  decrypt_with_sealing_key(KEY_ID_CERTIFICATE, encrypted_certificate, len_certificate, decrypted_certificate);
  ocall_print_string("certificado descifrado:");
  ocall_print_string(decrypted_certificate);

  return (char *)decrypted_certificate;
}

// Implementación de la función (puede ir antes o después del switch principal)
// Cambiar la firma de los handlers para recibir el buffer y tamaño
void installPSK(void* data, size_t size) {
    void *psk_buf = malloc(size);
    memcpy(psk_buf, data, size);
    ocall_print_string("PSK recibido:");
    ocall_print_string(psk_buf);
    key_material_t key_material = key_setup(psk_buf, 0);
    free(psk_buf);
}

void installMudURL(void* data, size_t size) {
    void *mudUrl_buf = malloc(size);
    memcpy(mudUrl_buf, data, size);
    ocall_print_string("MUD URL recibido:");
    ocall_print_string(mudUrl_buf);
    char* encrypted_mudUrl = encrypt_save_MudURL(mudUrl_buf);
    (void)encrypted_mudUrl;
    free(mudUrl_buf);
}

void installCertificate(void* data, size_t size) {
    void *cert_buf = malloc(size);
    memcpy(cert_buf, data, size);
    ocall_print_string("Certificado recibido:");
    ocall_print_string(cert_buf);
    char* encrypted_cert = encrypt_save_certificate(cert_buf, size);
    (void)encrypted_cert;
    free(cert_buf);
}

void getMudURL(void* data, size_t size) {
    // Print del puntero y los primeros bytes del buffer recibido
    ocall_print_string("[EAPP] getMudURL: llamada");
    print_hex("[EAPP] getMudURL: primeros bytes de data", data, 64);
    if (size < sizeof(void*) + sizeof(size_t)) {
        ocall_print_string("[EAPP] Error: tamaño de buffer insuficiente en getMudURL");
        return;
    }
    uintptr_t dest_ptr = 0;
    size_t mudurl_cifrado_size = 0;
    uint8_t* ptr = (uint8_t*)data;
    ocall_print_string("Piki Piki 1");
    memcpy(&dest_ptr, ptr, sizeof(void*)); ptr += sizeof(void*);
    ocall_print_string("Piki Piki 2");
    memcpy(&mudurl_cifrado_size, ptr, sizeof(size_t)); ptr += sizeof(size_t);
    ocall_print_string("Piki Piki 3");

    print_label_int("[EAPP] mudurl_cifrado_size (recibido)", mudurl_cifrado_size);
    print_hex("MudUrl Cifrado (en buffer)", ptr, mudurl_cifrado_size);
    ocall_send_random(mudurl_cifrado_size);
    void* encrypted_data = ptr;
    ocall_print_string("Piki Piki 4");
    void* decrypted_mudUrl = malloc(mudurl_cifrado_size);
    ocall_print_string("Piki Piki 5");
    decrypt_with_sealing_key(KEY_ID_MUD_URL, encrypted_data, mudurl_cifrado_size, decrypted_mudUrl);
    ocall_print_string("mudUrl descifrado:");
    ocall_print_string(decrypted_mudUrl);
    // Calcular el tamaño real del string descifrado
    size_t mudurl_descifrado_size = strlen((char*)decrypted_mudUrl) + 1;
    print_label_int("[EAPP] mudurl_descifrado_size (strlen+1)", mudurl_descifrado_size);
    print_hex("[EAPP] primeros bytes de mudUrl descifrado", (uint8_t*)decrypted_mudUrl, mudurl_descifrado_size > 32 ? 32 : mudurl_descifrado_size);
    ocall_write_to_host_ptr((void*)dest_ptr, decrypted_mudUrl, mudurl_descifrado_size);
    free(decrypted_mudUrl);
}

void getCertificate(void* data, size_t size) {
    print_hex("Certificado Cifrado", data, size);
    void * decrypted_cert = malloc(size);
    decrypt_with_sealing_key(KEY_ID_CERTIFICATE, data, size, decrypted_cert);
    ocall_print_string("Certificado descifrado:");
    ocall_print_string(decrypted_cert);
    free(decrypted_cert);
}

void derive_psk_msk(void* data, size_t size) {
    int session = *(int*)data;
    uint8_t* ptr = (uint8_t*)data + sizeof(int);
    int num_iterations = (session == SESSION_MSK) ? 1 : 1;
    for (int i = 0; i < num_iterations; ++i) {
        unsigned char * key_base = malloc(PSK_SIZE);
        memcpy(key_base, ptr, PSK_SIZE);
        ocall_print_string("Sesión recibida:");
        if (session == SESSION_PSK_KDK) {
            ocall_print_string("0 (PSK/KDK)");
        } else if (session == SESSION_MSK) {
            ocall_print_string("1 (MSK)");
        } else {
            ocall_print_string("Sesión desconocida");
        }
        ocall_print_string("Clave base recibida:");
        print_hex("Clave base", key_base, PSK_SIZE);
        unsigned char * decrypted_key_base = malloc(PSK_SIZE);
        decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, key_base, PSK_SIZE, decrypted_key_base);
        print_hex("Clave base descifrada", decrypted_key_base, PSK_SIZE);
        key_send_t derived_key;
        if (session == SESSION_PSK_KDK) {
            derived_key = derive_key(decrypted_key_base, KEY_TYPE_MSK);
            derived_key.type = KEY_TYPE_MSK;
            uint8_t digest[32];
            struct Sha_256 sha_256;
            sha_256_init(&sha_256, digest);
            // print_hex("########################### DERIVED_KEY#################", derived_key.key, 64);
            sha_256_write(&sha_256, derived_key.key, 64);
            uint8_t* hash_result = sha_256_close(&sha_256);
            print_hex("SHA256 digest del MSK derivado (por sha_256_close)", hash_result, 32);
            key_setup(digest, 1);
            ocall_send_key(&derived_key, sizeof(key_send_t));
        } else if (session == SESSION_MSK) {
            derived_key = derive_key(decrypted_key_base, KEY_TYPE_EDK);
            derived_key.type = KEY_TYPE_EDK;
            ocall_send_key(&derived_key, sizeof(key_send_t));
        }
        free(key_base);
        free(decrypted_key_base);
    }
}

void sign(void* data, size_t size) {
    uint8_t *sign_ptr = (uint8_t*)data;
    size_t key_id_len = *(size_t*)sign_ptr;
    sign_ptr += sizeof(size_t);
    uint8_t *key_id_encrypted = malloc(key_id_len);
    memcpy(key_id_encrypted, sign_ptr, key_id_len);
    sign_ptr += key_id_len;
    print_hex("Clave de firma cifrada", key_id_encrypted, key_id_len);
    uint8_t *key_id = malloc(key_id_len);
    if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, key_id_encrypted, key_id_len, key_id) == -1) {
        ocall_print_string("Error descifrando key_id para firma");
        free(key_id_encrypted);
        free(key_id);
        return;
    }
    free(key_id_encrypted);
    size_t algorithm_len = *(size_t*)sign_ptr;
    sign_ptr += sizeof(size_t);
    char *algorithm = malloc(algorithm_len + 1);
    memcpy(algorithm, sign_ptr, algorithm_len);
    // --- Lógica para algoritmo configurable ---
    if (strcmp(algorithm, "DEFAULT") == 0) {
        ocall_print_string("[EAPP] Algoritmo es DEFAULT, usando CMAC_AES por defecto");
        size_t len = strlen(CMAC_AES);
        memcpy(algorithm, CMAC_AES, len);
        algorithm[len] = '\0';
        ocall_print_string("[EAPP] Algoritmo FINAL utilizado:");
        ocall_print_string(algorithm);
    } else {
        ocall_print_string("[EAPP] Algoritmo recibido cifrado");
        print_label_int("[EAPP] algorithm_len", algorithm_len);
        ocall_print_string("[EAPP] Algoritmo FINAL utilizado del else:");
        ocall_print_string(algorithm);
        // Descifrar el algoritmo con la sealing key
        print_hex("[EAPP] ALGORITHM CIFRADO", algorithm, 16);
        char decrypted_algo[16] = {0};
        if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, algorithm, 16, decrypted_algo) == -1) {
            ocall_print_string("[EAPP] Error descifrando algoritmo, usando CMAC_AES por defecto");
            size_t len = strlen(CMAC_AES);
            memcpy(algorithm, CMAC_AES, len);
            algorithm[len] = '\0';
            ocall_print_string("[EAPP] Algoritmo FINAL utilizado:");
            ocall_print_string(algorithm);
            print_hex("[EAPP] ALGORITHM DESCIFRADO", decrypted_algo, 16);
        } else {
            ocall_print_string("[EAPP] Algoritmo descifrado:");
            ocall_print_string(decrypted_algo);
            print_label_int("[EAPP] algorithm_len", algorithm_len);
            decrypted_algo[algorithm_len] = '\0';
            if (strcmp(decrypted_algo, HMAC_MD5) == 0 || strcmp(decrypted_algo, CMAC_AES) == 0) {
                ocall_print_string("[EAPP] Algoritmo descifrado reconocido:");
                ocall_print_string(decrypted_algo);
                size_t len = strlen(decrypted_algo);
                memcpy(algorithm, decrypted_algo, len);
                algorithm[len] = '\0';
                ocall_print_string("[EAPP] Algoritmo FINAL utilizado:");
                ocall_print_string(algorithm);
            } else {
                ocall_print_string("[EAPP] Algoritmo descifrado NO reconocido, usando CMAC_AES por defecto:");
                ocall_print_string(decrypted_algo);
                size_t len = strlen(CMAC_AES);
                memcpy(algorithm, CMAC_AES, len);
                algorithm[len] = '\0';
                ocall_print_string("[EAPP] Algoritmo FINAL utilizado:");
                ocall_print_string(algorithm);
            }
        }
    }
    sign_ptr += algorithm_len;
    size_t data_len = *(size_t*)sign_ptr;
    sign_ptr += sizeof(size_t);
    uint8_t *data_to_sign = malloc(data_len);
    memcpy(data_to_sign, sign_ptr, data_len);
    sign_ptr += data_len;
    // Recuperar el puntero signature_ptr
    void* signature_ptr = NULL;
    memcpy(&signature_ptr, sign_ptr, sizeof(void*));
    print_label_int("[EAPP] sign: valor de signature_ptr", (uintptr_t)signature_ptr);
    ocall_print_string("Algoritmo recibido:");
    ocall_print_string(algorithm);
    ocall_print_string("Datos a firmar:");
    print_hex("Clave de firma descifrada", key_id, key_id_len);
    print_hex("Datos a firmar (hex)", data_to_sign, data_len);
    uint8_t signature[SIGNATURE_SIZE];
    memset(signature, 0, SIGNATURE_SIZE);
    if (strcmp(algorithm, HMAC_MD5) == 0) {
        hmac_md5(key_id, key_id_len, data_to_sign, data_len, signature);
        ocall_print_string("Firma generada (solo HMAC-MD5):");
        print_hex("HMAC-MD5", signature, MD5_DIGEST_SIZE);
    } else if (strcmp(algorithm, CMAC_AES) == 0) {
        unsigned char mac_simple_2[16];
        AES_CMAC(key_id, data_to_sign, data_len, mac_simple_2);
        print_hex("CMAC ejemplo", mac_simple_2, 16);
        print_label_int("[EAPP] sign: tamaño de CMAC", 16);
        print_hex("[EAPP] sign: primeros bytes de CMAC", mac_simple_2, 16);
        // Escribir la CMAC en el puntero recibido usando OCALL
        ocall_write_to_host_ptr(signature_ptr, mac_simple_2, 16);
    } else {
        free(key_id);
        free(algorithm);
        free(data_to_sign);
        return;
    }
    free(key_id);
    free(algorithm);
    free(data_to_sign);
}

void encrypt(void* data, size_t size) {
    uint8_t *ptr = (uint8_t*)data;
    size_t keyId_len = *(size_t*)ptr;
    ptr += sizeof(size_t);
    uint8_t *keyId = malloc(keyId_len);
    memcpy(keyId, ptr, keyId_len);
    ptr += keyId_len;
    print_hex("[ENCRYPT] keyId recibido", keyId, keyId_len);
    print_label_int("[ENCRYPT] keyId_len", keyId_len);
    size_t encrypt_data_len = *(size_t*)ptr;
    ptr += sizeof(size_t);
    uint8_t *data_buf = malloc(encrypt_data_len);
    memcpy(data_buf, ptr, encrypt_data_len);
    print_hex("[ENCRYPT] Datos a cifrar", data_buf, encrypt_data_len);
    print_label_int("[ENCRYPT] encrypt_data_len", encrypt_data_len);
    size_t block_size = AES_BLOCKLEN;
    size_t padding_len = block_size - (encrypt_data_len % block_size);
    if (padding_len == 0) padding_len = block_size;
    size_t padded_len = encrypt_data_len + padding_len;
    print_label_int("[ENCRYPT] block_size", block_size);
    print_label_int("[ENCRYPT] padding_len", padding_len);
    print_label_int("[ENCRYPT] padded_len", padded_len);
    uint8_t *padded_data = malloc(padded_len);
    memcpy(padded_data, data_buf, encrypt_data_len);
    for (size_t i = 0; i < padding_len; ++i) {
        *(padded_data + encrypt_data_len + i) = (uint8_t)padding_len;
    }
    print_hex("[ENCRYPT] padded_data (con padding)", padded_data, padded_len);
    uint8_t *encrypted = malloc(padded_len);
    if (!encrypted) {
        ocall_print_string("Error al reservar memoria para encrypted");
        free(keyId);
        free(data_buf);
        free(padded_data);
        return;
    }
    aes_type_t aes_type = (keyId_len == 32) ? AES_TYPE_256 : AES_TYPE_128;
    print_label_int("[ENCRYPT] aes_type", aes_type);
    aes_session encrypt_sess;
    AES_init(keyId, &encrypt_sess, keyId_len * 8, 1, 0, aes_type);
    print_label_int("[ENCRYPT] aes_type", aes_type);
    int res = AES_cipher(&encrypt_sess, padded_data, padded_len, encrypted, AES_MODE_CBC);
    AES_terminate(&encrypt_sess);
    if (res != 0) {
        ocall_print_string("Error en cifrado directo con keyId");
        free(keyId);
        free(data_buf);
        free(padded_data);
        free(encrypted);
        return;
    }
    print_hex("[ENCRYPT] Datos cifrados (output)", encrypted, padded_len);
    size_t out_total_size = sizeof(size_t) + keyId_len + sizeof(size_t) + padded_len;
    uint8_t* out_buffer = malloc(out_total_size);
    if (!out_buffer) {
        ocall_print_string("Error al reservar memoria para out_buffer");
        free(keyId);
        free(data_buf);
        free(padded_data);
        free(encrypted);
        return;
    }
    uint8_t* out_ptr = out_buffer;
    memcpy(out_ptr, &keyId_len, sizeof(size_t)); out_ptr += sizeof(size_t);
    memcpy(out_ptr, keyId, keyId_len); out_ptr += keyId_len;
    memcpy(out_ptr, &padded_len, sizeof(size_t)); out_ptr += sizeof(size_t);
    memcpy(out_ptr, encrypted, padded_len);
    print_hex("[ENCRYPT] Mensaje cifrado (paquete)", out_buffer, out_total_size > 64 ? 64 : out_total_size);
    ocall_send_random_buffer(out_buffer, out_total_size);
    free(keyId);
    free(data_buf);
    free(padded_data);
    free(encrypted);
    free(out_buffer);
}

void decrypt(void* data, size_t size) {
    uint8_t *ptr = (uint8_t*)data;
    size_t keyId_len = *(size_t*)ptr;
    ptr += sizeof(size_t);
    uint8_t *keyId = malloc(keyId_len);
    memcpy(keyId, ptr, keyId_len);
    ptr += keyId_len;
    print_hex("[DECRYPT] keyId recibido", keyId, keyId_len);
    print_label_int("[DECRYPT] keyId_len", keyId_len);
    size_t decrypt_data_len = *(size_t*)ptr;
    ptr += sizeof(size_t);
    uint8_t *data_cifrada = malloc(decrypt_data_len);
    memcpy(data_cifrada, ptr, decrypt_data_len);
    print_hex("[DECRYPT] Datos cifrados recibidos", data_cifrada, decrypt_data_len);
    print_label_int("[DECRYPT] decrypt_data_len", decrypt_data_len);
    uint8_t *decrypted = malloc(decrypt_data_len);
    if (!decrypted) {
        ocall_print_string("Error al reservar memoria para decrypted");
        free(keyId);
        free(data_cifrada);
        return;
    }
    aes_type_t aes_type = (keyId_len == 32) ? AES_TYPE_256 : AES_TYPE_128;
    aes_session decrypt_sess;
    AES_init(keyId, &decrypt_sess, keyId_len * 8, 1, 1, aes_type);
    print_label_int("[DECRYPT] aes_type", aes_type);
    int res = AES_cipher(&decrypt_sess, data_cifrada, decrypt_data_len, decrypted, AES_MODE_CBC);
    AES_terminate(&decrypt_sess);
    if (res != 0) {
        ocall_print_string("Error en descifrado directo con keyId");
        free(keyId);
        free(data_cifrada);
        free(decrypted);
        return;
    }
    print_hex("[DECRYPT] Mensaje descifrado (con padding)", decrypted, decrypt_data_len);
    uint8_t pad = decrypted[decrypt_data_len - 1];
    size_t plain_len = decrypt_data_len;
    if (pad > 0 && pad <= AES_BLOCKLEN) {
        plain_len -= pad;
    }
    print_hex("[DECRYPT] Mensaje descifrado (sin padding)", decrypted, plain_len);
    print_label_int("[DECRYPT] Valor de padding", pad);
    ocall_send_random_buffer(decrypted, plain_len);
    free(keyId);
    free(data_cifrada);
    free(decrypted);
}

void gen_random(void* data, size_t size) {
    size_t random_len = *(size_t*)data;
    uint8_t* random_buf = malloc(random_len);
    get_random_buffer(random_buf, random_len);
    print_hex("Random generado", random_buf, random_len);
    ocall_send_random_buffer(random_buf, random_len);
    free(random_buf);
}

// --- NUEVO: Handler para reconfigure ---
void handle_reconfigure(void* data, size_t size) {
    if (size < 1) {
        ocall_print_string("[EAPP] CMD_RECONFIGURE: buffer demasiado pequeño");
        return;
    }
    uint8_t opcode = *(uint8_t*)data;
    ocall_print_string("[EAPP] CMD_RECONFIGURE: recibido");
    print_label_int("[EAPP] reconfigure opcode", opcode);
    if (size > 1) {
        print_hex("[EAPP] reconfigure data", (uint8_t*)data + 1, size - 1);
    } else {
        ocall_print_string("[EAPP] reconfigure: sin datos adicionales");
    }
    // Lógica según opcode
    if (opcode == 1) {
        // Cifrar los datos antes de escribir el archivo
        size_t data_len = size - 1;
        uint8_t* plain_data = (uint8_t*)data + 1;
        if (data_len > 64) data_len = 64; // Limitar a 64 bytes como en MudURL
        print_hex("[EAPP] reconfigure data (plain)", plain_data, data_len);
        // Calcular tamaño cifrado real (múltiplo de 16)
        size_t block_size = 16;
        size_t padding_len = block_size - (data_len % block_size);
        if (padding_len == 0) padding_len = block_size;
        size_t encrypted_len = data_len + padding_len;
        uint8_t encrypted_data[16] = {0};
        encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, plain_data, 16, encrypted_data);
        print_hex("[EAPP] reconfigure data (encrypted)", encrypted_data, encrypted_len);
        // Desencriptar para comprobar
        uint8_t decrypted_data[16] = {0};
        decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_data, 16, decrypted_data);
        print_hex("[EAPP] reconfigure data (decrypted)", decrypted_data, data_len);
        ocall_write_file(SIGN_ALGO_PATH, encrypted_data, encrypted_len);
    }
}

EAPP_ENTRY eapp_entry(){
  struct edge_data msg;
  ocall_wait_for_message(&msg);
  void *buf = malloc(msg.size);
  copy_from_shared(buf, msg.offset, msg.size);
  int cmd = *(int*)buf;
  print_label_int("[ENTRY] Comando recibido", cmd);
  uint8_t* ptr = (uint8_t*)buf + sizeof(int);
  size_t size_rest = msg.size - sizeof(int);
  switch (cmd) {
    case CMD_INSTALL_PSK:
      installPSK(ptr, size_rest);
      break;
    case CMD_INSTALL_MUD_URL:
      installMudURL(ptr, size_rest);
      break;
    case CMD_INSTALL_CERT:
      installCertificate(ptr, size_rest);
      break;
    case CMD_GET_MUD_URL:
      getMudURL(ptr, size_rest);
      break;
    case CMD_GET_CERT:
      getCertificate(ptr, size_rest);
      break;
    case CMD_DERIVE_KEY:
      derive_psk_msk(ptr, size_rest);
      break;
    case CMD_SIGN:
      sign(ptr, size_rest);
      break;
    case CMD_ENCRYPT:
      encrypt(ptr, size_rest);
      break;
    case CMD_DECRYPT:
      decrypt(ptr, size_rest);
      break;
    case CMD_GEN_RANDOM:
      gen_random(ptr, size_rest);
      break;
    case CMD_RECONFIGURE:
      ocall_print_string("Ejecutando CMD_RECONFIGURE");
      handle_reconfigure(ptr, size_rest);
      break;
    default:
      ocall_print_string("Comando desconocido");
      break;
  }
  free(buf);
  EAPP_RETURN(0);
}