#include <bootstrapping_agent.h>

#define DECRIPTION_KEY_TAG "EDK_01"

int32_t reconfigure(uint8_t opcode, char * data, char * data_len_str);

int32_t decrypt_reconfiguration(char * data, char * data_len_str);

// Prototypes used by the reconfiguration server
int call_ta_reconfigure(unsigned char opcode, unsigned char* data, int data_len);
int call_ta_decrypt(unsigned char* data, int data_len, unsigned char** decrypted_output, int* output_len);

// Reconfiguration TCP server entrypoint
int run_reconfigure_server();
