//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "main.h"

int actual_test_case = 0;
int derive_session = SESSION_PSK_KDK; // Variable global para la sesión de derivación
struct edge_data global_random_msg = {0, 0};

void mostrar_menu() {
  printf("\n=== Menú de comandos disponibles ===\n");
  printf("%2d - Instalar PSK\n", CMD_INSTALL_PSK);
  printf("%2d - Instalar URL MUD\n", CMD_INSTALL_MUD_URL);
  printf("%2d - Obtener URL MUD\n", CMD_GET_MUD_URL);
  printf("%2d - Instalar Certificado\n", CMD_INSTALL_CERT);
  printf("%2d - Obtener Certificado\n", CMD_GET_CERT);
  printf("%2d - Derivar Clave (Sesión: %s)\n", CMD_DERIVE_KEY, 
         (derive_session == SESSION_PSK_KDK) ? "PSK/KDK" : "MSK");
  printf("%2d - Firmar\n", CMD_SIGN);
  printf("%2d - Encriptar\n", CMD_ENCRYPT);
  printf("%2d - Desencriptar\n", CMD_DECRYPT);
  printf("%2d - Generar Aleatorio\n", CMD_GEN_RANDOM);
  printf("%2d - Reconfigurar\n", CMD_RECONFIGURE);
  printf("=====================================\n");
}

/***
 * An example call that will be exposed to the enclave application as
 * an "ocall". This is performed by an edge_wrapper function (below,
 * print_string_wrapper) and by registering that wrapper with the
 * enclave object (below, main).
 ***/
unsigned long
print_string(char* str) {
  return printf("Enclave said: \"%s\"\n", str);
}

unsigned long
print_int(int* call_args) {
  printf("[HOST] Número aleatorio generado: %d\n", *call_args);
  return 0;
}

struct edge_data wait_for_message() {
  struct edge_data message = {0, 0};
  if (actual_test_case == CMD_INSTALL_PSK) {
      char psk[PSK_SIZE] = {0};
      size_t reply_size = sizeof(psk);

      uint8_t* buffer = (uint8_t*)malloc(reply_size);
      if (!buffer) {
        perror("malloc");
        return message;
      }

      memcpy(buffer, psk, reply_size);

      printf("[HOST] El host copia el psk en el buffer: %s\n", buffer);

      message.offset = (uintptr_t)buffer;
      message.size = reply_size;
    } else if (actual_test_case == CMD_GET_MUD_URL) {
    // Leer el MUD URL cifrado del archivo
    int fd = open(MUD_URL_PATH, O_RDONLY);
    if (fd < 0) {
      perror("[HOST] No se pudo abrir el archivo MUD_URL para enviar");
      return message; // mensaje nulo
    }

    uint8_t* buffer = (uint8_t*)malloc(64);
    if (!buffer) {
      perror("[HOST] malloc falló para enviar mud_url cifrado");
      close(fd);
      return message;
    }

    ssize_t bytes_read = read(fd, buffer, 64);
    close(fd);

    if (bytes_read != 64) {
      fprintf(stderr, "[HOST] Error: esperado 64 bytes para mud_url, leído %ld\n", bytes_read);
      free(buffer);
      return message;
    }

    printf("[HOST] Enviando mudUrlCifrado desde archivo\n");

    message.offset = (uintptr_t)buffer;
    message.size = 64;
  } else if (actual_test_case == CMD_INSTALL_MUD_URL){
    // Comportamiento por defecto
    const char* fake_msg = "http://localhost:8091/MUD_Collins_Bootstrapping";
    size_t reply_size = strlen(fake_msg) + 1;

    uint8_t* buffer = (uint8_t*)malloc(reply_size);
    if (!buffer) {
      perror("malloc");
      return message;
    }

    memcpy(buffer, fake_msg, reply_size);

    printf("[HOST] El host copia el mudURL en el buffer: %s\n", buffer);

    message.offset = (uintptr_t)buffer;
    message.size = reply_size;
  } else if(actual_test_case == CMD_INSTALL_CERT){
    char certificate[32] = {
    (char)0xa1, (char)0xb2, (char)0xc3, (char)0xd4,
    (char)0xe5, (char)0xf6, (char)0x07, (char)0x18,
    (char)0x29, (char)0x3a, (char)0x4b, (char)0x5c,
    (char)0x6d, (char)0x7e, (char)0x8f, (char)0x90,
    (char)0x11, (char)0x22, (char)0x33, (char)0x44,
    (char)0x55, (char)0x66, (char)0x77, (char)0x88,
    (char)0x99, (char)0xaa, (char)0xbb, (char)0xcc,
    (char)0xdd, (char)0xee, (char)0xff, (char)0x00
};

    size_t reply_size = sizeof(certificate);

    uint8_t* buffer = (uint8_t*)malloc(reply_size);
    if (!buffer) {
      perror("malloc");
      return message;
    }

    memcpy(buffer, certificate, reply_size);

    printf("[HOST] El host copia el certificado en el buffer: %s\n", buffer);

    message.offset = (uintptr_t)buffer;
    message.size = reply_size;
  } else if (actual_test_case == CMD_GET_CERT){
    // Leer el MUD URL cifrado del archivo
    int fd = open(CERTIFICATE_PATH, O_RDONLY);
    if (fd < 0) {
      perror("[HOST] No se pudo abrir el archivo CERTIFICATE para enviar");
      return message; // mensaje nulo
    }

    uint8_t* buffer = (uint8_t*)malloc(64);
    if (!buffer) {
      perror("[HOST] malloc falló para enviar el certificado cifrado");
      close(fd);
      return message;
    }

    ssize_t bytes_read = read(fd, buffer, 32);
    close(fd);

    if (bytes_read != 32) {
      fprintf(stderr, "[HOST] Error: esperado 32 bytes para el certificado, leído %ld\n", bytes_read);
      free(buffer);
      return message;
    }

    printf("[HOST] Enviando certificado desde archivo\n");

    message.offset = (uintptr_t)buffer;
    message.size = 64;

  }else if (actual_test_case == CMD_DERIVE_KEY){
    // Preparar datos para derivación: sesión y clave base
    const char* key_path = (derive_session == SESSION_PSK_KDK) ? KDK_PATH : MSK_PATH;
    int iterations = (key_path == KDK_PATH) ? 1 : 9;
    for (int i = 0; i < iterations; ++i) {
      // Leer la clave base (PSK o MSK) del archivo correspondiente
      int fd = open(key_path, O_RDONLY);
      if (fd < 0) {
        perror("[HOST] No se pudo abrir el archivo de clave base para enviar");
        return message; // mensaje nulo
      }

      // Calcular tamaño total: [session][key_data]
      size_t total_size = sizeof(int) + PSK_SIZE;
      
      uint8_t* buffer = (uint8_t*)malloc(total_size);
      if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de derivación");
        close(fd);
        return message;
      }
      
      uint8_t* ptr = buffer;
      
      // Escribir sesión
      memcpy(ptr, &derive_session, sizeof(int));
      ptr += sizeof(int);
      
      // Leer y escribir la clave base
      ssize_t bytes_read = read(fd, ptr, PSK_SIZE);
      close(fd);

      if (bytes_read != PSK_SIZE) {
        fprintf(stderr, "[HOST] Error: esperado %d bytes para la clave base, leído %ld\n", PSK_SIZE, bytes_read);
        free(buffer);
        return message;
      }

      printf("[HOST] Enviando datos para derivación - Sesión: %d (%s), Clave: %s, Iteración: %d\n", 
             derive_session, 
             (derive_session == SESSION_PSK_KDK) ? "PSK/KDK" : "MSK",
             (derive_session == SESSION_PSK_KDK) ? "PSK (KDK)" : "MSK",
             i+1);

      message.offset = (uintptr_t)buffer;
      message.size = total_size;
      // Si solo quieres enviar una vez en el caso de KDK_PATH, puedes hacer break aquí
      // if (key_path == KDK_PATH) break;
    }
  }else if (actual_test_case == CMD_SIGN){
    // Selecciona el path según la macro
#if USE_PSK_FOR_SIGN
    const char* key_path = KDK_PATH;
    size_t key_id_len = PSK_SIZE;
#else
    const char* key_path = MSK_PATH;
    size_t key_id_len = PSK_SIZE; // Ajusta si MSK tiene otro tamaño
#endif
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
    const char* algorithm = CMAC_AES;
    const char* data_to_sign = "Hola mundo, esto es un mensaje de prueba para firmar!";
    size_t algorithm_len = strlen(algorithm);
    size_t data_len = strlen(data_to_sign);
    size_t total_size = sizeof(size_t) + key_id_len + sizeof(size_t) + algorithm_len + sizeof(size_t) + data_len;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de firma");
        return message;
    }
    uint8_t* ptr = buffer;
    memcpy(ptr, &key_id_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, key_id_buf, key_id_len);      ptr += key_id_len;
    memcpy(ptr, &algorithm_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, algorithm, algorithm_len);    ptr += algorithm_len;
    memcpy(ptr, &data_len, sizeof(size_t));   ptr += sizeof(size_t);
    memcpy(ptr, data_to_sign, data_len);
    printf("[HOST] Enviando datos para firma - Key ID: %s, Algoritmo: %s, Datos: %s\n", USE_PSK_FOR_SIGN ? "PSK" : "MSK", algorithm, data_to_sign);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_ENCRYPT){
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
    // Datos a cifrar
    const char* data_to_encrypt = "Mensaje de prueba para cifrar con EDK";
    size_t data_len = strlen(data_to_encrypt);
    // Formato: [keyId_len][keyId_cifrado][data_len][data]
    size_t total_size = sizeof(size_t) + key_id_len + sizeof(size_t) + data_len;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de cifrado");
        return message;
    }
    uint8_t* ptr = buffer;
    memcpy(ptr, &key_id_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, key_id_buf, key_id_len);      ptr += key_id_len;
    memcpy(ptr, &data_len, sizeof(size_t));   ptr += sizeof(size_t);
    memcpy(ptr, data_to_encrypt, data_len);
    printf("[HOST] Enviando datos para cifrado - Key ID: EDK, Datos: %s\n", data_to_encrypt);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_DECRYPT){
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
    // Leer los datos cifrados del archivo temporal
    const char* encrypted_path = "/tmp/last_encrypted_input.bin";
    FILE* f = fopen(encrypted_path, "rb");
    if (!f) {
      fprintf(stderr, "[HOST] Error: No se encontró el mensaje cifrado en %s. Ejecuta primero CMD_ENCRYPT.\n", encrypted_path);
      return message;
    }
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size <= 0) {
      fprintf(stderr, "[HOST] Error: El archivo cifrado está vacío.\n");
      fclose(f);
      return message;
    }
    uint8_t* encrypted_data = (uint8_t*)malloc(file_size);
    if (!encrypted_data) {
      perror("[HOST] malloc falló para leer datos cifrados");
      fclose(f);
      return message;
    }
    fread(encrypted_data, 1, file_size, f);
    fclose(f);
    // Construir el paquete igual que en CMD_ENCRYPT
    size_t total_size = sizeof(size_t) + key_id_len + sizeof(size_t) + file_size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("[HOST] malloc falló para enviar datos de descifrado");
        free(encrypted_data);
        return message;
    }
    uint8_t* ptr = buffer;
    memcpy(ptr, &key_id_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, key_id_buf, key_id_len);      ptr += key_id_len;
    size_t data_len = file_size;
    memcpy(ptr, &data_len, sizeof(size_t));   ptr += sizeof(size_t);
    memcpy(ptr, encrypted_data, file_size);
    free(encrypted_data);
    printf("[HOST] Enviando datos para descifrado - Key ID: EDK, Datos cifrados len: %zu\n", data_len);
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  } else if (actual_test_case == CMD_GEN_RANDOM) {
    size_t random_len = 32; // Por ejemplo, 32 bytes
    uint8_t* random_buf = (uint8_t*)malloc(random_len);
    if (!random_buf) {
        fprintf(stderr, "[HOST] malloc falló para random_buf\n");
        perror("malloc");
    }
    // Rellenar el struct message correctamente
    message.offset = (uintptr_t)random_buf;
    message.size = random_len;
    //global_random_msg.offset = (uintptr_t)random_buf;
    //global_random_msg.size = random_len;
    // (Opcional) Imprimir el buffer (pero estará vacío hasta que el enclave lo rellene)
  }

  return message;
}

int main(int argc, char** argv) {
  Keystone::Enclave enclave;
  Keystone::Params params;

  // Verificar argumentos de línea de comandos
  if (argc < 4) {
    fprintf(stderr, "Uso: %s <eapp> <runtime> <loader> [session]\n", argv[0]);
    fprintf(stderr, "  session: 0=PSK/KDK (por defecto), 1=MSK\n");
    return 1;
  }

  // Configurar sesión de derivación si se proporciona
  if (argc > 4) {
    int session_arg = atoi(argv[4]);
    if (session_arg == 0) {
      derive_session = SESSION_PSK_KDK;
      printf("[HOST] Sesión configurada: PSK/KDK\n");
    } else if (session_arg == 1) {
      derive_session = SESSION_MSK;
      printf("[HOST] Sesión configurada: MSK\n");
    } else {
      fprintf(stderr, "Sesión inválida. Use 0 (PSK/KDK) o 1 (MSK)\n");
      return 1;
    }
  } else {
    printf("[HOST] Usando sesión por defecto: PSK/KDK\n");
  }

  params.setFreeMemSize(1024 * 1024 * 48);
  params.setUntrustedSize(1024 * 1024 * 4);

  enclave.init(argv[1], argv[2], argv[3], params);

  enclave.registerOcallDispatch(incoming_call_dispatch);

  size_t shared_size = enclave.getSharedBufferSize();
  printf("El tamaño del compatido es : %d", shared_size);

  /* We must specifically register functions we want to export to the
     enclave. */
  register_call(OCALL_PRINT_STRING, print_string_wrapper);
  register_call(OCALL_RECIVE_KEY_MATERIALS, print_write_key_materials);
  register_call(OCALL_RECIVE_MSK, print_write_key);
  register_call(OCALL_RECIVE_EDK, print_write_key);
  register_call(OCALL_RECIVE_MUD_URL, print_write_mud_url);
  register_call(OCALL_WAIT_FOR_MESSAGE, wait_for_message_wrapper);
  register_call(OCALL_RECIVE_CERTIFICATE, print_write_certificate);
  register_call(OCALL_SEND_RANDOM, print_write_random);
  register_call(OCALL_SEND_RANDOM_BUFFER, print_write_random_buffer);

  mostrar_menu();
  int user_cmd = 0;
  printf("Introduce un número de comando (1-11): ");
  scanf("%d", &user_cmd);
  actual_test_case = user_cmd;
  if (user_cmd < 1 || user_cmd > 11) {
    fprintf(stderr, "Comando inválido.\n");
    return 1;
  }

  // Inventadon historico
  char* shared_buffer = (char*) enclave.getSharedBuffer();
  size_t buffer_size = enclave.getSharedBufferSize();
  char config[256] = {0};

  // Preparar el comando según el tipo
  switch (user_cmd) {
    case CMD_INSTALL_PSK: {
      snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    }
    case CMD_INSTALL_MUD_URL: {
      snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    }
    case CMD_GET_MUD_URL:
      snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    case CMD_INSTALL_CERT:
      snprintf(config, sizeof(config), "%d", user_cmd);
        break;
    case CMD_GET_CERT:
      snprintf(config, sizeof(config), "%d", user_cmd);
       break;
    case CMD_DERIVE_KEY:
    snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    case CMD_SIGN:
    snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    case CMD_ENCRYPT:
    snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    case CMD_DECRYPT:
    snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    case CMD_GEN_RANDOM: 
      snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    case CMD_RECONFIGURE:
      snprintf(config, sizeof(config), "%d", user_cmd);
      break;
    default:
      fprintf(stderr, "Comando no reconocido.\n");
      return 1;
  }

  if (strlen(config) + 1 > buffer_size) {
    fprintf(stderr, "Config demasiado grande para el shared buffer\n");
    return 1;
  }

  printf("[HOST] Config enviada al enclave: %s\n", config);
  strcpy(shared_buffer, config);

  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  enclave.run();

  return 0;
}


/***
 * Example edge-wrapper function. These are currently hand-written
 * wrappers, but will have autogeneration tools in the future.
 ***/
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
    print_string("[HOST] Tamaño inesperado del key_material");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  key_material_t* keys = (key_material_t*) call_args;
  const char* tipo = (keys->type == 1) ? "MSK" : "PSK";
  printf("[HOST] [%s] AK recibido: ", tipo);
  for (int i = 0; i < AES128_BLOCK_SIZE; i++) printf("%02x", keys->ak[i]);
  printf("\n");
  printf("[HOST] [%s] KDK recibido: ", tipo);
  for (int i = 0; i < AES128_BLOCK_SIZE; i++) printf("%02x", keys->kdk[i]);
  printf("\n");
  // Guardar en rutas específicas
  const char* ak_path = (keys->type == 1) ? "/etc/myapp/key_material/ak_msk.bin" : AK_PATH;
  const char* kdk_path = (keys->type == 1) ? "/etc/myapp/key_material/kdk_msk.bin" : KDK_PATH;
  int fd_ak = open(ak_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd_ak >= 0) { write(fd_ak, keys->ak, AES128_BLOCK_SIZE); close(fd_ak); }
  int fd_kdk = open(kdk_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd_kdk >= 0) { write(fd_kdk, keys->kdk, AES128_BLOCK_SIZE); close(fd_kdk); }
  printf("[HOST] [%s] AK guardado en %s\n", tipo, ak_path);
  printf("[HOST] [%s] KDK guardado en %s\n", tipo, kdk_path);
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
    printf("[HOST] MSK derivado recibido: ");
    path = MSK_PATH;
    key_size = 64;
  } else if (key->type == KEY_TYPE_EDK) {
    printf("[HOST] EDK derivado recibido: ");
    // Generar nombre EDK_01.bin ... EDK_09.bin
    edk_counter++;
    if (edk_counter > 9) edk_counter = 9; // No más de 9
    snprintf(edk_path, sizeof(edk_path), "/etc/myapp/key_material/EDK_%02d.bin", edk_counter);
    path = edk_path;
    key_size = 32;
  } else {
    print_string("[HOST] Tipo de clave desconocido");
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
      perror("[HOST] No se pudo crear /etc/myapp");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
  if (stat((char*)key_material_dir, &st) == -1) {
    if (mkdir((char*)key_material_dir, 0700) != 0) {
      perror("[HOST] No se pudo crear /etc/myapp/key_material");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
  // Guardar clave
  int fd = open((char*)path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("[HOST] Error al abrir/crear archivo de clave");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  if (write(fd, key->key, key_size) != key_size) {
    perror("[HOST] Error al escribir clave");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  close(fd);
  printf("[HOST] Clave guardada en %s\n", (char*)path);

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

   printf("[HOST] El Mud URL Cifrado es: ");
   for (int i = 0; i < 64; i++) {
    printf("%02x", (unsigned char)cadena[i]);
   }
  printf("\n");

  // --- Crear directorio si no existe ---
  struct stat st = {0};
  if (stat(KEY_MATERIAL_DIR, &st) == -1) {
    if (mkdir(KEY_MATERIAL_DIR, 0700) != 0) {
      perror("[HOST] No se pudo crear el directorio para key material");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }

  // --- Guardar en disco ---
  int fd = open(MUD_URL_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("[HOST] Error al abrir/crear el archivo para guardar el key material");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  if (write(fd, cadena, 64) != 64) {
    perror("[HOST] Error al escribir el key material en disco");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  close(fd);
  printf("[HOST] Key material guardado en %s\n", MUD_URL_PATH);
 
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

   printf("[HOST] El Certificado Cifrado es: ");
   for (int i = 0; i < 64; i++) {
    printf("%02x", (unsigned char)cadena[i]);
   }
  printf("\n");

  // --- Crear directorio si no existe ---
  struct stat st = {0};
  if (stat(KEY_MATERIAL_DIR, &st) == -1) {
    if (mkdir(KEY_MATERIAL_DIR, 0700) != 0) {
      perror("[HOST] No se pudo crear el directorio para certificate");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }

  // --- Guardar en disco ---
  int fd = open(CERTIFICATE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("[HOST] Error al abrir/crear el archivo para guardar el certificado");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  if (write(fd, cadena, 64) != 64) {
    perror("[HOST] Error al escribir el certificado en disco");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  close(fd);
  printf("[HOST] Certificado guardado en %s\n", CERTIFICATE_PATH);
 
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
    printf("[HOST] Buffer aleatorio recibido del enclave (len=%zu):\n", arg_len);
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
        printf("[HOST] Mensaje descifrado recibido del enclave (texto):\n");
        // Asegurarse de que el buffer es imprimible y seguro
        printf("%.*s\n", (int)arg_len, (const char*)buf);
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