#include "ocall_handler.h"

extern int actual_test_case;

unsigned long print_string(char* str) {
  return printf("[EAPP] %s \n", str);
}

unsigned long print_int(int* call_args) {
  // printf("[HOST] Número aleatorio generado: %d\n", *call_args);
  return 0;
}

void print_string_wrapper(void* buffer) {
  /* Parse and validate the incoming call data */
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  /* Pass the arguments from the eapp to the exported ocall function */
  ret_val = print_string((char*)call_args);

  /* Setup return data from the ocall function */
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(
          edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  /* This will now eventually return control to the enclave */
  return;
}

void print_write_key_materials(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }
  if (arg_len != sizeof(key_material_t)) {
    print_string("[HOST] Unexpected key_material size");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  key_material_t* keys = (key_material_t*) call_args;
  const char* tipo = (keys->type == 1) ? "MSK" : "PSK";
  printf("[HOST] AK received from %s: ", tipo);
  for (int i = 0; i < AES128_BLOCK_SIZE; i++) printf("%02x", keys->ak[i]);
  printf("\n");
  printf("[HOST] KDK received from %s: ", tipo);
  for (int i = 0; i < AES128_BLOCK_SIZE; i++) printf("%02x", keys->kdk[i]);
  printf("\n");
  // --- Crear estructura de directorios ---
  struct stat st = {0};
  if (stat(BASE_DIR, &st) == -1) {
    if (mkdir(BASE_DIR, 0700) != 0) {
      perror("[HOST] Can not create /etc/myapp");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    } else {
      // printf("[DEBUG] %s creado correctamente.\n", BASE_DIR);
    }
  } else {
    // printf("[DEBUG] %s ya existe.\n", BASE_DIR);
  }
  // printf("[DEBUG] Comprobando existencia de %s...\n", KEY_MATERIAL_DIR);
  if (stat(KEY_MATERIAL_DIR, &st) == -1) {
    // printf("[DEBUG] %s no existe. Intentando crearlo...\n", KEY_MATERIAL_DIR);
    if (mkdir(KEY_MATERIAL_DIR, 0700) != 0) {
      perror("[HOST] Can not create /etc/myapp/key_material");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    } else {
      // printf("[DEBUG] %s creado correctamente.\n", KEY_MATERIAL_DIR);
    }
  } else {
    // printf("[DEBUG] %s ya existe.\n", KEY_MATERIAL_DIR);
  }
  // Guardar en rutas específicas usando macros
  const char* ak_path = (keys->type == 1) ? AK_MSK_PATH : AK_PSK_PATH;
  const char* kdk_path = (keys->type == 1) ? KDK_MSK_PATH : KDK_PSK_PATH;
  int fd_ak = open(ak_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd_ak >= 0) { write(fd_ak, keys->ak, AES128_BLOCK_SIZE); close(fd_ak); }
  int fd_kdk = open(kdk_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd_kdk >= 0) { write(fd_kdk, keys->kdk, AES128_BLOCK_SIZE); close(fd_kdk); }
  printf("[HOST] AK from %s saved in %s\n", tipo, ak_path);
  printf("[HOST] KDK from %s saved in %s\n", tipo, kdk_path);
  unsigned long ret_val = 0;
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }
}

void print_write_key(void* buffer) {
  static int edk_counter = 0; // Contador estático para EDKs
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  size_t arg_len;

  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  if (arg_len != sizeof(key_send_t)) {
    print_string("[HOST] Tamaño inesperado de key_send_t");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  key_send_t* key = (key_send_t*) call_args;

  const char* path = NULL;
  char edk_path[128] = {0};
  int key_size = 0;
  if (key->type == KEY_TYPE_MSK) {
    printf("[HOST] MSK derivative received: ");
    path = MSK_PATH;
    key_size = 64;
  } else if (key->type == KEY_TYPE_EDK) {
    printf("[HOST] EDK derivative received: ");
    // Generar nombre EDK_01.bin ... EDK_09.bin
    edk_counter++;
    if (edk_counter > 9) edk_counter = 9; // No más de 9
    snprintf(edk_path, sizeof(edk_path), "/etc/myapp/key_material/EDK_%02d.bin", edk_counter);
    path = edk_path;
    key_size = 32;
  } else {
    print_string("[HOST] Unknown key type");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  for (int i = 0; i < key_size; i++) {
    printf("%02x", (unsigned char)key->key[i]);
  }
  printf("\n");

  // --- Crear estructura de directorios ---
  char base_dir[] = BASE_DIR;
  char key_material_dir[] = KEY_MATERIAL_DIR;
  struct stat st = {0};
  if (stat((char*)base_dir, &st) == -1) {
    if (mkdir((char*)base_dir, 0700) != 0) {
      perror("[HOST] Can not create /etc/myapp");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
  if (stat((char*)key_material_dir, &st) == -1) {
    if (mkdir((char*)key_material_dir, 0700) != 0) {
      perror("[HOST] Can not create /etc/myapp/key_material");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
  // Guardar clave
  int fd = open((char*)path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("[HOST] Error opening/creating key file");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  if (write(fd, key->key, key_size) != key_size) {
    perror("[HOST] Error entering key to disk");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  close(fd);
  printf("[HOST] Key saved in %s\n", (char*)path);

  // --- Retorno a enclave ---
  unsigned long ret_val = 0;
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }
}

void print_write_mud_url(void* buffer) {
   /* Parse and validate the incoming call data */
   struct edge_call* edge_call = (struct edge_call*)buffer;
   uintptr_t call_args;
   unsigned long ret_val;
   size_t arg_len;
   if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
     edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
     return;
   }
 
   char* cadena = (char*) call_args;

   printf("[HOST] MudURL encrypt: ");
   for (int i = 0; i < 64; i++) {
    printf("%02x", (unsigned char)cadena[i]);
   }
  printf("\n");

  // --- Crear directorio si no existe ---
  struct stat st = {0};
  if (stat(KEY_MATERIAL_DIR, &st) == -1) {
    if (mkdir(KEY_MATERIAL_DIR, 0700) != 0) {
      perror("[HOST] Can not create MudURL directory");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }

  // --- Guardar en disco ---
  int fd = open(MUD_URL_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("[HOST] Error opening/creating the MudURL file");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  if (write(fd, cadena, 64) != 64) {
    perror("[HOST] Error writing the MudURL to disk");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  close(fd);
  printf("[HOST] MudURL saved in %s\n", MUD_URL_PATH);
 
   /* Setup return data from the ocall function */
   uintptr_t data_section = edge_call_data_ptr();
   memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
   if (edge_call_setup_ret(
           edge_call, (void*)data_section, sizeof(unsigned long))) {
     edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
   } else {
     edge_call->return_data.call_status = CALL_STATUS_OK;
   }
 
   /* This will now eventually return control to the enclave */
   return;
}

void print_write_certificate(void* buffer) {
   /* Parse and validate the incoming call data */
   struct edge_call* edge_call = (struct edge_call*)buffer;
   uintptr_t call_args;
   unsigned long ret_val;
   size_t arg_len;
   if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
     edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
     return;
   }
 
   char* cadena = (char*) call_args;

   printf("[HOST] Certificate encrypt: ");
   for (int i = 0; i < 64; i++) {
    printf("%02x", (unsigned char)cadena[i]);
   }
  printf("\n");

  // --- Crear directorio si no existe ---
  struct stat st = {0};
  if (stat(KEY_MATERIAL_DIR, &st) == -1) {
    if (mkdir(KEY_MATERIAL_DIR, 0700) != 0) {
      perror("[HOST] Can not create the directory for certificate");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }

  // --- Guardar en disco ---
  int fd = open(CERTIFICATE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("[HOST] Error opening/creating the certificate file");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  if (write(fd, cadena, 64) != 64) {
    perror("[HOST] Error writing the certificate to disk");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  close(fd);
  printf("[HOST] Certificate saved in %s\n", CERTIFICATE_PATH);
 
   /* Setup return data from the ocall function */
   uintptr_t data_section = edge_call_data_ptr();
   memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
   if (edge_call_setup_ret(
           edge_call, (void*)data_section, sizeof(unsigned long))) {
     edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
   } else {
     edge_call->return_data.call_status = CALL_STATUS_OK;
   }
 
   /* This will now eventually return control to the enclave */
   return;
}

void print_write_random(void* buffer) {
    /* Parse and validate the incoming call data */
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  /* Pass the arguments from the eapp to the exported ocall function */
  ret_val = print_int((int*)call_args);

  /* Setup return data from the ocall function */
  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(
          edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  /* This will now eventually return control to the enclave */
  return;
}

void print_write_random_buffer(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*)buffer;
    uintptr_t call_args;
    unsigned long ret_val;
    size_t arg_len;
    if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
        return;
    }
    uint8_t* buf = (uint8_t*)call_args;
    printf("[HOST] Random buffer received from the enclave:\n");
    for (size_t i = 0; i < arg_len; i++) {
        printf("%02x", buf[i]);
        if ((i+1) % 16 == 0) printf("\n");
        else printf(" ");
    }
    if (arg_len % 16 != 0) printf("\n");
    // Guardar solo los datos cifrados si es CMD_ENCRYPT
    if (actual_test_case == CMD_ENCRYPT) {
        // El paquete recibido es: [keyId_len][keyId][data_len][data_cifrada]
        size_t offset = 0;
        size_t keyId_len = 0;
        memcpy(&keyId_len, buf, sizeof(size_t));
        offset += sizeof(size_t);
        offset += keyId_len; // saltar keyId
        size_t data_len = 0;
        memcpy(&data_len, buf + offset, sizeof(size_t));
        offset += sizeof(size_t);
        // Ahora offset apunta al inicio de data_cifrada
        FILE* f = fopen("/tmp/last_encrypted_input.bin", "wb");
        if (f) {
            fwrite(buf + offset, 1, data_len, f);
            fclose(f);
            printf("[HOST] Solo datos cifrados guardados en /tmp/last_encrypted_input.bin\n");
        } else {
            perror("[HOST] No se pudo guardar los datos cifrados para descifrado");
        }
    }
    // Si es CMD_DECRYPT, imprime el mensaje descifrado como string
    if (actual_test_case == CMD_DECRYPT) {
        printf("[HOST] Decrypted message received from the enclave (hex):\n");
        for (size_t i = 0; i < arg_len; i++) {
            printf("%02x", buf[i]);
            if ((i+1) % 16 == 0) printf("\n");
            else printf(" ");
        }
        if (arg_len % 16 != 0) printf("\n");
        printf("[HOST] Decrypted message received from the enclave (ASCII):\n");
        for (size_t i = 0; i < arg_len; i++) {
            unsigned char c = buf[i];
            if (c >= 32 && c <= 126) putchar((int)c);
            else putchar('.');
        }
        putchar('\n');
        fflush(stdout);
    }
    // Retorno estándar
    uintptr_t data_section = edge_call_data_ptr();
    memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
    if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    } else {
        edge_call->return_data.call_status = CALL_STATUS_OK;
    }
}

void write_to_host_ptr(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*)buffer;
    uintptr_t call_args;
    unsigned long ret_val = 0;
    size_t arg_len;
    if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
        return;
    }
    uint8_t* ptr = (uint8_t*)call_args;
    void* dest = NULL;
    size_t size = 0;
    memcpy(&dest, ptr, sizeof(void*)); ptr += sizeof(void*);
    memcpy(&size, ptr, sizeof(size_t)); ptr += sizeof(size_t);
    // printf("[HOST] write_to_host_ptr: dest=%p, size=%zu\n", dest, size);
    printf("[HOST] Received first bytes from Enclave (Secure World): ");
    for (size_t i = 0; i < (size < 16 ? size : 16); ++i) printf("%02x ", ptr[i]);
    printf("\n");
    memcpy(dest, ptr, size);
    // printf("[HOST] write_to_host_ptr: Copiados %zu bytes a %p desde buffer recibido\n", size, dest);
    uintptr_t data_section = edge_call_data_ptr();
    memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
    if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    } else {
        edge_call->return_data.call_status = CALL_STATUS_OK;
    }
}

void write_file_handler(void* buffer) {
    struct edge_call* edge_call = (struct edge_call*)buffer;
    uintptr_t call_args;
    unsigned long ret_val = 0;
    size_t arg_len;
    if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
        return;
    }
    uint8_t* ptr = (uint8_t*)call_args;
    size_t path_len = 0;
    memcpy(&path_len, ptr, sizeof(size_t)); ptr += sizeof(size_t);
    char* path = (char*)ptr;
    ptr += path_len;
    size_t data_len = 0;
    memcpy(&data_len, ptr, sizeof(size_t)); ptr += sizeof(size_t);
    void* data = ptr;
    printf("[HOST] Writing sign algorith in %s\n", path);
    printf("[HOST] OCALL_WRITE_FILE: primeros bytes de data: ");
    for (size_t i = 0; i < (data_len < 16 ? data_len : 16); ++i) printf("%02x ", ((uint8_t*)data)[i]);
    printf("\n");
    FILE* f = fopen(path, "wb");
    if (!f) {
        // perror("[HOST] OCALL_WRITE_FILE: error al abrir archivo");
        ret_val = 1;
    } else {
        if (fwrite(data, 1, data_len, f) != data_len) {
            // perror("[HOST] OCALL_WRITE_FILE: error al escribir archivo");
            ret_val = 2;
        } else {
            // printf("[HOST] OCALL_WRITE_FILE: archivo escrito correctamente\n");
            ret_val = 0;
        }
        fclose(f);
    }
    uintptr_t data_section = edge_call_data_ptr();
    memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
    if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
        edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    } else {
        edge_call->return_data.call_status = CALL_STATUS_OK;
    }
}