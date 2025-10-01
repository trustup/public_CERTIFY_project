#include "Dpabc_types.h"
#include "types_impl.h"
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

const char * const ALPHABET =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const char ALPHABET_MAP[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1,
    -1,  9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
    -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

const double iFactor = 1.36565823730976103695740418120764243208481439700722980119458355862779176747360903943915516885072037696111192757109;

// result must be declared (for the worst case): char result[len * 2];
size_t DecodeBase58(
    const unsigned char *str, int len, unsigned char *result) {
    result[0] = 0;
    size_t resultlen = 1;
    for (int i = 0; i < len; i++) {
        unsigned int carry = (unsigned int) ALPHABET_MAP[str[i]];
        if (carry == -1) return 0;
        for (int j = 0; j < resultlen; j++) {
            carry += (unsigned int) (result[j]) * 58;
            result[j] = (unsigned char) (carry & 0xff);
            carry >>= 8;
        }
        while (carry > 0) {
            result[resultlen++] = carry & 0xff;
            carry >>= 8;
        }
    }

    for (int i = 0; i < len && str[i] == '1'; i++)
        result[resultlen++] = 0;

    // Poorly coded, but guaranteed to work.
    for (int i = resultlen - 1, z = (resultlen >> 1) + (resultlen & 1);
        i >= z; i--) {
        int k = result[i];
        result[i] = result[resultlen - i - 1];
        result[resultlen - i - 1] = k;
    }
    return resultlen;
}

int main(int argc, char ** argv) { // Args: pkeyBase58, indexes, epoch, attributes

	DPABC_session session;
	int nattr = argc - 4; // Discard first three arguments
	char pk_buffer[1024];
	char * pk;
	size_t pk_sz;
	char signatureBytes[1024];
	size_t signatureBytes_sz;

	for (int i = 1; i < argc; i++) {
		printf("argument: %s\n", argv[i]);
	}

	char * pkeyBase58 = argv[1];
	char * epochAttr = argv[2];
	char * sign = argv[3];

	DPABC_initialize(&session);

	// Zp * attr[] = {zpFromInt(0), zpFromInt(1), zpFromInt(-1),
	// 			zpFromInt(0), zpFromInt(1), zpFromInt(-1)};
	
	// if (DPABC_get_key(&session, keyID, &pk, &pk_sz) != STATUS_OK) {
	// 	printf("Error getting public key\n");
	// 	return 1;
	// }
	
	pk_sz = DecodeBase58((unsigned char *)pkeyBase58, strlen(pkeyBase58), (unsigned char *)pk_buffer) - 2;

	pk = pk_buffer + 2; // We don't care about first 2 bytes
	

	base64_decode(sign, signatureBytes, &signatureBytes_sz);

	signature * composedSignature = dpabcSignFromBytes(signatureBytes);

	Zp ** attr = malloc(sizeof(Zp*) * nattr);
	for (int i = 0; i < nattr; i++) {
		attr[i] = hashToZp(argv[i+4], strlen(argv[i+4]));
	}

	Zp * epoch = hashToZp(epochAttr, strlen(epochAttr));

	publicKey * composedPk = dpabcPkFromBytes(pk);

	printf("Nattrs of public key: %d\n", composedPk->n);

	if (!verify(composedPk, composedSignature, epoch, (const Zp **)attr)) {
	    printf("Failed verification\n");
	    return 1;
	}
	else {
	    printf("Succsess\n");
	}

	DPABC_finalize(&session); 
	return 0;
}
