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
  if (AES_init(key_buffer.key, &seal_sess, SEALING_KEY_BITS_LEN, 1,  0) == -1) {
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
  if (AES_init(key_buffer.key, &desseal_sess, SEALING_KEY_BITS_LEN, 1,  1) == -1) {
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
  int res = AES_init(pSKValue, &sess, AES128_BLOCK_SIZE * 8, 1, 0);
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

  char input_block[AES128_BLOCK_SIZE] = { 0 };
	char inter_block[AES128_BLOCK_SIZE] = { 0 };
	char inter_block_b[AES128_BLOCK_SIZE] = { 0 };
	char key[AES128_BLOCK_SIZE] = { 0 };

  size_t dst_sz;

  aes_session sess; 
  int res = AES_init(key_value, &sess, AES128_BLOCK_SIZE * 8, 1, 0);
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

  memmove(inter_block_b,inter_block,AES128_BLOCK_SIZE);

  inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x05; // Usar constante para EDK
  // Cifrar para obtener EDK derivado
  res = AES_cipher(&sess, inter_block_b, AES128_BLOCK_SIZE, key, AES_MODE_CBC);
  if (res == -1) {
    ocall_print_string("AES cipher failed");
    return derived_key;
  }

  print_hex("KEY derivado generado", (uint8_t*)key, AES128_BLOCK_SIZE);

  AES_terminate(&sess);

  unsigned char encrypted_key[AES128_BLOCK_SIZE];

  if (encrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, (uint8_t*)key, AES128_BLOCK_SIZE, encrypted_key) == -1) {
    return derived_key;
  }

  // Guardar el EDK cifrado en la estructura
  memcpy(derived_key.key, encrypted_key, AES128_BLOCK_SIZE);

  print_hex("KEY derivado cifrado", (uint8_t*)encrypted_key, AES128_BLOCK_SIZE);

  unsigned char desencrypted_key[AES128_BLOCK_SIZE];

  // Descifrar (debug)
  if (decrypt_with_sealing_key(KEY_ID_KEY_MATERIAL, encrypted_key, AES128_BLOCK_SIZE, desencrypted_key) == -1) {
    return derived_key;
  }

  print_hex("KEY derivado descifrado", (uint8_t*)desencrypted_key, AES128_BLOCK_SIZE);

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

int AES_init(const uint8_t *key, aes_session *session, uint32_t key_size_bits, int requires_iv, uint32_t operation_mode) {
	// Validar tamaño de clave
	if (key_size_bits != 128 && key_size_bits != 192 && key_size_bits != 256)
	{
    ocall_print_string("key size PSK error");
		return -1;
	}
  session->mode = operation_mode;
  session->key_size = key_size_bits;
  memset(session->iv, 0, sizeof(session->iv));
  
	// Inicializar contexto AES
	if (requires_iv)
	{
		AES_init_ctx_iv(&session->ctx, key, session->iv);
	}
	else
	{
		AES_init_ctx(&session->ctx, key);
	}

	session->key_size = key_size_bits;
	session->mode = operation_mode;
  return 0;
}

int AES_cipher(aes_session *session, const uint8_t *src, size_t src_sz, uint8_t *dst, int cipher_mode) {
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

        if (cipher_mode == AES_MODE_CBC) {
            AES_CBC_encrypt_buffer(&session->ctx, dst, padded_sz);
        } else if (cipher_mode == AES_MODE_ECB) {
            // ECB opera por bloques de 16 bytes
            for (size_t i = 0; i < padded_sz; i += AES_BLOCKLEN) {
                AES_ECB_encrypt(&session->ctx, dst + i);
            }
        } else {
            return -2; // Modo no soportado
        }
    }
    else if (session->mode == 1) // Modo descifrado
    {
        if (src_sz % AES_BLOCKLEN != 0)
        {
            return -1; // El tamaño cifrado debe ser múltiplo de bloque
        }

        memcpy(dst, src, src_sz);
        if (cipher_mode == AES_MODE_CBC) {
            AES_CBC_decrypt_buffer(&session->ctx, dst, src_sz);
        } else if (cipher_mode == AES_MODE_ECB) {
            for (size_t i = 0; i < src_sz; i += AES_BLOCKLEN) {
                AES_ECB_decrypt(&session->ctx, dst + i);
            }
        } else {
            return -2; // Modo no soportado
        }

        // Remover el padding PKCS#7 después del descifrado
        size_t pad_len = dst[src_sz - 1];
        if (pad_len > 0 && pad_len <= AES_BLOCKLEN)
            src_sz -= pad_len;
    }

    return 0;
}

int AES_terminate(aes_session *session) {
	memset(&session->ctx, 0, sizeof(session->ctx));
	memset(session->iv, 0, sizeof(session->iv));
	return 0;
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
          sha_256_init(&sha_256, hash);
          sha_256_write(&sha_256, input, len);
          (void)sha_256_close(&sha_256);
          print_hex("SHA256 digest del MSK derivado", digest, 32);
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
      
      // La estructura esperada: [key_id_length][key_id][algorithm_length][algorithm][data_length][data]
      uint8_t *sign_ptr = (uint8_t*)sign_data_buf;
      
      // Leer longitud del key_id
      size_t key_id_len = *(size_t*)sign_ptr;
      sign_ptr += sizeof(size_t);
      
      // Leer key_id
      char *key_id = malloc(key_id_len + 1);
      memcpy(key_id, sign_ptr, key_id_len);
      key_id[key_id_len] = '\0';
      sign_ptr += key_id_len;
      
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
      
      ocall_print_string("Key ID recibido:");
      ocall_print_string(key_id);
      
      ocall_print_string("Algoritmo recibido:");
      ocall_print_string(algorithm);
      
      ocall_print_string("Datos a firmar:");
      ocall_print_string((char*)data_to_sign);
      
      // Obtener la clave de sealing basada en el key_id
      struct sealing_key key_buffer;
      if (get_sealing_key(&key_buffer, sizeof(key_buffer), (void *)key_id, strlen(key_id)) != 0) {
        ocall_print_string("Error obteniendo sealing key para firma");
        free(key_id);
        free(algorithm);
        free(data_to_sign);
        free(sign_data_buf);
        break;
      }
      
      print_hex("Clave de firma obtenida", key_buffer.key, 16);
      
      // Generar firma según el algoritmo especificado
      uint8_t signature[SIGNATURE_SIZE];
      memset(signature, 0, SIGNATURE_SIZE); // Inicializar con ceros
      
      if (strcmp(algorithm, HMAC_MD5) == 0) {
        // Solo HMAC-MD5
        hmac_md5(key_buffer.key, 16, data_to_sign, data_len, signature);
        ocall_print_string("Firma generada (solo HMAC-MD5):");
        print_hex("HMAC-MD5", signature, MD5_DIGEST_SIZE);
      } else if (strcmp(algorithm, CMAC_AES) == 0) {
        // Solo CMAC-AES
        cmac_aes(key_buffer.key, 16, data_to_sign, data_len, signature);
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
      
      // Generar firma de verificación con los mismos datos y algoritmo
      uint8_t verification_signature[SIGNATURE_SIZE];
      memset(verification_signature, 0, SIGNATURE_SIZE);
      
      if (strcmp(algorithm, HMAC_MD5) == 0) {
        hmac_md5(key_buffer.key, 16, data_to_sign, data_len, verification_signature);
      } else if (strcmp(algorithm, CMAC_AES) == 0) {
        cmac_aes(key_buffer.key, 16, data_to_sign, data_len, verification_signature);
      } else {
        hmac_md5(key_buffer.key, 16, data_to_sign, data_len, verification_signature);
        cmac_aes(key_buffer.key, 16, data_to_sign, data_len, verification_signature + MD5_DIGEST_SIZE);
      }
      
      // Comparar firmas
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
    case CMD_ENCRYPT:
      ocall_print_string("Ejecutando CMD_ENCRYPT");
      break;
    case CMD_DECRYPT:
      ocall_print_string("Ejecutando CMD_DECRYPT");
      break;
    case CMD_GEN_RANDOM:
      ocall_print_string("Ejecutando CMD_GEN_RANDOM");
      int random = get_random();
      ocall_send_random(random);
      break;
    case CMD_RECONFIGURE:
      ocall_print_string("Ejecutando CMD_RECONFIGURE");
      break;
    default:
      ocall_print_string("Comando desconocido");
      break;
  }

  EAPP_RETURN(0);
}