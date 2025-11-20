#include "main_eapp.h"

unsigned long ocall_send_key_material(key_material_t* keys);
unsigned long ocall_send_mudUrl_string(char ocall_data[64]);
void ocall_wait_for_message(struct edge_data *msg);
unsigned long ocall_print_string(char* string);
unsigned long ocall_send_certificate_string(char ocall_data[32]);
unsigned long ocall_send_random(int random_value);
unsigned long ocall_send_random_buffer(uint8_t* buf, size_t len);
unsigned long ocall_send_key(key_send_t* key, size_t size);
unsigned long ocall_write_to_host_ptr(void* dest, void* src, size_t size);
unsigned long ocall_write_file(const char* path, const void* data, size_t dataLen);


