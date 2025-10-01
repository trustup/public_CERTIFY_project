#include <custom_se_pkcs11.h>
#include <stdio.h>

#define DECRIPTION_KEY_TAG "EDK_01"

int32_t reconfigure(uint8_t opcode, char * data, char * data_len_str);

int32_t decrypt_reconfiguration(char * data, char * data_len_str);
