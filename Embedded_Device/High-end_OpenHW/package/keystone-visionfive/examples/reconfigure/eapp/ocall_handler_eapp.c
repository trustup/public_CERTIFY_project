#include "ocall_handler_eapp.h"

unsigned long ocall_send_key_material(key_material_t* keys){
  unsigned long retval;
  ocall(OCALL_RECIVE_KEY_MATERIALS, keys, sizeof(key_material_t), &retval ,sizeof(unsigned long));
  return retval;
}

unsigned long ocall_send_mudUrl_string(char ocall_data[64]){
  unsigned long retval;
  ocall(OCALL_RECIVE_MUD_URL, ocall_data, 64, &retval, sizeof(unsigned long));
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
    ocall_print_string("Sending MSK to the host");
    ocall(OCALL_RECIVE_MSK, key, size, &retval ,sizeof(unsigned long));
  } else if (key->type == KEY_TYPE_EDK) {
    ocall_print_string("Sending EDK to the host");
    ocall(OCALL_RECIVE_EDK, key, size, &retval ,sizeof(unsigned long));
  } else {
    ocall_print_string("Unknown key type");
    return -1;
  }
  return retval;
}

unsigned long ocall_write_to_host_ptr(void* dest, void* src, size_t size) {
    unsigned long retval;
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: llamada");
    // print_label_int("[EAPP] ocall_write_to_host_ptr: valor de size (entrada)", size);
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: preparando buffer");
    // Construir el buffer: [dest][size][data]
    size_t total_size = sizeof(void*) + sizeof(size_t) + size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    uint8_t* ptr = buffer;
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: copiando dest");
    memcpy(ptr, &dest, sizeof(void*)); ptr += sizeof(void*);
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: copiando size");
    memcpy(ptr, &size, sizeof(size_t)); ptr += sizeof(size_t);
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: copiando data");
    memcpy(ptr, src, size);
    // print_label_int("[EAPP] ocall_write_to_host_ptr: valor de size (buffer)", size);
    print_hex("Sending first bytes to Host (Normal World)", (uint8_t*)src, size > 32 ? 32 : size);
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: buffer construido, llamando a ocall");
    ocall(OCALL_WRITE_TO_HOST_PTR, buffer, total_size, &retval, sizeof(unsigned long));
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: ocall retornó");
    free(buffer);
    // ocall_print_string("[EAPP] ocall_write_to_host_ptr: buffer liberado");
    return retval;
}

unsigned long ocall_write_file(const char* path, const void* data, size_t dataLen) {
    unsigned long retval;
    size_t path_len = strlen(path) + 1;
    size_t total_size = sizeof(size_t) + path_len + sizeof(size_t) + dataLen;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    uint8_t* ptr = buffer;
    // print_label_int("[EAPP] ocall_write_file: path_len", path_len);
    // print_label_int("[EAPP] ocall_write_file: dataLen", dataLen);
    // print_label_int("[EAPP] ocall_write_file: total_size", total_size);
    // ocall_print_string("[EAPP] ocall_write_file: path");
    // ocall_print_string((char*)path);
    memcpy(ptr, &path_len, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, path, path_len); ptr += path_len;
    memcpy(ptr, &dataLen, sizeof(size_t)); ptr += sizeof(size_t);
    memcpy(ptr, data, dataLen);
    // print_hex("[EAPP] ocall_write_file: primeros bytes de buffer", buffer, total_size > 32 ? 32 : total_size);
    // ocall_print_string("[EAPP] ocall_write_file: llamando a OCALL_WRITE_FILE");
    ocall(OCALL_WRITE_FILE, buffer, total_size, &retval, sizeof(unsigned long));
    free(buffer);
    return retval;
}
