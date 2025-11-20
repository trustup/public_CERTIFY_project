//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "main.h"

int actual_test_case = 0;

void mostrar_menu() {
  printf("\n=== Menú de comandos disponibles ===\n");
  printf("%2d - Instalar PSK\n", CMD_INSTALL_PSK);
  printf("%2d - Instalar URL MUD\n", CMD_INSTALL_MUD_URL);
  printf("%2d - Obtener URL MUD\n", CMD_GET_MUD_URL);
  printf("%2d - Instalar Certificado\n", CMD_INSTALL_CERT);
  printf("%2d - Obtener Certificado\n", CMD_GET_CERT);
  printf("%2d - Derivar Clave\n", CMD_DERIVE_KEY);
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
      char psk[16] = {0};
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
  }else if (actual_test_case == CMD_SIGN){
    // Preparar datos para firma: key_id y datos a firmar
    const char* key_id = "identifier1"; // Puede ser "PSK" o "MSK" o cualquier key_id válido
    const char* data_to_sign = "Hola mundo, esto es un mensaje de prueba para firmar!";
    
    size_t key_id_len = strlen(key_id);
    size_t data_len = strlen(data_to_sign);
    
    // Calcular tamaño total: [key_id_length][key_id][data_length][data]
    size_t total_size = sizeof(size_t) + key_id_len + sizeof(size_t) + data_len;
    
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
      perror("[HOST] malloc falló para enviar datos de firma");
      return message;
    }
    
    uint8_t* ptr = buffer;
    
    // Escribir longitud del key_id
    memcpy(ptr, &key_id_len, sizeof(size_t));
    ptr += sizeof(size_t);
    
    // Escribir key_id
    memcpy(ptr, key_id, key_id_len);
    ptr += key_id_len;
    
    // Escribir longitud de los datos
    memcpy(ptr, &data_len, sizeof(size_t));
    ptr += sizeof(size_t);
    
    // Escribir datos a firmar
    memcpy(ptr, data_to_sign, data_len);
    
    printf("[HOST] Enviando datos para firma - Key ID: %s, Datos: %s\n", key_id, data_to_sign);
    
    message.offset = (uintptr_t)buffer;
    message.size = total_size;
  }

  return message;
}



int
main(int argc, char** argv) {
  Keystone::Enclave enclave;
  Keystone::Params params;

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
  register_call(OCALL_RECIVE_MUD_URL, print_write_mud_url);
  register_call(OCALL_WAIT_FOR_MESSAGE, wait_for_message_wrapper);
  register_call(OCALL_READ_ENCRYPTED_MUD_URL, read_encrypted_mud_url_wrapper);
  register_call(OCALL_RECIVE_CERTIFICATE, print_write_certificate);
  register_call(OCALL_SEND_CERTIFICATE, wait_for_message_wrapper);
  register_call(OCALL_READ_ENCRYPTED_CERTIFICATE, read_encrypted_certificate_wrapper);
  register_call(OCALL_SEND_RANDOM, print_write_random);

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
void
print_string_wrapper(void* buffer) {
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
 
  printf("[HOST] AK recibido: ");
  for (int i = 0; i < AES128_BLOCK_SIZE; i++) {
    printf("%02x", keys->ak[i]);
  }
  printf("\n");
 
  printf("[HOST] KDK recibido: ");
  for (int i = 0; i < AES128_BLOCK_SIZE; i++) {
    printf("%02x", keys->kdk[i]);
  }
  printf("\n");
 
  // --- Crear estructura de directorios ---
  const char* base_dir = "/etc/myapp";
  const char* key_material_dir = "/etc/myapp/key_material";
  const char* ak_dir = "/etc/myapp/key_material/ak";
  const char* kdk_dir = "/etc/myapp/key_material/kdk";
 
  struct stat st = {0};
 
  if (stat(base_dir, &st) == -1) {
    if (mkdir(base_dir, 0700) != 0) {
      perror("[HOST] No se pudo crear /etc/myapp");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }

   if (stat(key_material_dir, &st) == -1) {
    if (mkdir(key_material_dir, 0700) != 0) {
      perror("[HOST] No se pudo crear /etc/myapp/key_material");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
 
  if (stat(ak_dir, &st) == -1) {
    if (mkdir(ak_dir, 0700) != 0) {
      perror("[HOST] No se pudo crear subdirectorio ak");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
 
  if (stat(kdk_dir, &st) == -1) {
    if (mkdir(kdk_dir, 0700) != 0) {
      perror("[HOST] No se pudo crear subdirectorio kdk");
      edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
      return;
    }
  }
 
  // --- Guardar claves ---
  const char* ak_path = "/etc/myapp/key_material/ak/ak.bin";
  const char* kdk_path = "/etc/myapp/key_material/kdk/kdk.bin";
 
  // Guardar AK
  int fd_ak = open(ak_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd_ak < 0) {
    perror("[HOST] Error al abrir/crear archivo ak");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  if (write(fd_ak, keys->ak, AES128_BLOCK_SIZE) != AES128_BLOCK_SIZE) {
    perror("[HOST] Error al escribir AK");
    close(fd_ak);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  close(fd_ak);
 
  // Guardar KDK
  int fd_kdk = open(kdk_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd_kdk < 0) {
    perror("[HOST] Error al abrir/crear archivo kdk");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  if (write(fd_kdk, keys->kdk, AES128_BLOCK_SIZE) != AES128_BLOCK_SIZE) {
    perror("[HOST] Error al escribir KDK");
    close(fd_kdk);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }
  close(fd_kdk);
 
  printf("[HOST] AK guardado en %s\n", ak_path);
  printf("[HOST] KDK guardado en %s\n", kdk_path);
 
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


void read_encrypted_mud_url_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;

  // Verificamos que el archivo existe
  int fd = open(MUD_URL_PATH, O_RDONLY);
  if (fd < 0) {
    perror("[HOST] Error al abrir el archivo del MUD URL cifrado");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  // Leemos exactamente 64 bytes
  char* mud_data = (char*) malloc(64);
  if (!mud_data) {
    perror("[HOST] Error en malloc para MUD URL cifrado");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  ssize_t read_bytes = read(fd, mud_data, 64);
  close(fd);

  if (read_bytes != 64) {
    fprintf(stderr, "[HOST] Error: se esperaban 64 bytes, se leyeron %ld\n", read_bytes);
    free(mud_data);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  printf("[HOST] MUD URL cifrado leído del archivo:\n");
  for (int i = 0; i < 64; i++) {
    printf("%02x", (unsigned char)mud_data[i]);
  }
  printf("\n");

  // Retorno al enclave
  if (edge_call_setup_wrapped_ret(edge_call, mud_data, 64)) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  free(mud_data);
  return;
}

void read_encrypted_certificate_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;

  // Verificamos que el archivo existe
  int fd = open(CERTIFICATE_PATH, O_RDONLY);
  if (fd < 0) {
    perror("[HOST] Error al abrir el archivo del certificado cifrado");
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  // Leemos exactamente 64 bytes
  char* cert_data = (char*) malloc(64);
  if (!cert_data) {
    perror("[HOST] Error en malloc para certificado cifrado");
    close(fd);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  ssize_t read_bytes = read(fd, cert_data, 64);
  close(fd);

  if (read_bytes != 64) {
    fprintf(stderr, "[HOST] Error: se esperaban 64 bytes, se leyeron %ld\n", read_bytes);
    free(cert_data);
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
    return;
  }

  printf("[HOST] Certificado cifrado leído del archivo:\n");
  for (int i = 0; i < 64; i++) {
    printf("%02x", (unsigned char)cert_data[i]);
  }
  printf("\n");

  // Retorno al enclave
  if (edge_call_setup_wrapped_ret(edge_call, cert_data, 64)) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }

  free(cert_data);
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
