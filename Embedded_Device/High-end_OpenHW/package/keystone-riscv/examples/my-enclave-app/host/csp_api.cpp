#include "ocall_handler.h"

dynamic_args_t global_dynamic_args = {0};

int actual_test_case = 0; 
// Variable global para la sesión de derivación

// Variables globales para los binarios
extern char* eapp;
extern char* runtime;
extern char* loader;

// Función auxiliar para crear, inicializar y ejecutar un enclave con una operación
void run_enclave_with_operation(int test_case) {
  actual_test_case = test_case;
  Keystone::Enclave enclave;
  Keystone::Params params;
  params.setFreeMemSize(1024 * 1024 * 48);
  params.setUntrustedSize(1024 * 1024 * 4);

  enclave.init(eapp, runtime, loader, params);
  enclave.registerOcallDispatch(incoming_call_dispatch);
  register_call(OCALL_PRINT_STRING, print_string_wrapper);
  register_call(OCALL_RECIVE_KEY_MATERIALS, print_write_key_materials);
  register_call(OCALL_RECIVE_MSK, print_write_key);
  register_call(OCALL_RECIVE_EDK, print_write_key);
  register_call(OCALL_RECIVE_MUD_URL, print_write_mud_url);
  register_call(OCALL_WAIT_FOR_MESSAGE, wait_for_message_wrapper);
  register_call(OCALL_RECIVE_CERTIFICATE, print_write_certificate);
  register_call(OCALL_SEND_RANDOM, print_write_random);
  register_call(OCALL_SEND_RANDOM_BUFFER, print_write_random_buffer);
  register_call(OCALL_WRITE_TO_HOST_PTR, write_to_host_ptr);
  register_call(OCALL_WRITE_FILE, write_file_handler);

  edge_call_init_internals((uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());
  enclave.run();
}

// Declaración de la nueva función
int32_t csp_installPSK(unsigned char* pSKValue) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  char* psk = (char*)calloc(16, 1); // 16 bytes a cero
  if (!psk) return -1;
  if (pSKValue) {
    memcpy(psk, pSKValue, 16);
  }
  global_dynamic_args.args[0].data = psk;
  global_dynamic_args.args[0].size = 16;
  global_dynamic_args.num_args = 1;
  run_enclave_with_operation(CMD_INSTALL_PSK);
  return 0;
}

// Declaración de la nueva función para instalar MudURL
int32_t csp_installMudURL(char* mUDuRLValue) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  const char* default_msg = "http://localhost:8091/MUD_Collins_Bootstrapping";
  const char* src = mUDuRLValue ? mUDuRLValue : default_msg;
  size_t msg_len = strlen(src) + 1;
  char* mud_url = (char*)calloc(msg_len, 1);
  if (!mud_url) return -1;
  memcpy(mud_url, src, msg_len);
  global_dynamic_args.args[0].data = mud_url;
  global_dynamic_args.args[0].size = msg_len;
  global_dynamic_args.num_args = 1;
  run_enclave_with_operation(CMD_INSTALL_MUD_URL);
  return 0;
}

// Declaración de la nueva función para obtener MudURL
int32_t csp_getMuDFileURL(char* mUDuRLValue) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  if (!mUDuRLValue) return -1;

  // Primer argumento: puntero de destino
  global_dynamic_args.args[0].data = mUDuRLValue;
  global_dynamic_args.args[0].size = sizeof(void*); // o sizeof(uintptr_t)

  // Segundo argumento: buffer con el contenido del fichero
  unsigned char mud_url_file_buffer[512];
  memset(mud_url_file_buffer, 0, 512);
  size_t mud_url_file_size = 0;
  FILE* f = fopen(MUD_URL_PATH, "rb");
  if (f) {
    size_t read_bytes = fread(mud_url_file_buffer, 1, 512, f); 
    fclose(f); 
    mud_url_file_size = read_bytes; // Actualiza el tamaño con los bytes leídos
    if (read_bytes < 512) { 
      memset(mud_url_file_buffer + read_bytes, 0, 512 - read_bytes); 
    }
    printf("[HOST] mud_url_file_buffer (hex): ");
    for (size_t i = 0; i < 512; ++i) {
        printf("%02x ", mud_url_file_buffer[i]);
    }
    printf("\n");
  }
  // Si no existe, queda a cero
  global_dynamic_args.args[1].data = mud_url_file_buffer;
  global_dynamic_args.args[1].size = mud_url_file_size;
  global_dynamic_args.num_args = 2;

  run_enclave_with_operation(CMD_GET_MUD_URL);
  return 0;
}

// Declaración de la nueva función para instalar certificado
int32_t csp_installCertificate(unsigned char* certificateValue, uint16_t certificateValueLen) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  uint16_t len = certificateValueLen > 0 ? certificateValueLen : 32;
  char* cert = (char*)calloc(len, 1);
  if (!cert) return -1;
  if (certificateValue && certificateValueLen > 0) {
    memcpy(cert, certificateValue, certificateValueLen);
  } else {
    char example_cert[32] = {
      (char)0xa1, (char)0xb2, (char)0xc3, (char)0xd4,
      (char)0xe5, (char)0xf6, (char)0x07, (char)0x18,
      (char)0x29, (char)0x3a, (char)0x4b, (char)0x5c,
      (char)0x6d, (char)0x7e, (char)0x8f, (char)0x90,
      (char)0x11, (char)0x22, (char)0x33, (char)0x44,
      (char)0x55, (char)0x66, (char)0x77, (char)0x88,
      (char)0x99, (char)0xaa, (char)0xbb, (char)0xcc,
      (char)0xdd, (char)0xee, (char)0xff, (char)0x00
    };
    memcpy(cert, example_cert, 32);
    len = 32;
  }
  global_dynamic_args.args[0].data = cert;
  global_dynamic_args.args[0].size = len;
  global_dynamic_args.num_args = 1;
  run_enclave_with_operation(CMD_INSTALL_CERT);
  return 0;
}

// Declaración de la nueva función para obtener el certificado
int32_t csp_getCertificateValue(unsigned char* certificateValue, uint16_t* certificateValueLen) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  if (!certificateValue || !certificateValueLen) return -1;
  memset(certificateValue, 0, 64); // buffer de 64 bytes
  FILE* f = fopen(CERTIFICATE_PATH, "rb");
  if (f) {
    size_t read_bytes = fread(certificateValue, 1, 64, f);
    fclose(f);
    *certificateValueLen = (uint16_t)read_bytes;
    if (read_bytes < 64) {
      memset(certificateValue + read_bytes, 0, 64 - read_bytes);
    }
  } else {
    *certificateValueLen = 0;
  }
  global_dynamic_args.args[0].data = certificateValue;
  global_dynamic_args.args[0].size = *certificateValueLen;
  global_dynamic_args.num_args = 1;
  run_enclave_with_operation(CMD_GET_CERT);
  return 0;
}

// Declaración de la nueva función para derivar clave
int32_t csp_deriveKey(char* baseKeyID, char* derivedKeyID, unsigned char* salt, unsigned char* info, uint16_t infoLen, uint16_t algo) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  if (!salt) return -1;
  unsigned char* salt_buf = (unsigned char*)malloc(32);
  if (!salt_buf) return -1;
  memcpy(salt_buf, salt, 32);
  global_dynamic_args.args[0].data = salt_buf;
  global_dynamic_args.args[0].size = 32;

  if (baseKeyID) {
    char* baseKeyID_buf = strdup(baseKeyID); // reserva memoria y copia el string
    global_dynamic_args.args[1].data = baseKeyID_buf;
    global_dynamic_args.args[1].size = strlen(baseKeyID);
  } else {
    global_dynamic_args.args[1].data = NULL;
    global_dynamic_args.args[1].size = 0;
  }
  global_dynamic_args.num_args = 2;
  run_enclave_with_operation(CMD_DERIVE_KEY);
  return 0;
}

// Declaración de la nueva función para firmar datos
int32_t csp_sign(char* signatureKeyID, unsigned char* dataToSign, uint16_t dataToSignLen, unsigned char* signature) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  int* use_psk_for_sign = (int*)malloc(sizeof(int));
  *use_psk_for_sign = (signatureKeyID && strcmp(signatureKeyID, "PSK") == 0) ? 1 : 0;
  unsigned char* data_buf = (unsigned char*)malloc(dataToSignLen);
  memcpy(data_buf, dataToSign, dataToSignLen);
  size_t* len_ptr = (size_t*)malloc(sizeof(size_t));
  *len_ptr = dataToSignLen;
  global_dynamic_args.args[0].data = use_psk_for_sign;
  global_dynamic_args.args[0].size = sizeof(int);
  global_dynamic_args.args[1].data = data_buf;
  global_dynamic_args.args[1].size = dataToSignLen;
  global_dynamic_args.args[2].data = len_ptr;
  global_dynamic_args.args[2].size = sizeof(size_t);
  global_dynamic_args.args[3].data = signature;
  global_dynamic_args.args[3].size = sizeof(void*);
  global_dynamic_args.num_args = 4; 
  run_enclave_with_operation(CMD_SIGN);
  return 0;
}

// Declaración de la nueva función para cifrar datos
int32_t csp_encryptData(char* encryptionKeyID, unsigned char* dataToEncrypt, uint16_t dataToEncryptLen, unsigned char* encryptedData, uint16_t algo) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  unsigned char* data_buf = (unsigned char*)malloc(dataToEncryptLen);
  memcpy(data_buf, dataToEncrypt, dataToEncryptLen);
  size_t* len_ptr = (size_t*)malloc(sizeof(size_t));
  *len_ptr = dataToEncryptLen;
  global_dynamic_args.args[0].data = data_buf;
  global_dynamic_args.args[0].size = dataToEncryptLen;
  global_dynamic_args.args[1].data = len_ptr;
  global_dynamic_args.args[1].size = sizeof(size_t);
  global_dynamic_args.args[2].data = encryptedData;
  global_dynamic_args.args[2].size = 0; // El tamaño se llenará tras el cifrado
  global_dynamic_args.num_args = 3;
  run_enclave_with_operation(CMD_ENCRYPT);
  return 0;
}

// Declaración de la nueva función para descifrar datos
int32_t csp_decryptData(char* decryptionKeyID, unsigned char* dataToDecrypt, uint16_t dataToDecryptLen, unsigned char* decryptedData, uint16_t* decryptedDataLen, uint16_t algo) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  unsigned char* data_buf = (unsigned char*)malloc(dataToDecryptLen);
  memcpy(data_buf, dataToDecrypt, dataToDecryptLen);
  size_t* len_ptr = (size_t*)malloc(sizeof(size_t));
  *len_ptr = dataToDecryptLen;
  global_dynamic_args.args[0].data = data_buf;
  global_dynamic_args.args[0].size = dataToDecryptLen;
  global_dynamic_args.args[1].data = len_ptr;
  global_dynamic_args.args[1].size = sizeof(size_t);
  global_dynamic_args.args[2].data = decryptedData;
  global_dynamic_args.args[2].size = 0;
  global_dynamic_args.args[3].data = decryptedDataLen;
  global_dynamic_args.args[3].size = 0;
  global_dynamic_args.num_args = 4;
  run_enclave_with_operation(CMD_DECRYPT);
  return 0;
}

// Declaración de la nueva función para generar aleatorio
int32_t csp_generateRandom(unsigned char * randomBuffer, uint16_t randomBufferLen) {
  memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
  unsigned char* buf = (unsigned char*)malloc(randomBufferLen);
  size_t* len_ptr = (size_t*)malloc(sizeof(size_t));
  *len_ptr = randomBufferLen;
  global_dynamic_args.args[0].data = buf;
  global_dynamic_args.args[0].size = randomBufferLen;
  global_dynamic_args.args[1].data = len_ptr;
  global_dynamic_args.args[1].size = sizeof(size_t);
  global_dynamic_args.num_args = 2;
  run_enclave_with_operation(CMD_GEN_RANDOM);
  return 0;
}

int32_t csp_reconfigure(uint8_t opcode, unsigned char * operationData, uint16_t operationDataLen) {
    memset(&global_dynamic_args, 0, sizeof(global_dynamic_args));
    // Argumento 0: opcode (como puntero a uint8_t)
    global_dynamic_args.args[0].data = &opcode;
    global_dynamic_args.args[0].size = sizeof(uint8_t);

    // Argumento 1: datos de operación (puede ser NULL)
    global_dynamic_args.args[1].data = operationData;
    global_dynamic_args.args[1].size = operationDataLen;

    global_dynamic_args.num_args = 2;
    run_enclave_with_operation(CMD_RECONFIGURE);

    return 0;
}

