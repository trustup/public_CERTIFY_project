#include <ERA_agent.h>

int32_t reconfigure(uint8_t opcode, char * data, char * data_len_str) {

	int32_t res;
	csp_initialize();

	for (int i = 0; i < atoi(data_len_str); i++) data[i] -= 0x01; // Data is sended +1 to avoid sending zeros as a parameter to a program

	res = csp_reconfigure(opcode, (unsigned char*)data, atoi(data_len_str));

	csp_terminate();

	return res;
}

int32_t decrypt_reconfiguration(char * data, char * data_len_str) {

	int32_t res;
	uint16_t data_len = atoi(data_len_str);
	uint16_t final_data_len = data_len;
	uint16_t decrypted_len = 0;
	char * decrypted = malloc(data_len * 2);
	char * final_data = malloc(data_len);

	csp_initialize();

	int j = 0;
	for (int i = 0; i < final_data_len; i++) {
		if (data[j] == 0xFF) {
			if (data[j+1] == 0xFF) final_data[i] = 0xFF;
			else if (data[j+1] == 0x01) final_data[i] = 0x00;
			else {
				printf("Error in ERA decoding\n");
				return 0xffff;  // Error generic
			}
			j++;
			final_data_len--;
		}

		else final_data[i] = data[j];

		j++;
	}

	res = csp_decryptData(DECRIPTION_KEY_TAG, (unsigned char*)data, data_len, (unsigned char*)decrypted, &decrypted_len, AES_256_CBC);

	for (int i = 0; i < decrypted_len; i++) decrypted[i] += 0x01; // Data is sended +1 to avoid sending zeros as a parameter to a program 255 is not an issue as it shouldn't be used

	csp_terminate();

	printf("decrypted: %.*s", decrypted_len, decrypted);

	return res;

}
