#include "Dpabc_types.h"
#include <Zp.h>
#include <Dpabc.h>
#include <dpabc_middleware.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint32_t base64_decode(char *input, char *output, size_t * output_sz) {
    int i, j, k = 0;
    unsigned char temp[4], decoded[3];

    for (i = 0; input[i] != '\0' && input[i] != '='; i += 4) {
        for (j = 0; j < 4; j++) {
            if (input[i + j] == '\0' || input[i + j] == '=') {
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
	    if (!decoded[j]) {
		break;
	    }
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

uint32_t format(uint16_t nattrs, int * revealed, int revealed_sz, char * zktokenBytes, size_t zktokenBytes_sz, char * res, size_t * res_sz) {

    char * offset;
    char * bit_vec;
    res[0] = ( nattrs >> 8 ) & 0xFF;
    res[1] = ( nattrs >> 0 ) & 0xFF;
    offset = res + 2 + (nattrs / 8) + 1;
    bit_vec = res + 2;

    bit_vec[0] |= 1;
    int revIndx = 0;
    for (int i = 0; i < nattrs && revIndx < revealed_sz; i++) {
	if (revealed[revIndx] == i) {
	    int idx = (i+1) / 8;
	    int bit = (i+1) % 8;
	    bit_vec[idx] |= 1 << bit;
	    revIndx++;
	}
    }

    int i = 0;
    int j = (nattrs / 8);
    char temp;
    
    while (i < j) {
        // Swap s[i] and s[j]
        temp = bit_vec[i];
        bit_vec[i] = bit_vec[j];
        bit_vec[j] = temp;
        
        // Move towards the middle
        i++;
        j--;
    }

    memcpy(offset, zktokenBytes, zktokenBytes_sz);
    *res_sz = 2 + (nattrs / 8) + 1 + zktokenBytes_sz;

    return 0;
}

int main(int argc, char ** argv) { // Args: pkeyBase58, indexes, epoch, attributes

	DPABC_session session;
	int nattr = argc - 5; // Discard first 5 arguments
	char pk_buffer[1024];
	char formated[1024];
	size_t formated_sz;
	char * pk;
	size_t pk_sz;
	char nonce[1024];
	size_t nonce_sz;
	char * tokenBytes;
	size_t zksize;
	char * Binaryepoch;
	char * sign_id = "test_signature";

	for (int i = 1; i < argc; i++) {
		printf("argument: %s\n", argv[i]);
	}

	char * pkeyBase58 = argv[1];
	char * indexes = argv[2];
	char * base64Nonce = argv[3];
	char * epochAttr = argv[4];

	DPABC_initialize(&session);

	// Zp * attr[] = {zpFromInt(0), zpFromInt(1), zpFromInt(-1),
	// 			zpFromInt(0), zpFromInt(1), zpFromInt(-1)};
	
	// if (DPABC_get_key(&session, keyID, &pk, &pk_sz) != STATUS_OK) {
	// 	printf("Error getting public key\n");
	// 	return 1;
	// }
	
	// base58_decode(pkeyBase58, pk, &pk_sz);
	pk_sz = DecodeBase58((unsigned char *)pkeyBase58, strlen(pkeyBase58), (unsigned char *)pk_buffer) - 2;

	if (base64_decode(base64Nonce, nonce, &nonce_sz)) {
	    printf("Error decoding base64 nonce: %s\n", base64Nonce);
	    return 1;
	}

	printf("Nonce?: ");

	for (int i = 0; i < nonce_sz; i++) {
	    printf("0x%02X, ", nonce[i]);
	}
	printf("\n");
	
	pk = pk_buffer + 2; // We don't care about first 2 bytes
	
	Zp ** attr = malloc(sizeof(Zp*) * nattr);
	for (int i = 0; i < nattr; i++) {
		attr[i] = hashToZp(argv[i+5], strlen(argv[i+5]));
	}

	Zp * epoch = hashToZp(epochAttr, strlen(epochAttr));

	int * indexReveal = malloc(nattr * sizeof(int));
	int indexReveal_sz = 0;

	char * token = strtok(indexes, ",");
	for (int i = 0; i < nattr && token; i++) {
		indexReveal[i] = atoi(token);
		indexReveal_sz++;
		token = strtok(NULL, ",");
		printf("index %d: %d\n", i, indexReveal[i]);
	}

	printf("Index Reveal Size: %d\n", indexReveal_sz);

	// epoch = decode_base64(argv[2]);

	char * binaryAttr = malloc(nattr*zpByteSize());
	for (int i = 0; i < nattr; i++)
		zpToBytes(binaryAttr + i*zpByteSize(), attr[i]);

	Binaryepoch = malloc(zpByteSize());
	zpToBytes(Binaryepoch, epoch);

	if (DPABC_generateZKtoken(&session, 
			      pk, pk_sz,
			      sign_id, 
			      Binaryepoch, zpByteSize(), 
		              binaryAttr, nattr * zpByteSize(), 
			      indexReveal, indexReveal_sz * sizeof(int), 
		              nonce, nonce_sz, 
			      &tokenBytes, &zksize) != STATUS_OK) 
	{
		printf("Error generating zkToken\n");
		return 1;
	}

	zkToken * tokencete = dpabcZkFromBytes(tokenBytes);
	publicKey * pkceta = dpabcPkFromBytes(pk);
	Zp ** revealeditos = malloc(indexReveal_sz * sizeof(Zp*));
	for (int i = 0; i < indexReveal_sz; i++) {
	    revealeditos[i] = attr[indexReveal[i]];
	}

	if (!verifyZkToken(tokencete, pkceta, epoch, (const Zp**)revealeditos, (int*)indexReveal, indexReveal_sz, nonce, nonce_sz)) {
	    printf("Algo salio mal\n");
	} else {
	    printf("Todo ok\n");
	}

	if(format(nattr, indexReveal, indexReveal_sz, tokenBytes, zksize, formated, &formated_sz)) {
	    printf("Error formating zkToken\n");
	    return 1;
	}

	char * zkPostFormat = malloc(zksize);

	memcpy(zkPostFormat, formated + 3, formated_sz - 3);
	zkToken * composedZkPostFormat = dpabcZkFromBytes(zkPostFormat);

	if (!verifyZkToken(composedZkPostFormat, pkceta, epoch, (const Zp**)revealeditos, (int*)indexReveal, indexReveal_sz, nonce, nonce_sz)) {
	    printf("Algo salio mal con el recomposed\n");
	} else {
	    printf("Todo ok con el recomposed\n");
	}

	printf("zktoken:");
	fwrite(formated, 1, formated_sz, stdout);

	free(binaryAttr);
	free(Binaryepoch);
	DPABC_finalize(&session); 
	return 0;
}
