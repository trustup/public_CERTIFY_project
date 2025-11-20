#ifndef OCALL_HANDLER_H
#define OCALL_HANDLER_H
#define MAX_ARGS 8

#include "csp_api.h"

extern int actual_test_case;
extern dynamic_args_t global_dynamic_args; 

unsigned long print_string(char* str);
unsigned long print_int(int* call_args);
void print_string_wrapper(void* buffer);
void print_write_key_materials(void* buffer);
void print_write_key(void* buffer);
void print_write_mud_url(void* buffer);
void print_write_certificate(void* buffer);
void print_write_random(void* buffer);
void print_write_random_buffer(void* buffer);
void write_file_handler(void* buffer);

struct edge_data wait_for_message(void);
void wait_for_message_wrapper(void* buffer);
void write_to_host_ptr(void* buffer);

#endif