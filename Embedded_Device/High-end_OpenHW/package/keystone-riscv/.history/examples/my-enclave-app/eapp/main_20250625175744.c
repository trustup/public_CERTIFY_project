//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "main.h"

// Agregar defines para los modos de cifrado al inicio del archivo (después de includes)
#define AES_MODE_CBC 0
#define AES_MODE_ECB 1

unsigned long ocall_send_mudUrl_string(char ocall_data[64]){
  unsigned long retval;
  ocall(OCALL_RECIVE_MUD_URL, ocall_data, 64, &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_send_key_material(key_material_t* keys){
  unsigned long retval;
  ocall(OCALL_RECIVE_KEY_MATERIALS, keys, sizeof(key_material_t), &retval ,sizeof(unsigned long));
  return retval;
}

void ocall_wait_for_message(struct edge_data *msg){
  ocall(OCALL_WAIT_FOR_MESSAGE, NULL, 0, msg, sizeof(struct edge_data));
}

unsigned long ocall_print_string(char* string){
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, strlen(string)+1, &retval ,sizeof(unsigned long));
  return retval;
}

unsigned long ocall_send_certificate_string(char ocall_data[32]){
  unsigned long retval;
  ocall(OCALL_RECIVE_CERTIFICATE, ocall_data, 32, &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_send_random(int random_value) {
  unsigned long retval;
  ocall(OCALL_SEND_RANDOM, &random_value, sizeof(int), &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_send_random_buffer(uint8_t* buf, size_t len) {
  unsigned long retval;
  ocall(OCALL_SEND_RANDOM_BUFFER, buf, len, &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_send_key(key_send_t* key, size_t size){
  unsigned long retval;
  if (key->type == KEY_TYPE_MSK) {
    ocall_print_string("Enviando MSK al host");
    ocall(OCALL_RECIVE_MSK, key, size, &retval ,sizeof(unsigned long));
  } else if (key->type == KEY_TYPE_EDK) {
    ocall_print_string("Enviando EDK al host");
    ocall(OCALL_RECIVE_EDK, key, size, &retval ,sizeof(unsigned long));
  } else {
    ocall_print_string("Tipo de clave desconocido");
    return -1;
  }
  return retval;
}

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

int encrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  struct sealing_key key_buffer;

  if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_identifier, strlen(key_identifier)) != 0) {
    //ocall_print("Error obteniendo sealing key");
    return -1;
  }
  print_hex("Sealing key utilizada para cifrado", key_buffer.key, 16);
  aes_session seal_sess;
  if (AES_init(key_buffer.key, &seal_sess, SEALING_KEY_BITS_LEN, 1,  0, AES_TYPE_128) == -1) {
    ocall_print_string("Error inicializando sesión de cifrado con sealing key");
    return -1;
  }

  if (AES_cipher(&seal_sess, input, input_len, output, AES_MODE_CBC) == -1) {
    ocall_print_string("Error cifrando con sealing key");
    AES_terminate(&seal_sess);
    return -1;
  }

  AES_terminate(&seal_sess);
  return 0;
}

int decrypt_with_sealing_key(const char* key_identifier, const uint8_t* input, size_t input_len, uint8_t* output) {
  struct sealing_key key_buffer;

  if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_identifier, strlen(key_identifier)) != 0) {
    //ocall_print("Error obteniendo sealing key");
    return -1;
  }
  print_hex("Sealing key utilizada para descifrado", key_buffer.key, 16);
  aes_session desseal_sess;
  if (AES_init(key_buffer.key, &desseal_sess, SEALING_KEY_BITS_LEN, 1,  1, AES_TYPE_128) == -1) {
    //ocall_print("Error inicializando sesión de descifrado con sealing key");
    return -1;
  }

  if (AES_cipher(&desseal_sess, input, input_len, output, AES_MODE_CBC) == -1) {
    //ocall_print("Error descifrando con sealing key");
    AES_terminate(&desseal_sess);
    return -1;
  }

  AES_terminate(&desseal_sess);
  return 0;
}

void get_random_buffer(uint8_t* buf, size_t len) {
  for (size_t i = 0; i < len; i += sizeof(int)) {
    int r = get_random();
    size_t copy_len = (len - i < sizeof(int)) ? (len - i) : sizeof(int);
    memcpy(buf + i, &r, copy_len);
  }
}

// Criptography Functions
key_material_t installPSK(unsigned char* pSKValue){
  key_material_t keys;
  if (!pSKValue) {
    ocall_print_string("Invalid PSK input");
    return;
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
    ocall_print_string("Error on init Install PSK");
    return;
  }
  dst_sz = AES128_BLOCK_SIZE;
	res = AES_cipher(&sess, input_block, AES128_BLOCK_SIZE, inter_block, AES_MODE_ECB);
    if (res == -1) {
        ocall_print_string("AES cipher failed");
        return;
    }

  memmove(inter_block_b,inter_block,AES128_BLOCK_SIZE);

  inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x01;
  // Cifrar para obtener AK
  res = AES_cipher(&sess, inter_block_b, AES128_BLOCK_SIZE, ak, AES_MODE_ECB);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return;
  }

  print_hex("AK generado", (uint8_t*)ak, AES128_BLOCK_SIZE);

  memmove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
  inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x02;

  res = AES_cipher(&sess, inter_block_b, AES128_BLOCK_SIZE, kdk, AES_MODE_ECB);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return;
  }
  print_hex("KDK generado", (uint8_t*)kdk, AES128_BLOCK_SIZE);

  AES_terminate(&sess);

  unsigned char encrypted_ak[AES128_BLOCK_SIZE];
  unsigned char encrypted_kdk[AES128_BLOCK_SIZE];

  if (encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, (uint8_t*)ak, AES128_BLOCK_SIZE, encrypted_ak) == -1 ||
    encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, (uint8_t*)kdk, AES128_BLOCK_SIZE, encrypted_kdk) == -1) {
    return;
  }

  memcpy(keys.ak, encrypted_ak, AES128_BLOCK_SIZE);
  memcpy(keys.kdk, encrypted_kdk, AES128_BLOCK_SIZE);


  print_hex("AK cifrado", (uint8_t*)encrypted_ak, AES128_BLOCK_SIZE);
  print_hex("kdk cifrado", (uint8_t*)encrypted_kdk, AES128_BLOCK_SIZE);

  ocall_send_key_material(&keys);

  unsigned char desencrypted_ak[AES128_BLOCK_SIZE];
  unsigned char desencrypted_kdk[AES128_BLOCK_SIZE];

  // Descifrar (debug)
  if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_kdk, AES128_BLOCK_SIZE, desencrypted_kdk) == -1 || 
  decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_ak, AES128_BLOCK_SIZE, desencrypted_ak) == -1 ) {
    return;
  }

  print_hex("AK descifrado", (uint8_t*)desencrypted_ak, AES128_BLOCK_SIZE);
  print_hex("kdk descifrado", (uint8_t*)desencrypted_kdk, AES128_BLOCK_SIZE);

  return keys;
}

key_send_t derive_key(unsigned char* key_value){
  key_send_t derived_key;
  if (!key_value) {
    ocall_print_string("Invalid KDK input");
    return derived_key;
  }

  char input_block[AES128_BLOCK_SIZE] = {0xb8, 0xae, 0x30, 0x9f, 0x2e, 0x8a, 0xdf, 0x65, 0xb6, 0x7b, 0x2e, 0x2e, 0xc0, 0x51, 0x23, 0xe8};
	//char input_block[AES128_BLOCK_SIZE] = { 0 };
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

  unsigned char encrypted_key[AES128_BLOCK_SIZE];

  if (encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, (uint8_t*)key, AES128_BLOCK_SIZE, encrypted_key) == -1) {
    return derived_key;
  }

  // Guardar el EDK cifrado en la estructura
  memcpy(derived_key.key, key, AES128_BLOCK_SIZE * 4);

  print_hex("KEY derivado cifrado", (uint8_t*)encrypted_key, AES128_BLOCK_SIZE);

  unsigned char desencrypted_key[AES128_BLOCK_SIZE];

  // Descifrar (debug)
  if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_key, AES128_BLOCK_SIZE, desencrypted_key) == -1) {
    return derived_key;
  }

  print_hex("KEY derivado descifrado", (uint8_t*)desencrypted_key, AES128_BLOCK_SIZE);

  print_hex("&&&&&&&&&&&&&&&&&&&&&&&&&&&ANTES DE DEVOLVER EL DERIVED_KEY&&&&&&&&&&&&&&&&&&&&", (uint8_t*)derived_key.key, AES128_BLOCK_SIZE*4);

  return derived_key;
}

char* installMudURL(char* mUDuRLValue) {
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

char* installCertificate(char* certificate, size_t len_certificate){
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

EAPP_ENTRY eapp_entry(){
  int offset_shared_mem = 0;
  char op_buf[4];  // Tamaño máximo esperado
  // offset 0 porque el host escribe desde el inicio
  int ret = copy_from_shared(op_buf, offset_shared_mem, sizeof(op_buf));

  if(ret != 0){
    //ocall_print("Error, reading op param)");
    EAPP_RETURN(1);
  }else{
    offset_shared_mem = offset_shared_mem + 4;
    op_buf[sizeof(op_buf) - 1] = '\0';
    //ocall_print_string(op_buf);
  }

  int cmd = atoi(op_buf); // Usamos nuestra versión de atoi

  switch (cmd) {
    case CMD_INSTALL_PSK:
      ocall_print_string("Ejecutando CMD_INSTALL_PSK");
      struct edge_data msg1;
      ocall_wait_for_message(&msg1);
      void * psk_buf = malloc(msg1.size);

      copy_from_shared(psk_buf, msg1.offset, msg1.size);

      ocall_print_string("PSK recibido:");
      ocall_print_string(psk_buf);

      key_material_t key_material = installPSK(psk_buf);

      break;
    case CMD_INSTALL_MUD_URL:
      ocall_print_string("Ejecutando CMD_INSTALL_MUD_URL");
      struct edge_data msg;
      ocall_wait_for_message(&msg);
      void * mudUrl_buf = malloc(msg.size);

      copy_from_shared(mudUrl_buf, msg.offset, msg.size);

      ocall_print_string("MUD URL recibido:");
      ocall_print_string(mudUrl_buf);

      char* encrypted_mudUrl = installMudURL(mudUrl_buf);
     
      break;
    case CMD_GET_MUD_URL:
      ocall_print_string("Ejecutando CMD_GET_MUD_URL");
      struct edge_data msg2;
      ocall_wait_for_message(&msg2);
       void * encrypt_mudUrl_buf = malloc(msg2.size);

      copy_from_shared(encrypt_mudUrl_buf, msg2.offset, msg2.size);

      print_hex("MudUrl Cifrado",encrypt_mudUrl_buf,msg2.size);

       void * decrypted_mudUrl = malloc(msg2.size);
  
      decrypt_with_sealing_key(KEY_ID_MUD_URL, encrypt_mudUrl_buf, msg2.size, decrypted_mudUrl);
      ocall_print_string("mudUrl descifrado:");
      ocall_print_string(decrypted_mudUrl);
      break;
    case CMD_INSTALL_CERT:
      ocall_print_string("Ejecutando CMD_INSTALL_CERT");
      struct edge_data msg3;
      ocall_wait_for_message(&msg3);
      // char cert_buf[64];
      void * cert_buf = malloc(msg3.size);

      copy_from_shared(cert_buf, msg3.offset, msg3.size);

      ocall_print_string("Certificado recibido:");
      ocall_print_string(cert_buf);

      char* encrypted_cert = installCertificate(cert_buf, msg3.size);
      break;
    case CMD_GET_CERT:
      ocall_print_string("Ejecutando CMD_GET_CERT");
      struct edge_data msg4;
      ocall_wait_for_message(&msg4);
      void * encrypt_cert_buf = malloc(msg4.size);


      copy_from_shared(encrypt_cert_buf, msg4.offset, msg4.size);

      print_hex("Certificado Cifrado",encrypt_cert_buf,msg4.size);

       void * decrypted_cert = malloc(msg4.size);
  
      decrypt_with_sealing_key(KEY_ID_CERTIFICATE, encrypt_cert_buf, msg4.size, decrypted_cert);
      ocall_print_string("Certificado descifrado:");
      ocall_print_string(decrypted_cert);
      break;
    case CMD_DERIVE_KEY:
      ocall_print_string("Ejecutando CMD_DERIVE_KEY");
      // Obtener datos de derivación desde el host: sesión y clave base
      struct edge_data msg_derive_key;
      int num_iterations = 1;
      int session_tmp = 0;
      // Esperar el primer mensaje para saber la sesión
      ocall_wait_for_message(&msg_derive_key);
      void * derive_data_buf = malloc(msg_derive_key.size);
      copy_from_shared(derive_data_buf, msg_derive_key.offset, msg_derive_key.size);
      uint8_t *ptr = (uint8_t*)derive_data_buf;
      session_tmp = *(int*)ptr;
      if (session_tmp == SESSION_MSK) num_iterations = 9;
      free(derive_data_buf);
      
      for (int i = 0; i < num_iterations; ++i) {
        if (i > 0) {
          ocall_wait_for_message(&msg_derive_key);
        }
        void * derive_data_buf_loop = malloc(msg_derive_key.size);
        copy_from_shared(derive_data_buf_loop, msg_derive_key.offset, msg_derive_key.size);
        uint8_t *ptr_loop = (uint8_t*)derive_data_buf_loop;
        // Leer sesión
        int session = *(int*)ptr_loop;
        ptr_loop += sizeof(int);
        // Leer clave base
        unsigned char * key_base = malloc(PSK_SIZE);
        memcpy(key_base, ptr_loop, PSK_SIZE);
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
        // Descifrar la clave base
        unsigned char * decrypted_key_base = malloc(PSK_SIZE);
        decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, key_base, PSK_SIZE, decrypted_key_base);
        print_hex("Clave base descifrada", decrypted_key_base, PSK_SIZE);
        // Derivar clave según la sesión
        key_send_t derived_key;
        derived_key = derive_key(decrypted_key_base);
        if (session == SESSION_PSK_KDK) {
          derived_key.type = KEY_TYPE_MSK;
          // LLAMAR AL 256DIGEST
          uint8_t digest[32];
          struct Sha_256 sha_256;
          sha_256_init(&sha_256, digest);
          print_hex("########################### DERIVED_KEY#################", derived_key.key, 64);
          sha_256_write(&sha_256, derived_key.key, 64);
          uint8_t* hash_result = sha_256_close(&sha_256);
          print_hex("SHA256 digest del MSK derivado (por sha_256_close)", hash_result, 32);
          installPSK(digest); // digest tiene 32 bytes, pero installPSK solo usará los primeros 16
          // Enviar el MSK derivado al host
          ocall_send_key(&derived_key, sizeof(key_send_t));
        } else if (session == SESSION_MSK) {
          derived_key.type = KEY_TYPE_EDK;
          // LLAMAR AL 256DIGEST
          // Enviar el EDK derivado al host
          ocall_send_key(&derived_key, sizeof(key_send_t));
        }
        // Limpiar memoria
        free(derive_data_buf_loop);
        free(key_base);
        free(decrypted_key_base);
      }
      break;
    case CMD_SIGN:
      ocall_print_string("Ejecutando CMD_SIGN");
      // Recibir datos del host: key_id, algoritmo y datos a firmar
      struct edge_data msg_sign;
      ocall_wait_for_message(&msg_sign);
      // Leer la estructura que contiene key_id, algoritmo y datos
      void * sign_data_buf = malloc(msg_sign.size);
      copy_from_shared(sign_data_buf, msg_sign.offset, msg_sign.size);
      uint8_t *sign_ptr = (uint8_t*)sign_data_buf;
      // Leer longitud del key_id
      size_t key_id_len = *(size_t*)sign_ptr;
      sign_ptr += sizeof(size_t);
      // Leer key_id cifrado
      uint8_t *key_id_encrypted = malloc(key_id_len);
      memcpy(key_id_encrypted, sign_ptr, key_id_len);
      sign_ptr += key_id_len;
      // Descifrar el key_id
      uint8_t *key_id = malloc(key_id_len);
      if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, key_id_encrypted, key_id_len, key_id) == -1) {
          ocall_print_string("Error descifrando key_id para firma");
          free(key_id_encrypted);
          free(key_id);
          free(sign_data_buf);
          break;
      }
      free(key_id_encrypted);
      // Leer longitud del algoritmo
      size_t algorithm_len = *(size_t*)sign_ptr;
      sign_ptr += sizeof(size_t);
      // Leer algoritmo
      char *algorithm = malloc(algorithm_len + 1);
      memcpy(algorithm, sign_ptr, algorithm_len);
      algorithm[algorithm_len] = '\0';
      sign_ptr += algorithm_len;
      // Leer longitud de los datos
      size_t data_len = *(size_t*)sign_ptr;
      sign_ptr += sizeof(size_t);
      // Leer datos a firmar
      uint8_t *data_to_sign = malloc(data_len);
      memcpy(data_to_sign, sign_ptr, data_len);
      ocall_print_string("Algoritmo recibido:");
      ocall_print_string(algorithm);
      ocall_print_string("Datos a firmar:");
      ocall_print_string((char*)data_to_sign);
      // Usar key_id descifrado como clave para firmar
      print_hex("Clave de firma descifrada", key_id, key_id_len);
      print_hex("Datos a firmar (hex)", data_to_sign, data_len);
      // Generar firma según el algoritmo especificado
      uint8_t signature[SIGNATURE_SIZE];
      memset(signature, 0, SIGNATURE_SIZE); // Inicializar con ceros
      if (strcmp(algorithm, HMAC_MD5) == 0) {
        hmac_md5(key_id, key_id_len, data_to_sign, data_len, signature);
        ocall_print_string("Firma generada (solo HMAC-MD5):");
        print_hex("HMAC-MD5", signature, MD5_DIGEST_SIZE);
      } else if (strcmp(algorithm, CMAC_AES) == 0) {
        cmac_aes(key_id, key_id_len, data_to_sign, data_len, signature);
        ocall_print_string("Firma generada (solo CMAC-AES):");
        print_hex("CMAC-AES", signature, 16);
      } else {
          free(key_id);
          free(algorithm);
          free(data_to_sign);
          free(sign_data_buf);
          break;
      }
      // Verificar que la firma se generó correctamente
      ocall_print_string("Verificando firma...");
      uint8_t verification_signature[SIGNATURE_SIZE];
      memset(verification_signature, 0, SIGNATURE_SIZE);
      if (strcmp(algorithm, HMAC_MD5) == 0) {
        hmac_md5(key_id, key_id_len, data_to_sign, data_len, verification_signature);
      } else if (strcmp(algorithm, CMAC_AES) == 0) {
        cmac_aes(key_id, key_id_len, data_to_sign, data_len, verification_signature);
      } else {
        hmac_md5(key_id, key_id_len, data_to_sign, data_len, verification_signature + MD5_DIGEST_SIZE);
        cmac_aes(key_id, key_id_len, data_to_sign, data_len, verification_signature + MD5_DIGEST_SIZE + 16);
      }
      print_hex("Signature (hex)", signature, 16);
      print_hex("Verification signature (hex)", verification_signature, 16);
      int signatures_match = 1;
      size_t compare_size = (strcmp(algorithm, HMAC_MD5) == 0) ? MD5_DIGEST_SIZE : 
                           (strcmp(algorithm, CMAC_AES) == 0) ? 16 : SIGNATURE_SIZE;
      for (size_t i = 0; i < compare_size; i++) {
          if (signature[i] != verification_signature[i]) {
              signatures_match = 0;
              break;
          }
      }
      if (signatures_match) {
          ocall_print_string("✓ Verificación exitosa: Las firmas coinciden");
      } else {
          ocall_print_string("✗ Error: Las firmas no coinciden");
      }
      // Limpiar memoria
      free(key_id);
      free(algorithm);
      free(data_to_sign);
      free(sign_data_buf);
      break;
    case CMD_ENCRYPT: {
      ocall_print_string("Ejecutando CMD_ENCRYPT");
      struct edge_data msg_encrypt;
      ocall_wait_for_message(&msg_encrypt);
 
      // Esperamos: [keyId_len][keyId][data_len][data]
      uint8_t *buf = malloc(msg_encrypt.size);
      copy_from_shared(buf, msg_encrypt.offset, msg_encrypt.size);
      uint8_t *ptr = buf;
 
      // Leer keyId_len y keyId
      size_t keyId_len = *(size_t*)ptr;
      ptr += sizeof(size_t);
      uint8_t *keyId = malloc(keyId_len);
      memcpy(keyId, ptr, keyId_len);
      ptr += keyId_len;
      print_hex("[ENCRYPT] keyId recibido", keyId, keyId_len);
      print_label_int("[ENCRYPT] keyId_len", keyId_len);
 
      // Leer data_len y data
      size_t encrypt_data_len = *(size_t*)ptr;
      ptr += sizeof(size_t);
      uint8_t *data = malloc(encrypt_data_len);
      memcpy(data, ptr, encrypt_data_len);
      print_hex("[ENCRYPT] Datos a cifrar", data, encrypt_data_len);
      print_label_int("[ENCRYPT] encrypt_data_len", encrypt_data_len);

      // Calcular el tamaño con padding PKCS#7
      size_t block_size = AES_BLOCKLEN;
      size_t padding_len = block_size - (encrypt_data_len % block_size);
      if (padding_len == 0) padding_len = block_size;
      size_t padded_len = encrypt_data_len + padding_len;
      print_label_int("[ENCRYPT] block_size", block_size);
      print_label_int("[ENCRYPT] padding_len", padding_len);
      print_label_int("[ENCRYPT] padded_len", padded_len);

      // Reservar buffer de entrada con padding
      uint8_t *padded_data = malloc(padded_len);
      memcpy(padded_data, data, encrypt_data_len);
      for (size_t i = 0; i < padding_len; ++i) {
          *(padded_data + encrypt_data_len + i) = (uint8_t)padding_len;
      }
      print_hex("[ENCRYPT] padded_data (con padding)", padded_data, padded_len);

      // Reservar buffer de salida
      uint8_t *encrypted = malloc(padded_len);
      if (!encrypted) {
          ocall_print_string("Error al reservar memoria para encrypted");
          free(keyId);
          free(data);
          free(padded_data);
          free(buf);
          break;
      }

      // Detectar tipo de AES según tamaño de clave
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
          free(data);
          free(padded_data);
          free(encrypted);
          free(buf);
          break;
      }
      print_hex("[ENCRYPT] Datos cifrados (output)", encrypted, padded_len);

      // Enviar resultado al host en formato [keyId_len][keyId][data_len][data_cifrada]
      size_t out_total_size = sizeof(size_t) + keyId_len + sizeof(size_t) + padded_len;
      uint8_t* out_buffer = malloc(out_total_size);
      if (!out_buffer) {
          ocall_print_string("Error al reservar memoria para out_buffer");
          free(keyId);
          free(data);
          free(padded_data);
          free(encrypted);
          free(buf);
          break;
      }
      uint8_t* out_ptr = out_buffer;
      memcpy(out_ptr, &keyId_len, sizeof(size_t)); out_ptr += sizeof(size_t);
      memcpy(out_ptr, keyId, keyId_len); out_ptr += keyId_len;
      memcpy(out_ptr, &padded_len, sizeof(size_t)); out_ptr += sizeof(size_t);
      memcpy(out_ptr, encrypted, padded_len);
      print_hex("[ENCRYPT] Mensaje cifrado (paquete)", out_buffer, out_total_size > 64 ? 64 : out_total_size);
      ocall_send_random_buffer(out_buffer, out_total_size);

      // Limpiar memoria
      free(keyId);
      free(data);
      free(padded_data);
      free(encrypted);
      free(out_buffer);
      free(buf);
      break;
    }
    case CMD_DECRYPT: {
      ocall_print_string("Ejecutando CMD_DECRYPT");
      struct edge_data msg_decrypt;
      ocall_wait_for_message(&msg_decrypt);

      // Esperamos el paquete completo: [keyId_len][keyId][data_len][data_cifrada]
      uint8_t *buf = malloc(msg_decrypt.size);
      copy_from_shared(buf, msg_decrypt.offset, msg_decrypt.size);
      uint8_t *ptr = buf;

      // Leer keyId_len y keyId
      size_t keyId_len = *(size_t*)ptr;
      ptr += sizeof(size_t);
      uint8_t *keyId = malloc(keyId_len);
      memcpy(keyId, ptr, keyId_len);
      ptr += keyId_len;
      print_hex("[DECRYPT] keyId recibido", keyId, keyId_len);
      print_label_int("[DECRYPT] keyId_len", keyId_len);

      // Leer data_len y data_cifrada
      size_t decrypt_data_len = *(size_t*)ptr;
      ptr += sizeof(size_t);
      uint8_t *data_cifrada = malloc(decrypt_data_len);
      memcpy(data_cifrada, ptr, decrypt_data_len);
      print_hex("[DECRYPT] Datos cifrados recibidos", data_cifrada, decrypt_data_len);
      print_label_int("[DECRYPT] decrypt_data_len", decrypt_data_len);

      // Reservar buffer de salida
      uint8_t *decrypted = malloc(decrypt_data_len);
      if (!decrypted) {
          ocall_print_string("Error al reservar memoria para decrypted");
          free(keyId);
          free(data_cifrada);
          free(buf);
          break;
      }

      // Detectar tipo de AES según tamaño de clave
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
          free(buf);
          break;
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

      // Limpiar memoria
      free(keyId);
      free(data_cifrada);
      free(decrypted);
      free(buf);
      break;
    }
    case CMD_GEN_RANDOM: {
      ocall_print_string("Ejecutando CMD_GEN_RANDOM");
      struct edge_data msg_buffer;
      ocall_wait_for_message(&msg_buffer);
      size_t random_len = msg_buffer.size;
      uint8_t* random_buf = malloc(random_len);
      get_random_buffer(random_buf, random_len);
      print_hex("Random generado", random_buf, random_len);
      ocall_send_random_buffer(random_buf, random_len);
      free(random_buf);
      break;
    }
    case CMD_RECONFIGURE:
      ocall_print_string("Ejecutando CMD_RECONFIGURE");
      break;
    default:
      ocall_print_string("Comando desconocido");
      break;
  }

  EAPP_RETURN(0);
}