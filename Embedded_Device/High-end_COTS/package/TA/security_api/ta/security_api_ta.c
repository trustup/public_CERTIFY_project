#include <stdio.h>
#include <string.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <utils.h>

#include <security_api_ta.h>

#define AK_ID				"rfc4764_ak"
#define AK_ID_SZ			10

#define KDK_ID				"rfc4764_kdk"
#define KDK_ID_SZ			11

#define MSK_ID				"rfc4764_msk"
#define MSK_ID_SZ			11

#define MSK_DERIVED_AK_ID		"msk_derived_ak"
#define MSK_DERIVED_AK_ID_SZ		14

#define MSK_DERIVED_KDK_ID		"msk_derived_kdk"
#define MSK_DERIVED_KDK_ID_SZ		15

#define EDK_ID				"EDK"
#define EDK_ID_SZ			3

#define SIGNATURE_ALGORITHM_ID		"signature_algorithm"
#define SIGNATURE_ALGORITHM_ID_SZ	19	

#define NUM_STORED_ELEMENTS		4	



#define DEFAULT_SIGNATURE_ALGORITHM	0x01

const char * stored_bootstrap_elements[] = { 
					MSK_ID,
					MSK_DERIVED_AK_ID, 
					MSK_DERIVED_KDK_ID,
					"EDK_01",
					};


const int stored_bootstrap_elements_sz[] = { 
					MSK_ID_SZ,
					MSK_DERIVED_AK_ID_SZ, 
					MSK_DERIVED_KDK_ID_SZ,
					6,
					};
/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	(void)param_types; // unused

	DMSG("has been called");

	/* Unused parameter */
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	IMSG("Session created\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye!\n");
}


static TEE_Result key_setup(char * psk, char * ak_id, size_t ak_id_sz, char * kdk_id, size_t kdk_id_sz) {

	char input_block[AES128_BLOCK_SIZE] = { 0 };
	char inter_block[AES128_BLOCK_SIZE] = { 0 };
	char inter_block_b[AES128_BLOCK_SIZE] = { 0 };
	char ak[AES128_BLOCK_SIZE] = { 0 };
	char kdk[AES128_BLOCK_SIZE] = { 0 };

	size_t dst_sz;

	TEE_Result res;

	aes_session sess;


	DMSG("Value of psk: ");
	DBG_print_block(psk);

	res = AES_128_init(psk, &sess);

	if (res != TEE_SUCCESS) {
		EMSG("AES initialization failed: 0x%08x", res);
		return res;
	}

	dst_sz = AES128_BLOCK_SIZE;
	res = AES_128_cipher(sess.op_handle, input_block, AES128_BLOCK_SIZE, inter_block, &dst_sz);

	if (res != TEE_SUCCESS) {
		EMSG("AES cipher failed: 0x%08x", res);
		return res;
	}
	
	TEE_MemMove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
	inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x01;
	
	dst_sz = AES128_BLOCK_SIZE;
	res = AES_128_cipher(sess.op_handle, inter_block_b, AES128_BLOCK_SIZE, ak, &dst_sz);

	if (res != TEE_SUCCESS) {
		EMSG("AES cipher failed: 0x%08x", res);
		return res;
	}
	DMSG("Value of ak (%ld): ", dst_sz);
	DBG_print_block(ak);

	TEE_MemMove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
	inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x02;

	dst_sz = AES128_BLOCK_SIZE;
	res = AES_128_cipher(sess.op_handle, inter_block_b, AES128_BLOCK_SIZE, kdk, &dst_sz);

	if (res != TEE_SUCCESS) {
		EMSG("AES cipher failed: 0x%08x", res);
		return res;
	}


	DMSG("Value of kdk (%ld): ", dst_sz);
	DBG_print_block(kdk);

	// Get generated keys in secure storage
	uint32_t ak_data_flag =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
				 TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
				 TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */

	res = create_write(ak_data_flag, ak_id, ak_id_sz, ak, AES128_BLOCK_SIZE); 
	
	if (res != TEE_SUCCESS) {
		EMSG("Failed to store AK: 0x%08x", res);
		return res;
	}

	uint32_t kdk_data_flag = ak_data_flag;

	res = create_write(kdk_data_flag, kdk_id, kdk_id_sz, kdk, AES128_BLOCK_SIZE);
	
	if (res != TEE_SUCCESS) {
		EMSG("Failed to store KDK: 0x%08x", res);
		return res;
	}

	AES_128_terminate(&sess);

	return TEE_SUCCESS;

}

static TEE_Result reduce_and_install_msk(char * msk) {
	
	char msk_hashed[AES128_BLOCK_SIZE * 2];
	aes_session aes_sess;

	TEE_Result res = SHA_256_init(&aes_sess);

	if (res != TEE_SUCCESS) {
		EMSG("SHA init failed: 0x%08x", res);
		return res;
	} 

	size_t hashed_size = AES128_BLOCK_SIZE * 2;

	res = SHA_256_digest(aes_sess.op_handle, msk, AES128_BLOCK_SIZE * 4, msk_hashed, &hashed_size);

	if (res != TEE_SUCCESS) {
		EMSG("SHA digest failed: 0x%08x", res);
		return res;
	} 

	if (hashed_size != AES128_BLOCK_SIZE * 2) {
		EMSG("Incorrect output size for SHA digest");
		return TEE_ERROR_BAD_STATE;
	} 

	res = SHA_256_terminate(&aes_sess);

	if (res != TEE_SUCCESS) {
		EMSG("SHA termination failed: 0x%08x", res);
		return res;
	} 

	res = key_setup(msk_hashed, 
		MSK_DERIVED_AK_ID, MSK_DERIVED_AK_ID_SZ, 
		MSK_DERIVED_KDK_ID, MSK_DERIVED_KDK_ID_SZ);

	if (res != TEE_SUCCESS) {
		EMSG("Reduced MSK installation failed: 0x%08x", res);
		return res;
	} 

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */


	res = create_write(flags, MSK_ID, MSK_ID_SZ, msk, AES128_BLOCK_SIZE * 4);
	
	if (res != TEE_SUCCESS) {
		EMSG("Failed to store MSK: 0x%08x", res);
		return res;
	}

	DMSG("Successfully stored MSK: ");
	DBG_print_extended_block(msk, 64);

	return res;

}

static TEE_Result reduce_and_install_edk(char * key, unsigned char session) {
	
	char edk[AES128_BLOCK_SIZE * 2];
	char sess_str[2];
	char * edk_id;
	size_t edk_id_sz;
	aes_session aes_sess;

	if (session > 99) {
		EMSG("Invalid session number");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	TEE_Result res = SHA_256_init(&aes_sess);

	if (res != TEE_SUCCESS) {
		EMSG("SHA init failed: 0x%08x", res);
		return res;
	} 

	size_t hashed_size = AES128_BLOCK_SIZE * 2;

	res = SHA_256_digest(aes_sess.op_handle, key, AES128_BLOCK_SIZE * 4, edk, &hashed_size);

	if (res != TEE_SUCCESS) {
		EMSG("SHA digest failed: 0x%08x", res);
		return res;
	} 

	if (hashed_size != AES128_BLOCK_SIZE * 2) {
		EMSG("Incorrect output size for SHA digest");
		return TEE_ERROR_BAD_STATE;
	} 

	res = SHA_256_terminate(&aes_sess);

	if (res != TEE_SUCCESS) {
		EMSG("SHA termination failed: 0x%08x", res);
		return res;
	} 

	edk_id_sz = EDK_ID_SZ + 3;
	edk_id = TEE_Malloc(edk_id_sz, TEE_MALLOC_FILL_ZERO);
	TEE_MemMove(edk_id, EDK_ID, EDK_ID_SZ);
	edk_id[EDK_ID_SZ] = '_';
	edk_id[EDK_ID_SZ + 1] = '0' + (session / 10);
	edk_id[EDK_ID_SZ + 2] = '0' + (session % 10);

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */
	
	// char trampas[64] = {
	//     0xc3, 0x4d, 0x31, 0x77, 0x23, 0x97, 0x80, 0xf2,
	//     0x48, 0x66, 0xbc, 0x9a, 0x51, 0x9a, 0xe3, 0xce,
	//     0x32, 0x5d, 0x53, 0x66, 0xf5, 0x15, 0x2e, 0x2d,
	//     0xb9, 0x15, 0x27, 0x92, 0x85, 0x67, 0x2b, 0xaa
	// };


	res = create_write(flags, edk_id, edk_id_sz, key, AES128_BLOCK_SIZE * 2);
	
	if (res != TEE_SUCCESS) {
		EMSG("Failed to store EDK: 0x%08x", res);
		return res;
	}

	IMSG("Successfully installed EDK: ");
	DBG_print_extended_block(edk, 32);

	return res;
}

// A value of session equal to 0 indicates the generation of the original MSK
static TEE_Result derive_session_keys(char * rand_p, char * kdk_id, size_t kdk_id_sz, unsigned char session) {

	TEE_Result res;
	aes_session aes_sess;
	char kdk[AES128_KEY_BYTE_SIZE];

	char tek[AES128_BLOCK_SIZE];
	char key[AES128_BLOCK_SIZE * 4];
	char emsk[AES128_BLOCK_SIZE * 4];
	char inter_block[AES128_BLOCK_SIZE];
	char inter_block_b[AES128_BLOCK_SIZE];

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ;

	size_t dst_sz;


	size_t read_bytes = AES128_KEY_BYTE_SIZE;
	res = read_raw_object(kdk_id, kdk_id_sz, kdk, AES128_KEY_BYTE_SIZE, &read_bytes, flags);

	if (res != TEE_SUCCESS || read_bytes != AES128_KEY_BYTE_SIZE) {
		EMSG("Failed to retreive KDK: 0x%08x", res);
		return res;
	}

	res = AES_128_init(kdk, &aes_sess);

	if (res != TEE_SUCCESS) {
		EMSG("AES init failed: 0x%08x", res);
		return res;
	} 

	dst_sz = AES128_BLOCK_SIZE;
	res = AES_128_cipher(aes_sess.op_handle, rand_p, AES128_BLOCK_SIZE, inter_block, &dst_sz);

	if (res != TEE_SUCCESS) {
		EMSG("AES cipher failed: 0x%08x", res);
		return res;
	}
	
	TEE_MemMove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
	inter_block_b[AES128_BLOCK_SIZE-1] ^= 0x01;
	
	dst_sz = AES128_BLOCK_SIZE;
	res = AES_128_cipher(aes_sess.op_handle, inter_block_b, AES128_BLOCK_SIZE, tek, &dst_sz);

	if (res != TEE_SUCCESS) {
		EMSG("AES cipher failed: 0x%08x", res);
		return res;
	}

	for (uint8_t i = 0; i < 4; i++) {
		TEE_MemMove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
		inter_block_b[AES128_BLOCK_SIZE-1] ^= (0x02 + i);

		dst_sz = AES128_BLOCK_SIZE;
		res = AES_128_cipher(aes_sess.op_handle, inter_block_b, AES128_BLOCK_SIZE, key + AES128_BLOCK_SIZE * i, &dst_sz);

		if (res != TEE_SUCCESS) {
			EMSG("AES cipher failed: 0x%08x", res);
			return res;
		}
	}

	for (uint8_t i = 0; i < 4; i++) {
		TEE_MemMove(inter_block_b, inter_block, AES128_BLOCK_SIZE);
		inter_block_b[AES128_BLOCK_SIZE-1] ^= (0x05 + i);

		dst_sz = AES128_BLOCK_SIZE;
		res = AES_128_cipher(aes_sess.op_handle, inter_block_b, AES128_BLOCK_SIZE, emsk + AES128_BLOCK_SIZE * i, &dst_sz);

		if (res != TEE_SUCCESS) {
			EMSG("AES cipher failed: 0x%08x", res);
			return res;
		}
	}

	res = AES_128_terminate(&aes_sess);

	if (res != TEE_SUCCESS) {
		EMSG("AES termination failed: 0x%08x", res);
		return res;
	} 

	// If we are generating the original MSK, we install its reduced
	// version for future key derivation
	if (!session) {

		res = reduce_and_install_msk(key);

		if (res != TEE_SUCCESS) {
			EMSG("Failed to reduce MSK: 0x%08x", res);
			return res;
		}

	} else {

		res = reduce_and_install_edk(key, session);

		if (res != TEE_SUCCESS) {
			EMSG("Failed to reduce key to EDK: 0x%08x", res);
			return res;
		}

	}

	return res;

}

static TEE_Result installPSK(uint32_t param_types,
	TEE_Param params[4])
{

	TEE_Result res;
	char * default_signature_algorithm_buff;
	size_t default_signature_algorithm_buff_sz = 4;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	
	char psk[AES128_KEY_BYTE_SIZE];

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	if (params[0].memref.size != AES128_KEY_BYTE_SIZE) 
		return TEE_ERROR_BAD_PARAMETERS;

	default_signature_algorithm_buff = TEE_Malloc(default_signature_algorithm_buff_sz, TEE_MALLOC_FILL_ZERO);
	if (!default_signature_algorithm_buff) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	default_signature_algorithm_buff[0] = DEFAULT_SIGNATURE_ALGORITHM;

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */

	res = create_write(flags, SIGNATURE_ALGORITHM_ID, SIGNATURE_ALGORITHM_ID_SZ, default_signature_algorithm_buff, default_signature_algorithm_buff_sz);
	if (res != TEE_SUCCESS) {
		EMSG("Error while attempting to write default signature algorithm to TA: 0x%08x\n", res);	
	}

	TEE_Free(default_signature_algorithm_buff);

	TEE_MemMove(psk, params[0].memref.buffer, AES128_KEY_BYTE_SIZE);

	return key_setup(psk, AK_ID, AK_ID_SZ, KDK_ID, KDK_ID_SZ);

}

static TEE_Result deriveMSK(uint32_t param_types,
	TEE_Param params[4])
{

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	
	char rand_p[AES128_KEY_BYTE_SIZE];

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	if (params[0].memref.size != AES128_KEY_BYTE_SIZE) 
		return TEE_ERROR_BAD_PARAMETERS;

	TEE_MemMove(rand_p, params[0].memref.buffer, AES128_KEY_BYTE_SIZE);

	return derive_session_keys(rand_p, KDK_ID, KDK_ID_SZ, 0);

}

static TEE_Result deriveEDK(uint32_t param_types,
	TEE_Param params[4])
{

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	
	char rand_p[AES128_KEY_BYTE_SIZE];

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	if (params[0].memref.size != AES128_KEY_BYTE_SIZE) 
		return TEE_ERROR_BAD_PARAMETERS;

	TEE_MemMove(rand_p, params[0].memref.buffer, AES128_KEY_BYTE_SIZE);

	return derive_session_keys(rand_p, MSK_DERIVED_KDK_ID, MSK_DERIVED_KDK_ID_SZ, params[1].value.a);

}

static TEE_Result sign(uint32_t param_types,
	TEE_Param params[4])
{
	char * keyId;
	size_t keyId_sz;
	
	char * data;
	size_t data_sz;

	char algo_buff[4];
	uint8_t algo;

	char key[AES128_BLOCK_SIZE];
	aes_session sess;

	TEE_Result res;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ;
	size_t read_bytes = 4;
	res = read_raw_object(SIGNATURE_ALGORITHM_ID, SIGNATURE_ALGORITHM_ID_SZ, algo_buff, 4, &read_bytes, flags);

	if (res != TEE_SUCCESS) {
		EMSG("Error while reading signature algorithm: 0x%08x", res);
		return res;
	}

	algo = (uint8_t)(*algo_buff);

	keyId_sz = params[0].memref.size;
	keyId = TEE_Malloc(keyId_sz, TEE_MALLOC_FILL_ZERO);
	if (!keyId) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(keyId, params[0].memref.buffer, keyId_sz);


	data_sz = params[1].memref.size;
	data = TEE_Malloc(data_sz, TEE_MALLOC_FILL_ZERO);
	if (!data) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	read_bytes = AES128_KEY_BYTE_SIZE;
	res = read_raw_object(keyId, keyId_sz, key, AES128_KEY_BYTE_SIZE, &read_bytes, flags);

	if (res != TEE_SUCCESS || read_bytes != AES128_KEY_BYTE_SIZE) {
		EMSG("Failed to retreive KDK: 0x%08x", res);
		return res;
	}

	switch (algo) {
		case 0x00:
			if (params[2].memref.size != AES128_BLOCK_SIZE) {
				return TEE_ERROR_BAD_PARAMETERS;
			}

			IMSG("Calling init");

			res = AES_HMAC_MD5_init(key, &sess);
			if (res != TEE_SUCCESS) {
				EMSG("Failed to initializate HMAC: 0x%08x", res);
				return res;
			}

			IMSG("Calling cipher");
			size_t hmac_sz = AES128_BLOCK_SIZE;
			res = AES_HMAC_MD5_cipher(sess.op_handle, data, data_sz, params[2].memref.buffer, &params[2].memref.size);
			if (res != TEE_SUCCESS) {
				EMSG("HMAC failed : 0x%08x", res);
				return res;
			}

			DMSG("Size of generated HMAC: %ld", hmac_sz);

			AES_HMAC_MD5_terminate(&sess);

			return TEE_SUCCESS;
		break;
		case 0x01:
			if (params[2].memref.size != AES128_BLOCK_SIZE) {
				return TEE_ERROR_BAD_PARAMETERS;
			}

			res = AES_CMAC_128_init(key, &sess);
			if (res != TEE_SUCCESS) {
				EMSG("Failed to initializate AES CMAC: 0x%08x", res);
				return res;
			}

			size_t cmac_sz = AES128_BLOCK_SIZE;
			res = AES_CMAC_128_cipher(sess.op_handle, data, data_sz, params[2].memref.buffer, &params[2].memref.size);
			if (res != TEE_SUCCESS) {
				EMSG("AES CMAC failed : 0x%08x", res);
				return res;
			}

			DMSG("Size of generated CMAC: %ld", cmac_sz);

			AES_CMAC_128_terminate(&sess);

			return TEE_SUCCESS;
		break;
		default:
			EMSG("Unrecognized alrogithm: 0x%08x", algo);
			return TEE_ERROR_SIGNATURE_INVALID;
	
	}

}

static TEE_Result generateRandom(uint32_t param_types,
	TEE_Param params[4])
{

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	TEE_GenerateRandom(params[0].memref.buffer, params[0].memref.size);

	return TEE_SUCCESS;
}

static TEE_Result store(uint32_t param_types,
	TEE_Param params[4])
{
	TEE_Result res;
	char * storeId;
	size_t storeId_sz;

	char * data;
	size_t data_sz;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	storeId_sz = params[0].memref.size;
	storeId = TEE_Malloc(storeId_sz, TEE_MALLOC_FILL_ZERO);
	if (!storeId) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(storeId, params[0].memref.buffer, storeId_sz);


	data_sz = params[1].memref.size;
	data = TEE_Malloc(data_sz, TEE_MALLOC_FILL_ZERO);
	if (!data) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	IMSG("Will store in %.*s: ", (int)storeId_sz, storeId);
	DBG_print_extended_block(data, data_sz);

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */
	res = create_write(flags, storeId, storeId_sz, data, data_sz); 
	
	if (res != TEE_SUCCESS) {
		goto exit;
	}

exit:
	TEE_Free(storeId);
	TEE_Free(data);
	return res;

}

static TEE_Result wipe_bootstrap(uint32_t param_types,
	TEE_Param params[4])
{
	TEE_Result res;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */


	for (int i = 0; i < NUM_STORED_ELEMENTS; i++) {

		res = remove_object(flags, stored_bootstrap_elements[i], stored_bootstrap_elements_sz[i]); 
		
		if (res != TEE_SUCCESS) {
			return res;
		}
	}
	return TEE_SUCCESS;
}

static TEE_Result retreive(uint32_t param_types,
	TEE_Param params[4])
{
	TEE_Result res;
	char * storeId;
	size_t storeId_sz;
	size_t retreived_sz;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	storeId_sz = params[0].memref.size;
	storeId = TEE_Malloc(storeId_sz, TEE_MALLOC_FILL_ZERO);
	if (!storeId) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(storeId, params[0].memref.buffer, storeId_sz);


	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */
	res = read_raw_object(storeId, storeId_sz, params[1].memref.buffer, params[1].memref.size, &retreived_sz, flags); 
	
	params[1].memref.size = retreived_sz;
	printf("retreived cert at sec_api: ");
	printf("0x");
	for (uint32_t i = 0; i < params[1].memref.size; i++) printf("%02x ", ((char*)params[1].memref.buffer)[i]);
	printf("\n");

	if (res != TEE_SUCCESS) {
		goto exit;
	}

exit:
	TEE_Free(storeId);
	return res;

}

static TEE_Result change_signature_alg(uint32_t param_types,
	TEE_Param params[4])
{
	TEE_Result res;
	char * algorithm;
	size_t algorithm_sz = 4; // uint32

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			  TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			  TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */

	algorithm = TEE_Malloc(algorithm_sz, TEE_MALLOC_FILL_ZERO);
	if (!algorithm) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(algorithm, &(params[0].value.a), algorithm_sz);

	res = remove_object(flags, SIGNATURE_ALGORITHM_ID, SIGNATURE_ALGORITHM_ID_SZ);
	if (res != TEE_SUCCESS) {
		EMSG("Error while attemting to remove previous signature algorithm\n");
		return res;
	}

	res = create_write(flags, SIGNATURE_ALGORITHM_ID, SIGNATURE_ALGORITHM_ID_SZ, algorithm, algorithm_sz);
	if (res != TEE_SUCCESS) {
		EMSG("Error while attemting to write new signature algorithm\n");
		return res;
	}

	return res;

}

static TEE_Result api_aes256(uint32_t param_types,
	TEE_Param params[4])
{
	char * keyId;
	size_t keyId_sz;
	
	char * data;
	size_t data_sz;

	char * padded_data;
	size_t padded_data_sz;

	char key[AES256_KEY_BYTE_SIZE];
	aes_session sess;

	TEE_Result res;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (params[2].memref.size != params[1].memref.size + AES256_BLOCK_SIZE - (params[1].memref.size % AES256_BLOCK_SIZE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	keyId_sz = params[0].memref.size;
	keyId = TEE_Malloc(keyId_sz, TEE_MALLOC_FILL_ZERO);
	if (!keyId) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(keyId, params[0].memref.buffer, keyId_sz);


	data_sz = params[1].memref.size;
	data = TEE_Malloc(data_sz, TEE_MALLOC_FILL_ZERO);
	if (!data) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	padded_data_sz = data_sz + AES256_BLOCK_SIZE - (data_sz % AES256_BLOCK_SIZE);
	padded_data = TEE_Malloc(padded_data_sz, TEE_MALLOC_FILL_ZERO);
	if (!padded_data) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	// ISO9797 M2 Padding

	TEE_MemMove(padded_data, data, data_sz);
	TEE_MemFill(padded_data + data_sz, 0x80, 1);
	TEE_MemFill(padded_data + data_sz + 1, 0x00, padded_data_sz - data_sz - 1);

	IMSG("Padded message");
	DBG_print_extended_block(padded_data, padded_data_sz);

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ;
	size_t read_bytes = AES256_KEY_BYTE_SIZE;
	res = read_raw_object(keyId, keyId_sz, key, AES256_KEY_BYTE_SIZE, &read_bytes, flags);
	DBG_print_extended_block(key, read_bytes);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to retreive key: 0x%08x", res);
		return res;
	}

	if (read_bytes != AES256_KEY_BYTE_SIZE ) {
		EMSG("Invalid size key: %lu\n", read_bytes);
		return TEE_ERROR_BAD_FORMAT;
	}


	res = AES_256_init(key, &sess);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to initializate AES 256: 0x%08x", res);
		return res;
	}

	res = AES_256_cipher(sess.op_handle, padded_data, padded_data_sz, params[2].memref.buffer, &params[2].memref.size);
	if (res != TEE_SUCCESS) {
		EMSG("AES 256 failed : 0x%08x", res);
		return res;
	}

	DMSG("Size of generated cypher: %d", params[2].memref.size);

	AES_256_terminate(&sess);

	return TEE_SUCCESS;
}

static TEE_Result api_aes_decode_256(uint32_t param_types,
	TEE_Param params[4])
{
	char * keyId;
	size_t keyId_sz;
	
	char * encoded_data;
	size_t encoded_data_sz;

	char * padded_data;
	size_t padded_data_sz;

	char key[AES256_KEY_BYTE_SIZE];
	aes_session sess;

	TEE_Result res;

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	keyId_sz = params[0].memref.size;
	keyId = TEE_Malloc(keyId_sz, TEE_MALLOC_FILL_ZERO);
	if (!keyId) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(keyId, params[0].memref.buffer, keyId_sz);


	encoded_data_sz = params[1].memref.size;
	encoded_data = TEE_Malloc(encoded_data_sz, TEE_MALLOC_FILL_ZERO);
	if (!encoded_data) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(encoded_data, params[1].memref.buffer, encoded_data_sz);

	padded_data_sz = encoded_data_sz;
	padded_data = TEE_Malloc(padded_data_sz, TEE_MALLOC_FILL_ZERO);
	if (!padded_data) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	uint32_t flags =  TEE_DATA_FLAG_ACCESS_READ;
	size_t read_bytes = AES256_KEY_BYTE_SIZE;
	res = read_raw_object(keyId, keyId_sz, key, AES256_KEY_BYTE_SIZE, &read_bytes, flags);
	DBG_print_extended_block(key, read_bytes);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to retreive key: 0x%08x", res);
		return res;
	}

	if (read_bytes != AES256_KEY_BYTE_SIZE ) {
		EMSG("Invalid size key: %lu\n", read_bytes);
		return TEE_ERROR_BAD_FORMAT;
	}


	res = AES_decode_256_init(key, &sess);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to initializate AES 256 in decode mode: 0x%08x", res);
		return res;
	}

	res = AES_256_cipher(sess.op_handle, encoded_data, encoded_data_sz, padded_data, &padded_data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("AES 256 failed : 0x%08x", res);
		return res;
	}

	if (padded_data_sz != encoded_data_sz) {
		EMSG("Error in decoded data, size does not match encode data: %lu", padded_data_sz);
		return TEE_ERROR_BAD_FORMAT;
	}

	IMSG("padded data size: %lu", padded_data_sz);
	DBG_print_extended_block(padded_data, padded_data_sz);

	AES_256_terminate(&sess);

	// ISO9797 M2 Padding
	
	for (int i = padded_data_sz-1; i >= 0; i--) {
		if (padded_data[i]) {
			if (padded_data[i] == (char)0x80) {
				TEE_MemMove(params[2].memref.buffer, padded_data, i);
				params[2].memref.size = i;
				IMSG("After unpadding: %u", i);
				DBG_print_extended_block(params[2].memref.buffer, i);
				return TEE_SUCCESS;
			}
			else {
				EMSG("Data decoded with incorrect padding");
				return TEE_ERROR_BAD_FORMAT;
			}
		}
	}

	return TEE_ERROR_BAD_FORMAT;
}


/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{

	switch (cmd_id) {
		case TA_INSTALL_PSK:
			return installPSK(param_types, params);
		case TA_DERIVE_MSK:
			return deriveMSK(param_types, params);
		case TA_DERIVE_EDK:
			return deriveEDK(param_types, params);
		case TA_SIGN:
			return sign(param_types, params);
		case TA_GENERATE_RANDOM:
			return generateRandom(param_types, params);
		case TA_STORE:
			return store(param_types, params);
		case TA_WIPE_BOOTSTRAP:
			return wipe_bootstrap(param_types, params);
		case TA_CHANGE_SIGNATURE_ALG:
			return change_signature_alg(param_types, params);
		case TA_RETREIVE:
			return retreive(param_types, params);
		case TA_AES_256:
			return api_aes256(param_types, params);
		case TA_AES_DECODE_256:
			return api_aes_decode_256(param_types, params);
		default:
			return TEE_ERROR_BAD_PARAMETERS;
	}
}

