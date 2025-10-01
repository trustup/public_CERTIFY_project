#include <Zp.h>
#include <Dpabc.h>
#include <dpabc_middleware.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

uint32_t base64_decode(char *input, char *output, size_t * output_sz) {
    int i, j, k = 0;
    unsigned char temp[4], decoded[3];

    for (i = 0; input[i] != '\0'; i += 4) {
        for (j = 0; j < 4; j++) {
            if (input[i + j] == '\0') {
                // Reached end of input string, stop decoding
                break;
            }
            temp[j] = strchr(base64_table, input[i + j]) - base64_table;
        }

        // Check if we reached end of input string before completing a group of 4 characters
        if (j < 4) {
            memset(&(temp[j]), 0, 4 - j);
        }

        decoded[0] = (temp[0] << 2) + ((temp[1] & 0x30) >> 4);
        decoded[1] = ((temp[1] & 0x0F) << 4) + ((temp[2] & 0x3C) >> 2);
        decoded[2] = ((temp[2] & 0x03) << 6) + temp[3];

        for (j = 0; j < 3; j++) {
            output[k++] = decoded[j];
        }
    }

    output[k] = '\0';
    *output_sz = k;
    return 0;
}

int main(int argc, char ** argv) { // Only arg should be signature in url-safe base64

	DPABC_session session;
	char sig[1024];
	char * sign_id = "test_signature";

	for (int i = 1; i < argc; i++) {
		printf("argument: %s\n", argv[i]);
	}

	size_t sig_sz;
	if (base64_decode(argv[1], sig, &sig_sz)) {
		printf("Error decoding signature\n");
		printf("Presented signature: %s\n", argv[1]);
		return 1;
	}

	DPABC_initialize(&session);

	if (DPABC_storeSignature(&session, sign_id, sig, dpabcSignByteSize()) != STATUS_OK) {
		printf("Store signature failed\n");
		return 1;
	}

	DPABC_finalize(&session); 
	printf("Success\n");
	return 0;
}
