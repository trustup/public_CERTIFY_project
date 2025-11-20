#include "ocall_handler.h"

extern int actual_test_case;
extern dynamic_args_t global_dynamic_args;

struct edge_data wait_for_message() {
  struct edge_data message = {0, 0};
  if (actual_test_case == CMD_INSTALL_PSK) {
      void* psk = global_dynamic_args.args[0].data;
      size_t reply_size = global_dynamic_args.args[0].size;
      size_t total_size = sizeof(int) + reply_size;
      uint8_t* buffer = (uint8_t*)malloc(total_size);
      if (!buffer) {
        perror("malloc");
        return message;
      }
      uint8_t* ptr = buffer;
      int cmd = CMD_INSTALL_PSK;
      memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
      memcpy(ptr, psk, reply_size);
      printf("[HOST] El host copia el psk en el buffer: %s\n", (char*)(buffer + sizeof(int)));
      message.offset = (uintptr_t)buffer;
      message.size = total_size;
    } else if (actual_test_case == CMD_GET_MUD_URL) {
    void* dest_ptr = global_dynamic_args.args[0].data;
    unsigned char* mudurl_cifrado_ptr = (unsigned char*)global_dynamic_args.args[1].data;
    size_t mudurl_cifrado_size = global_dynamic_args.args[1].size;
    printf("[HOST] dest_ptr: %p\n", dest_ptr);
    printf("[HOST] mudurl_cifrado_ptr: %p\n", mudurl_cifrado_ptr);
    printf("[HOST] mudurl_cifrado_size: %zu\n", mudurl_cifrado_size);

    printf("[HOST] mudurl_cifrado_ptr (hex): ");
    for (size_t i = 0; i < (mudurl_cifrado_size > 64 ? 64 : mudurl_cifrado_size); ++i) {
        printf("%02x ", mudurl_cifrado_ptr[i]);
    }
    printf("\n");

    size_t total_size = sizeof(int) + sizeof(void*) + sizeof(size_t) + mudurl_cifrado_size;
    // printf("[DEBUG] total_size %zu\n", total_size);
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("malloc");
      return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_GET_MUD_URL;
    memcpy(ptr, &cmd, sizeof(int)); 
    ptr += sizeof(int);
    memcpy(ptr, &dest_ptr, sizeof(void*)); 
    ptr += sizeof(void*);
    memcpy(ptr, &mudurl_cifrado_size, sizeof(size_t)); 
    ptr += sizeof(size_t);
    memcpy(ptr, mudurl_cifrado_ptr, mudurl_cifrado_size);
    printf("[HOST] Enviando comando, puntero destino, tamaño y buffer mudUrl cifrado\n");
    // printf("[DEBUG] buffer: ");
    for (size_t i = 0; i < (total_size > 64 ? 64 : total_size); ++i) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_INSTALL_MUD_URL){
    void* mud_url = global_dynamic_args.args[0].data;
    size_t reply_size = global_dynamic_args.args[0].size;
    size_t total_size = sizeof(int) + reply_size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("malloc");
      return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_INSTALL_MUD_URL;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, mud_url, reply_size);
    printf("[HOST] El host copia el mudURL en el buffer: %s\n", (char*)(buffer + sizeof(int)));
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if(actual_test_case == CMD_INSTALL_CERT){
    void* certificate = global_dynamic_args.args[0].data;
    size_t reply_size = global_dynamic_args.args[0].size;
    size_t total_size = sizeof(int) + reply_size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("malloc");
      return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_INSTALL_CERT;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, certificate, reply_size);
    printf("[HOST] El host copia el certificado en el buffer: %s\n", (char*)(buffer + sizeof(int)));
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_GET_CERT) {
    void* cert = global_dynamic_args.args[0].data;
    size_t reply_size = global_dynamic_args.args[0].size;
    size_t total_size = sizeof(int) + reply_size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("malloc");
      return message;
    }
    int cmd = CMD_GET_CERT;
    memcpy(buffer, &cmd, sizeof(int));
    memcpy(buffer + sizeof(int), cert, reply_size);
    printf("[HOST] Enviando certificado desde argumento dinámico\n");
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_DERIVE_KEY){
    void* random_buf = global_dynamic_args.args[0].data;
    size_t random_size = global_dynamic_args.args[0].size;
    char* baseKeyID = (char*)global_dynamic_args.args[1].data;
    const char* key_path = NULL;
    if (baseKeyID && strcmp(baseKeyID, "PSK") == 0) {
      key_path = KDK_PSK_PATH;
    } else if (baseKeyID && strcmp(baseKeyID, "MSK") == 0) {
      key_path = KDK_MSK_PATH;
    } else {
      fprintf(stderr, "[HOST] baseKeyID desconocido o nulo\n");
      return message;
    }
    int fd = open(key_path, O_RDONLY);
    if (fd < 0) {
      perror("[HOST] No se pudo abrir el archivo de clave base para enviar");
      return message;
    }
    // Formato: [cmd (int)][session (int)][key_data][random_buf]
    size_t total_size = sizeof(int) + sizeof(int) + PSK_SIZE + random_size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("[HOST] malloc falló para enviar datos de derivación");
      close(fd);
      return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_DERIVE_KEY;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    int session = 0;
    if (baseKeyID && strcmp(baseKeyID, "PSK") == 0) {
      session = 0;
    } else if (baseKeyID && strcmp(baseKeyID, "MSK") == 0) {
      session = 1;
    }
    memcpy(ptr, &session, sizeof(int)); ptr += sizeof(int);
    ssize_t bytes_read = read(fd, ptr, PSK_SIZE); ptr += PSK_SIZE;
    close(fd);
    if (bytes_read != PSK_SIZE) {
      fprintf(stderr, "[HOST] Error: esperado %d bytes para la clave base, leído %ld\n", PSK_SIZE, bytes_read);
      free(buffer);
      return message;
    }
    memcpy(ptr, random_buf, random_size); // Añadir el random al final
    printf("[HOST] Enviando datos para derivación - CMD: %d, baseKeyID: %s, Random añadido\n", cmd, baseKeyID ? baseKeyID : "NULL");
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  }else if (actual_test_case == CMD_SIGN){
    // Recibe los parámetros desde global_dynamic_args
    int use_psk_for_sign = *(int*)global_dynamic_args.args[0].data;
    unsigned char* data_to_sign = (unsigned char*)global_dynamic_args.args[1].data;
    size_t data_len = *(size_t*)global_dynamic_args.args[2].data;
    void* signature_ptr = global_dynamic_args.args[3].data;
    // Selecciona el path según el flag
    const char* key_path = use_psk_for_sign ? AK_PSK_PATH : AK_MSK_PATH;
    printf("+++++++++++++++++++%s\n", key_path);
    size_t key_id_len = PSK_SIZE;
    // Lee la clave del archivo
    int fd = open(key_path, O_RDONLY);
    if (fd < 0) {
        perror("[HOST] No se pudo abrir el archivo de clave para firmar");
        return message;
    }
    uint8_t key_id_buf[PSK_SIZE];
    ssize_t bytes_read = read(fd, key_id_buf, key_id_len);
    close(fd);
    if (bytes_read != key_id_len) {
        fprintf(stderr, "[HOST] Error: esperado %zu bytes para la clave, leído %ld\n", key_id_len, bytes_read);
        return message;
    }
    // --- Leer algoritmo de fichero ---
    char algorithm_buf[64] = {0};
    size_t algorithm_len = 0;
    FILE* f_algo = fopen(SIGN_ALGO_PATH, "rb");
    if (f_algo) {
        algorithm_len = fread(algorithm_buf, 1, sizeof(algorithm_buf)-1, f_algo);
        fclose(f_algo);
        if (algorithm_len == 0) {
            strcpy(algorithm_buf, "DEFAULT");
            algorithm_len = strlen(algorithm_buf);
        }
    } else {
        strcpy(algorithm_buf, "DEFAULT");
        algorithm_len = strlen(algorithm_buf);
    }
    // Formato: [cmd (int)][keyId_len][keyId][algorithm_len][algorithm][data_len][data][signature_ptr]
    size_t total_size = sizeof(int) + sizeof(size_t) + key_id_len + sizeof(size_t) + algorithm_len + sizeof(size_t) + data_len + sizeof(void*);
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de firma");
        return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_SIGN;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    printf("[HOST] CMD_SIGN buffer: cmd=%d\n", cmd);
    memcpy(ptr, &key_id_len, sizeof(size_t)); ptr += sizeof(size_t);
    printf("[HOST] CMD_SIGN buffer: key_id_len=%zu\n", key_id_len);
    printf("[HOST] CMD_SIGN buffer: key_id_buf (hex): ");
    for (size_t i = 0; i < key_id_len; ++i) printf("%02x ", key_id_buf[i]);
    printf("\n");
    memcpy(ptr, key_id_buf, key_id_len);      ptr += key_id_len;
    memcpy(ptr, &algorithm_len, sizeof(size_t)); ptr += sizeof(size_t);
    printf("[HOST] CMD_SIGN buffer: algorithm_len=%zu\n", algorithm_len);
    printf("[HOST] CMD_SIGN buffer: algorithm='%.*s'\n", (int)algorithm_len, algorithm_buf);
    memcpy(ptr, algorithm_buf, algorithm_len);    ptr += algorithm_len;
    memcpy(ptr, &data_len, sizeof(size_t));   ptr += sizeof(size_t);
    printf("[HOST] CMD_SIGN buffer: data_len=%zu\n", data_len);
    printf("[HOST] CMD_SIGN buffer: data_to_sign (hex): ");
    for (size_t i = 0; i < data_len; ++i) printf("%02x ", ((unsigned char*)data_to_sign)[i]);
    printf("\n");
    memcpy(ptr, data_to_sign, data_len);      ptr += data_len;
    printf("[HOST] CMD_SIGN buffer: signature_ptr=%p\n", signature_ptr);
    memcpy(ptr, &signature_ptr, sizeof(void*)); ptr += sizeof(void*);
    // Imprime el buffer completo en hex
    printf("[HOST] CMD_SIGN buffer completo (hex): ");
    for (size_t i = 0; i < total_size; ++i) printf("%02x ", buffer[i]);
    printf("\n");
    printf("[HOST] Enviando datos para firma - CMD: %d, Key ID: %s, Algoritmo: %.*s, Datos: [%zu bytes], signature_ptr: %p\n", cmd, use_psk_for_sign ? "PSK" : "MSK", (int)algorithm_len, algorithm_buf, data_len, signature_ptr);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_ENCRYPT){
    unsigned char* data_to_encrypt = (unsigned char*)global_dynamic_args.args[0].data;
    size_t data_len = *(size_t*)global_dynamic_args.args[1].data;
    void* encrypted_data_ptr = global_dynamic_args.args[2].data;
    // Leer la clave EDK del archivo
    const char* key_path = "/etc/myapp/key_material/EDK_01.bin"; // O el EDK que corresponda
    size_t key_id_len = 32;
    int fd = open(key_path, O_RDONLY);
    if (fd < 0) {
        perror("[HOST] No se pudo abrir el archivo de clave EDK para cifrar");
        return message;
    }
    uint8_t key_id_buf[32];
    ssize_t bytes_read = read(fd, key_id_buf, key_id_len);
    close(fd);
    if (bytes_read != key_id_len) {
        fprintf(stderr, "[HOST] Error: esperado %zu bytes para la clave EDK, leído %ld\n", key_id_len, bytes_read);
        return message;
    }
    // Formato: [cmd (int)][keyId_len][keyId][data_len][data][encryptedData_ptr]
    size_t total_size = sizeof(int) + sizeof(size_t) + key_id_len + sizeof(size_t) + data_len + sizeof(uintptr_t);
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de cifrado");
        return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_ENCRYPT;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, &key_id_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, key_id_buf, key_id_len);      ptr += key_id_len;
    memcpy(ptr, &data_len, sizeof(size_t));   ptr += sizeof(size_t);
    memcpy(ptr, data_to_encrypt, data_len);   ptr += data_len;
    uintptr_t encrypted_ptr_val = (uintptr_t)encrypted_data_ptr;
    memcpy(ptr, &encrypted_ptr_val, sizeof(uintptr_t)); ptr += sizeof(uintptr_t);
    printf("[HOST] Enviando datos para cifrado - CMD: %d, Key ID: EDK, Datos: %zu bytes, encryptedData_ptr: %p\n", cmd, data_len, encrypted_data_ptr);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_DECRYPT){
    unsigned char* encrypted_data = (unsigned char*)global_dynamic_args.args[0].data;
    size_t encrypted_data_len = *(size_t*)global_dynamic_args.args[1].data;
    void* decrypted_data_ptr = global_dynamic_args.args[2].data;
    void* decrypted_data_len_ptr = global_dynamic_args.args[3].data;
    // Leer la clave EDK del archivo (igual que en CMD_ENCRYPT)
    const char* key_path = "/etc/myapp/key_material/EDK_01.bin"; // O el EDK que corresponda
    size_t key_id_len = 32;
    int fd = open(key_path, O_RDONLY);
    if (fd < 0) {
        perror("[HOST] No se pudo abrir el archivo de clave EDK para descifrar");
        return message;
    }
    uint8_t key_id_buf[32];
    ssize_t bytes_read = read(fd, key_id_buf, key_id_len);
    close(fd);
    if (bytes_read != key_id_len) {
        fprintf(stderr, "[HOST] Error: esperado %zu bytes para la clave EDK, leído %ld\n", key_id_len, bytes_read);
        return message;
    }
    // Formato: [cmd (int)][keyId_len][keyId][data_len][data][decryptedData_ptr][decryptedDataLen_ptr]
    size_t total_size = sizeof(int) + sizeof(size_t) + key_id_len + sizeof(size_t) + encrypted_data_len + sizeof(uintptr_t) + sizeof(uintptr_t);
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de descifrado");
        return message;
    }
    uint8_t* ptr = buffer;
    int cmd = CMD_DECRYPT;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, &key_id_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, key_id_buf, key_id_len);      ptr += key_id_len;
    memcpy(ptr, &encrypted_data_len, sizeof(size_t));   ptr += sizeof(size_t);
    memcpy(ptr, encrypted_data, encrypted_data_len);    ptr += encrypted_data_len;
    uintptr_t decrypted_ptr_val = (uintptr_t)decrypted_data_ptr;
    memcpy(ptr, &decrypted_ptr_val, sizeof(uintptr_t)); ptr += sizeof(uintptr_t);
    uintptr_t decrypted_len_ptr_val = (uintptr_t)decrypted_data_len_ptr;
    memcpy(ptr, &decrypted_len_ptr_val, sizeof(uintptr_t)); ptr += sizeof(uintptr_t);
    printf("[HOST] Enviando datos para descifrado - CMD: %d, Key ID: EDK, Datos cifrados len: %zu, decryptedData_ptr: %p, decryptedDataLen_ptr: %p\n", cmd, encrypted_data_len, decrypted_data_ptr, decrypted_data_len_ptr);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_GEN_RANDOM) {
    unsigned char* random_buf = (unsigned char*)global_dynamic_args.args[0].data;
    size_t random_len = *(size_t*)global_dynamic_args.args[1].data;
    // Formato: [cmd (int)][random_len (size_t)][random_buf_ptr]
    size_t total_size = sizeof(int) + sizeof(size_t) + sizeof(uintptr_t);
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        fprintf(stderr, "[HOST] malloc falló para random_buf\n");
        perror("malloc");
    }
    int cmd = CMD_GEN_RANDOM;
    uint8_t* ptr = buffer;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, &random_len, sizeof(size_t)); ptr += sizeof(size_t);
    uintptr_t random_buf_ptr = (uintptr_t)random_buf;
    memcpy(ptr, &random_buf_ptr, sizeof(uintptr_t)); ptr += sizeof(uintptr_t);
    printf("[HOST] Enviando petición de aleatorio - CMD: %d, random_len: %zu, random_buf_ptr: %p\n", cmd, random_len, random_buf);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_RECONFIGURE) {
    uint8_t* opcode_ptr = (uint8_t*)global_dynamic_args.args[0].data;
    size_t opcode_size = global_dynamic_args.args[0].size;
    unsigned char* operation_data = (unsigned char*)global_dynamic_args.args[1].data;
    size_t operation_data_len = global_dynamic_args.args[1].size;
    if (!opcode_ptr || opcode_size != 1) {
      fprintf(stderr, "[HOST] CMD_RECONFIGURE: opcode inválido\n");
      return message;
    }
    int cmd = CMD_RECONFIGURE;
    size_t total_size = sizeof(int) + sizeof(uint8_t) + operation_data_len;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("[HOST] malloc falló para CMD_RECONFIGURE");
      return message;
    }
    uint8_t* ptr = buffer;
    memcpy(ptr, &cmd, sizeof(int)); ptr += sizeof(int);
    memcpy(ptr, opcode_ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
    if (operation_data && operation_data_len > 0) {
      memcpy(ptr, operation_data, operation_data_len);
    }
    printf("[HOST] Enviando CMD_RECONFIGURE: opcode=0x%02x, data_len=%zu\n", *opcode_ptr, operation_data_len);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  }

  return message;
}

void wait_for_message_wrapper(void* buffer)
{

  /* For now we assume the call struct is at the front of the shared
   * buffer. This will have to change to allow nested calls. */
  struct edge_call* edge_call = (struct edge_call*)buffer;

  uintptr_t call_args;
  unsigned long ret_val;
  size_t args_len;
  if(edge_call_args_ptr(edge_call, &call_args, &args_len) != 0){
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  struct edge_data host_msg = wait_for_message();

  // This handles wrapping the data into an edge_data_t and storing it
  // in the shared region.
  if( edge_call_setup_wrapped_ret(edge_call, (void*)host_msg.offset, host_msg.size)){
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  }
  else{
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  return;
}