#include <custom_se_pkcs11.h>
#include <stdint.h>
#include <tee_client_api.h>
#include <security_api_ta.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PSK_SIZE		16
#define P_RANDOM_SZ		16
#define CMAC_SIGNATURE_SIZE	16
#define CRC_16_SIGNATURE_SIZE	2

#define PSK_ID			"PSK"
#define MSK_ID			"MSK"
#define AK_ID			"rfc4764_ak"
#define MSK_DERIVED_AK_ID	"msk_derived_ak"
#define MUD_URL_ID		"mud_url"
#define CERTIFICATE_ID		"certificate"

#define BOOTSTRAP_STORED_SZ	6		

#define MAX_EDK			9

#define AES_BLOCK_SIZE		16

// Security api does not provide session information
// needed by op-tee int it's calls, so it needs to be
// handeled as a global variable

TEEC_Context ctx;
TEEC_Session session;

const char * edk_ids[] = {"EDK_01", "EDK_02", "EDK_03",
			  "EDK_04", "EDK_05", "EDK_06",
			  "EDK_07", "EDK_08", "EDK_09"};

int32_t store_to_ta(char * id, char * data, size_t data_sz) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE,
					 TEEC_NONE);


	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = strlen(id);

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_sz;


	printf("Invoking TA to store\n");
	res = TEEC_InvokeCommand(&(session), TA_STORE, &op,
				 &err_origin);

	if (res != TEEC_SUCCESS) {
		printf("store_to_ta failed: 0x%x / %u\n", res, err_origin);
		return 1;
	}
	printf("TA stored successfully\n");

	return 0;

}


int32_t csp_reconfigure(uint8_t opcode, unsigned char * operationData, uint16_t operationDataLen) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	switch (opcode) {
		case 0x01: // Change signature algorithm
			if (!operationData) {
				printf("Error: no data provided for signature change\n");
				return 1;
			}
			op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, 
							 TEEC_NONE,
							 TEEC_NONE,
							 TEEC_NONE);
			op.params[0].value.a = (uint32_t)operationData[0];
			printf("Invoking TA to change signature algorithm: %x\n", operationData[0]);
			res = TEEC_InvokeCommand(&(session), TA_CHANGE_SIGNATURE_ALG, &op,
						 &err_origin);

			if (res != TEEC_SUCCESS) {
				printf("reset node failed: 0x%x / %u\n", res, err_origin);
				return 1;
			}
			printf("TA changed algorithm successfully\n");
		break;
		case 0x04: // Reset the IoT node/device (no data)
			op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, 
							 TEEC_NONE,
							 TEEC_NONE,
							 TEEC_NONE);
			printf("Invoking TA to reset node\n");
			res = TEEC_InvokeCommand(&(session), TA_WIPE_BOOTSTRAP, &op,
						 &err_origin);

			if (res != TEEC_SUCCESS) {
				printf("reset node failed: 0x%x / %u\n", res, err_origin);
				return 1;
			}
			printf("TA reset node successfully\n");
		break;
		default:
			printf("Invalid reconfiguration opcode: 0x%x\n", opcode);
			res = -1;
	}



	return res;

}

static int32_t retreive_from_ta(char * id, char * data_buffer, size_t * buffer_sz) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE);


	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = strlen(id);

	op.params[1].tmpref.buffer = data_buffer;
	op.params[1].tmpref.size = *buffer_sz;


	printf("Invoking TA to retreive from secure storage\n");
	res = TEEC_InvokeCommand(&(session), TA_RETREIVE, &op,
				 &err_origin);

	*buffer_sz = op.params[1].tmpref.size;

	printf("retreived cert at retreive_from_ta: ");
	printf("0x");
	for (uint32_t i = 0; i < *buffer_sz; i++) printf("%02x ", ((char*)op.params[1].tmpref.buffer)[i]);
	printf("\n");

	if (res != TEEC_SUCCESS) {
		printf("retreive_from_ta failed: 0x%x / %u\n", res, err_origin);
		return 1;
	}
	printf("TA retreived successfully\n");

	return 0;

}

int32_t csp_initialize(void) {

	
	uint32_t err_origin;
	TEEC_Result res;

	TEEC_UUID uuid = TA_SECURITY_API_UUID;
	/* Initialize a context connecting us to the TEE */
        res = TEEC_InitializeContext(NULL, &ctx);
        if (res != TEEC_SUCCESS) {
		printf("TA context initialization error\n");
		return 1;
	}

        /*
         * Open a session to the "hello world" TA, the TA will print "hello
         * world!" in the log when the session is created.
         */
        res = TEEC_OpenSession(&ctx, &session, &uuid,
                               TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);

	if (res != TEEC_SUCCESS) {
		printf("TA session open error\n");
		return 1;
	}

	return 0;
}

int32_t csp_installPSK(unsigned char * pSKValue) {
	
	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;


	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);


	op.params[0].tmpref.buffer = pSKValue;
	op.params[0].tmpref.size = PSK_SIZE;


	printf("Invoking TA to install PSK\n");
	res = TEEC_InvokeCommand(&(session), TA_INSTALL_PSK, &op,
				 &err_origin);

	if (res != TEEC_SUCCESS) {
		printf("installPSK failed: 0x%x / %u\n", res, err_origin);
		return 1;
	}
	printf("TA installed PSK\n");

	return 0;
}

int32_t csp_installMudURL(char* mUDuRLValue) {
	return store_to_ta(MUD_URL_ID, mUDuRLValue, strlen(mUDuRLValue));
}


int32_t csp_getMuDFileURL(char* mUDuRLValue) {
	size_t buffer_sz = strlen(mUDuRLValue);
	int32_t res = retreive_from_ta(MUD_URL_ID, mUDuRLValue, &buffer_sz);
	if (res) {
		printf("Error retreiving Mud URL value (maybe buffer too small? Mud URL size: %d)\n", (int)buffer_sz);
	}
	return res;
}

int32_t csp_getCertificateValue(unsigned char* certificateValue, uint16_t * certificateValueLen) {
	size_t buffer_sz = *certificateValueLen;
	int32_t res = retreive_from_ta(CERTIFICATE_ID, (char*)certificateValue, &buffer_sz);
	if (res) {
		printf("Error retreiving Mud URL value (maybe buffer too small? Mud URL size: %d)\n", (int)buffer_sz);
	}
	// certificateValue[buffer_sz] = '\0';
	*certificateValueLen = buffer_sz;
	return res;

}

int32_t csp_installCertificate(unsigned char* certificateValue, uint16_t certificateValueLen) {
	return store_to_ta(CERTIFICATE_ID, (char *)certificateValue, certificateValueLen);
}

int32_t csp_deriveKey(char* baseKeyID, char* derivedKeyID, unsigned char* salt, unsigned char* info, uint16_t infoLen, uint16_t algo) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	if (algo != AES128) {
		printf("Algorithm not implemented\n");
		return 2;
	}

	if (!strcmp(baseKeyID, PSK_ID) && !strcmp(derivedKeyID, MSK_ID)) {

		memset(&op, 0, sizeof(op));

		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
						 TEEC_NONE,
						 TEEC_NONE,
						 TEEC_NONE);


		op.params[0].tmpref.buffer = salt;
		op.params[0].tmpref.size = P_RANDOM_SZ;


		printf("Invoking TA to derive MSK\n");
		res = TEEC_InvokeCommand(&(session), TA_DERIVE_MSK, &op,
					 &err_origin);

		if (res != TEEC_SUCCESS) {
			printf("derive MSK failed: 0x%x / %u\n", res, err_origin);
			return 1;
		}
		printf("TA generated key\n");

		return 0;

	}

	for (int i = 0; i < MAX_EDK; i++) {

		if (!strcmp(baseKeyID, MSK_ID) && !strcmp(derivedKeyID, edk_ids[i])) {

			memset(&op, 0, sizeof(op));

			op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
							 TEEC_VALUE_INPUT,
							 TEEC_NONE,
							 TEEC_NONE);


			op.params[0].tmpref.buffer = salt;
			op.params[0].tmpref.size = P_RANDOM_SZ;
			
			op.params[1].value.a = i+1;

			printf("Invoking TA to derive EDK\n");
			res = TEEC_InvokeCommand(&(session), TA_DERIVE_EDK, &op,
						 &err_origin);

			if (res != TEEC_SUCCESS) {
				printf("derive EDK failed: 0x%x / %u\n", res, err_origin);
				return 1;
			}
			printf("TA generated key\n");

			return 0;

		}
	}

	printf("Can not generate %s from %s\n", derivedKeyID, baseKeyID);
	return 2;

}


int32_t csp_sign(char* signatureKeyID, unsigned char* dataToSign, uint16_t dataToSignLen, unsigned char* signature) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE);

	if (!strcmp(signatureKeyID, PSK_ID)) {

		op.params[0].tmpref.buffer = AK_ID;
		op.params[0].tmpref.size = strlen(AK_ID);

	} else if (!strcmp(signatureKeyID, MSK_ID)) {
	
		op.params[0].tmpref.buffer = MSK_DERIVED_AK_ID;
		op.params[0].tmpref.size = strlen(MSK_DERIVED_AK_ID);

	} else {
		printf("Invalid key id: %s\n", signatureKeyID);
		return 2;
	}

	op.params[1].tmpref.buffer = dataToSign;
	op.params[1].tmpref.size = dataToSignLen;

	op.params[2].tmpref.buffer = signature;
	op.params[2].tmpref.size = CMAC_SIGNATURE_SIZE;

	printf("Invoking TA to sign\n");
	res = TEEC_InvokeCommand(&(session), TA_SIGN, &op,
				 &err_origin);

	if (res != TEEC_SUCCESS) {
		printf("sign failed: 0x%x / %u\n", res, err_origin);
		return 1;
	}
	printf("TA signed\n");

	return 0;
}

int32_t csp_encryptData(char* encriptionKeyId, unsigned char* dataToEncrypt, uint16_t dataToEncryptLen, unsigned char* encryptedData, uint16_t algo) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	if (algo != AES_256_CBC) {
		printf("Algorithm not implemented\n");
		return 2;
	}

	for (int i = 0; i < MAX_EDK; i++) {

		if (!strcmp(encriptionKeyId, edk_ids[i])) {

			memset(&op, 0, sizeof(op));

			op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
							 TEEC_MEMREF_TEMP_INPUT,
							 TEEC_MEMREF_TEMP_OUTPUT,
							 TEEC_NONE);


			op.params[0].tmpref.buffer = encriptionKeyId;
			op.params[0].tmpref.size = strlen(encriptionKeyId);

			op.params[1].tmpref.buffer = dataToEncrypt;
			op.params[1].tmpref.size = dataToEncryptLen;

			op.params[2].tmpref.buffer = encryptedData;
			op.params[2].tmpref.size = dataToEncryptLen + AES_BLOCK_SIZE - (dataToEncryptLen % AES_BLOCK_SIZE);

			printf("Invoking TA to cipher\n");
			res = TEEC_InvokeCommand(&(session), TA_AES_256, &op,
						 &err_origin);

			if (res != TEEC_SUCCESS) {
				printf("cipher failed: 0x%x / %u\n", res, err_origin);
				return 1;
			}

			printf("TA cipher\n");
			return 0;
		}
	}

	printf("Invalid key id: %s\n", encriptionKeyId);
	return 2;
}

int32_t csp_decryptData(char* decryptionKeyID, unsigned char* dataToDecrypt, uint16_t dataToDecryptLen, unsigned char* decryptedData, uint16_t * decryptedDataLen, uint16_t algo) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	if (algo != AES_256_CBC) {
		printf("Algorithm not implemented\n");
		return 2;
	}

	for (int i = 0; i < MAX_EDK; i++) {

		if (!strcmp(decryptionKeyID, edk_ids[i])) {

			memset(&op, 0, sizeof(op));

			op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
							 TEEC_MEMREF_TEMP_INPUT,
							 TEEC_MEMREF_TEMP_OUTPUT,
							 TEEC_NONE);


			op.params[0].tmpref.buffer = decryptionKeyID;
			op.params[0].tmpref.size = strlen(decryptionKeyID);

			op.params[1].tmpref.buffer = dataToDecrypt;
			op.params[1].tmpref.size = dataToDecryptLen;


			op.params[2].tmpref.buffer = decryptedData;
			op.params[2].tmpref.size = dataToDecryptLen;

			printf("Invoking TA to decode\n");
			res = TEEC_InvokeCommand(&(session), TA_AES_DECODE_256, &op,
						 &err_origin);

			if (res != TEEC_SUCCESS) {
				printf("decode failed: 0x%x / %u\n", res, err_origin);
				return 1;
			}
				printf("TA decoded\n");

			*decryptedDataLen = op.params[2].tmpref.size;

			return 0;
		}
	}

	printf("Invalid key id: %s\n", decryptionKeyID);
	return 2;
}

int32_t csp_generateRandom(unsigned char * randomBuffer, uint16_t randomBufferLen) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;


	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, 
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);


	op.params[0].tmpref.buffer = randomBuffer;
	op.params[0].tmpref.size = randomBufferLen;


	printf("Invoking TA to install generate random buffer\n");
	res = TEEC_InvokeCommand(&(session), TA_GENERATE_RANDOM, &op,
				 &err_origin);

	if (res != TEEC_SUCCESS) {
		printf("generateRandom failed: 0x%x / %u\n", res, err_origin);
		return 1;
	}
	printf("TA generated random buffer\n");

	return 0;
}

int32_t csp_terminate(void) {
	/*
	 * We're done with the TA, close the session and
	 * destroy the context.
	 *
	 * The TA will print "Goodbye!" in the log when the
	 * session is closed.
	 */

	TEEC_CloseSession(&(session));
	TEEC_FinalizeContext(&(ctx));

	return 0;
}

