#include "Dpabc_types.h"
#include <Zp.h>
#include <Dpabc.h>
#include <stdint.h>
#include <types_impl.h>

// #include <stdlib.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <dpabc_ta.h>



char client_auth[] = {0x00, 0x30, 0xd4, 0xc5, 0xbd, 0x4b, 0xd7, 0x0d, 0xb2, 0x91, 0xbb, 0xbd, 0xd6, 0x82, 0x87, 0x86, 0x04, 0x36, 0xf9, 0x18, 0x2e, 0x5f, 0x93, 0x3c, 0x5c, 0xfe, 0x58, 0x7f, 0x55, 0x65, 0x5b, 0x02};
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
	size_t hash_sz;	
	char * self_check;
	TEE_Result res;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* Unused parameter */
	(void)&sess_ctx;

	hash_sz = params[0].memref.size;

	self_check = TEE_Malloc(hash_sz, TEE_MALLOC_FILL_ZERO);
	if (!self_check) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(self_check, params[0].memref.buffer, hash_sz);

	IMSG("Received hash: ");
	for (size_t byte_id = 0; byte_id < hash_sz; byte_id++) {
		printf("%02x", self_check[byte_id]);
	}
	printf("\n");

	if (!USE_CLIENT_AUTH) {
		IMSG("Session created\n");
		res = TEE_SUCCESS;
		goto free_ret;
	}

	char cmp = (hash_sz == CLIENT_AUTH_SIZE);
	if (cmp) {
		for (uint8_t i = 0; i < CLIENT_AUTH_SIZE; i++) {
			if (self_check[i] != client_auth[i]) cmp = 0;
		}
	}

	if (!cmp) {
		IMSG("Invalid credentials\n");
		res = TEE_ERROR_ACCESS_DENIED;
		goto free_ret;
	}

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	IMSG("Session created\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	res = TEE_SUCCESS;

free_ret:
	TEE_Free(self_check);
	return res;
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

TEE_Result create_raw_object(uint32_t obj_data_flag, char * obj_id, size_t obj_id_sz, char * data, size_t data_sz) {

	TEE_ObjectHandle object;
	TEE_Result res;

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					obj_data_flag,
					TEE_HANDLE_NULL,
					NULL, 0,		/* we may not fill it right now */
					&object);

	if (res != TEE_SUCCESS) {
		EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
		return res;
	}


	res = TEE_WriteObjectData(object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(object);
	} else {
		TEE_CloseObject(object);
	}

	return TEE_SUCCESS;
}

TEE_Result get_raw_object_size(char * obj_id, size_t obj_id_sz, size_t * object_size) {
	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;

	/*
	 * Check the object exist and can be dumped into output buffer
	 * then dump it.
	 */
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to create persistent object, res=0x%08x", res);
		TEE_CloseObject(object);
		return res;
	}

	*object_size = object_info.dataSize;
	return res;
}

TEE_Result read_raw_object(char * obj_id, size_t obj_id_sz, char * data, size_t data_sz, uint32_t * read_bytes) {

	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;

	/*
	 * Check the object exist and can be dumped into output buffer
	 * then dump it.
	 */
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to create persistent object, res=0x%08x", res);
		TEE_CloseObject(object);
		return res;
	}

	if (object_info.dataSize > data_sz) {
		/*
		 * Provided buffer is too short.
		 * Return the expected size together with status "short buffer"
		 */
		res = TEE_ERROR_SHORT_BUFFER;
		TEE_CloseObject(object);
		return res;
	}

	res = TEE_ReadObjectData(object, data, object_info.dataSize,
				 read_bytes);
	if (res != TEE_SUCCESS || *read_bytes != object_info.dataSize) {
		EMSG("TEE_ReadObjectData failed 0x%08x, read %" PRIu32 " over %u",
				res, read_bytes, object_info.dataSize);
		TEE_CloseObject(object);
		return res;
	}

	TEE_CloseObject(object);
	return res;
}

TEE_Result retreive_secret_key(char * key_id, size_t key_id_sz, secretKey ** sk) {

	TEE_Result res;
	char * flat_key;
	size_t flat_key_sz;
	uint32_t read_bytes;


	res = get_raw_object_size(key_id, key_id_sz, &flat_key_sz);
	if (res != TEE_SUCCESS) {
		TEE_Free(key_id);
		return res;
	}

	flat_key = TEE_Malloc(flat_key_sz, TEE_MALLOC_FILL_ZERO);
	res = read_raw_object(key_id, key_id_sz, flat_key, flat_key_sz, &read_bytes);

	if (res != TEE_SUCCESS) {
		TEE_Free(flat_key);
		return res;
	}

	DMSG("Private key read with size: %" PRIu32 "\n", read_bytes);

	*sk = dpabcSkFromBytes(flat_key);
	TEE_Free(flat_key);

	return res;
}


TEE_Result retreive_signature(char * sign_id, size_t sign_id_sz, signature ** sign) {

	TEE_Result res;
	char * flat_sign;
	size_t flat_sign_sz;
	uint32_t read_bytes;

	res = get_raw_object_size(sign_id, sign_id_sz, &flat_sign_sz);
	if (res != TEE_SUCCESS) {
		TEE_Free(sign_id);
		return res;
	}

	flat_sign = TEE_Malloc(flat_sign_sz, TEE_MALLOC_FILL_ZERO);
	res = read_raw_object(sign_id, sign_id_sz, flat_sign, flat_sign_sz, &read_bytes);

	if (res != TEE_SUCCESS) {
		TEE_Free(flat_sign);
		return res;
	}

	DMSG("Signature read with size: %" PRIu32 "\n", read_bytes);

	*sign = dpabcSignFromBytes(flat_sign);
	TEE_Free(flat_sign);

	return res;
}

static TEE_Result dpabc_generate_key(uint32_t param_types,
	TEE_Param params[4])
{

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	const size_t seed_sz = 40;

	TEE_Result res;
	char * key_id;
	size_t key_id_sz;
	int nattr;
	char * flat_key;
	uint32_t key_data_flag;
	char * seed_buffer;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	key_id_sz = params[0].memref.size;

	nattr = params[1].value.a;

	key_id = TEE_Malloc(key_id_sz, TEE_MALLOC_FILL_ZERO);
	if (!key_id) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(key_id, params[0].memref.buffer, key_id_sz);

	/*
	 * Generate Public and Private Keys
	 */
	publicKey * pk;
	secretKey * sk;

	seed_buffer = TEE_Malloc(seed_sz, TEE_MALLOC_FILL_ZERO);
	TEE_GenerateRandom(seed_buffer, seed_sz);
	seedRng(seed_buffer, seed_sz);
	changeNattr(nattr);
	uint8_t keygenBufferLen = 128;
        char * keygenBuffer = TEE_Malloc(keygenBufferLen, TEE_MALLOC_FILL_ZERO);
	TEE_GenerateRandom(keygenBuffer, keygenBufferLen);
	keyGen(&sk, &pk, keygenBuffer, keygenBufferLen);
	TEE_Free(seed_buffer);


	/*
	 * Writte keys to storage
	 */
	key_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */
			// TEE_DATA_FLAG_OVERWRITE;		/* destroy existing object of same ID */

	flat_key = TEE_Malloc(dpabcSkByteSize(sk), TEE_MALLOC_FILL_ZERO);
	dpabcSkToBytes(flat_key, sk);

	res = create_raw_object(key_data_flag, key_id, key_id_sz, flat_key, dpabcSkByteSize(sk));

	if (res != TEE_SUCCESS) {
		EMSG("Error storing private key");
	}
	else {
		DMSG("Private key written with size: %" PRIu32 "\n",  dpabcSkByteSize(sk));
	}

	dpabcSkFree(sk);
	dpabcPkFree(pk);
	TEE_Free(key_id);
	TEE_Free(flat_key);

	return res;
}

static TEE_Result dpabc_read_key(uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	TEE_Result res;
	char * key_id;
	size_t key_id_sz;

	publicKey * pk;
	secretKey * sk;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	key_id_sz = params[0].memref.size;
	key_id = TEE_Malloc(key_id_sz, 0);
	if (!key_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(key_id, params[0].memref.buffer, key_id_sz);

	res = retreive_secret_key(key_id, key_id_sz, &sk);


	if (res == TEE_SUCCESS) {
		pk = dpabcSkToPk(sk);
		if (dpabcPkByteSize(pk) > params[1].memref.size)
			res = TEE_ERROR_SHORT_BUFFER;
		else 
			dpabcPkToBytes(params[1].memref.buffer, pk);

		/* Return the number of byte effectively filled */
		params[1].memref.size = dpabcPkByteSize(pk);
		dpabcPkFree(pk);
	}

	TEE_Free(key_id);
	dpabcSkFree(sk);
	return res;

}

static TEE_Result dpabc_sign(uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT);

	TEE_Result res;
	char * key_id;
	size_t key_id_sz;
	char * attr;
	size_t attr_sz;
	char * epoch;
	size_t epoch_sz;

	secretKey * sk;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	key_id_sz = params[0].memref.size;
	key_id = TEE_Malloc(key_id_sz, 0);
	if (!key_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(key_id, params[0].memref.buffer, key_id_sz);

	attr_sz = params[1].memref.size;
	attr = TEE_Malloc(attr_sz, 0);
	if (!attr)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(attr, params[1].memref.buffer, attr_sz);

	epoch_sz = params[2].memref.size;
	epoch = TEE_Malloc(epoch_sz, 0);
	if (!epoch)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(epoch, params[2].memref.buffer, epoch_sz);

	res = retreive_secret_key(key_id, key_id_sz, &sk);

	if (sk->n != (attr_sz / zpByteSize())) {
		EMSG("Number of int attributes (%ld) missmatch with key attributes (%d)", 
			(attr_sz / zpByteSize()),
			sk->n
		);
		TEE_Free(key_id);
		TEE_Free(attr);
		dpabcSkFree(sk);
		return TEE_ERROR_BAD_PARAMETERS;
	}


	if (res == TEE_SUCCESS) {
		Zp * composedEpoch = zpFromBytes(epoch);
		Zp ** composedAttr = TEE_Malloc(sizeof(Zp*) * sk->n, TEE_MALLOC_FILL_ZERO);

		for (int i = 0; i < sk->n; i++)
			composedAttr[i] = zpFromBytes(attr + i*zpByteSize());

		signature *sig = sign(sk, composedEpoch, (const Zp **) composedAttr);

		if (!sig){
			EMSG("DPABC signature error\n");
			res = TEE_ERROR_GENERIC;
		}
		else {
			if (dpabcSignByteSize(sig) > params[3].memref.size)
				res = TEE_ERROR_SHORT_BUFFER;
			else 
				dpabcSignToBytes(params[3].memref.buffer, sig);
		}

		params[3].memref.size = dpabcSignByteSize(sig);

		zpFree(composedEpoch);
		for (size_t i = 0; i < sk->n; i++)
			zpFree(composedAttr[i]);
		dpabcSignFree(sig);
	}

	TEE_Free(key_id);
	TEE_Free(attr);
	TEE_Free(epoch);
	dpabcSkFree(sk);
	return res;

}


static TEE_Result dpabc_sign_store(uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT);

	TEE_Result res;
	char * key_id;
	size_t key_id_sz;
	char * sig_id;
	size_t sig_id_sz;
	char * flat_sig;
	size_t flat_sig_sz;
	char * attr;
	size_t attr_sz;
	char * epoch;
	size_t epoch_sz;
	uint32_t sig_data_flag;

	secretKey * sk;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	key_id_sz = params[0].memref.size;
	key_id = TEE_Malloc(key_id_sz, 0);
	if (!key_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(key_id, params[0].memref.buffer, key_id_sz);

	attr_sz = params[1].memref.size;
	attr = TEE_Malloc(attr_sz, 0);
	if (!attr)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(attr, params[1].memref.buffer, attr_sz);

	epoch_sz = params[2].memref.size;
	epoch = TEE_Malloc(epoch_sz, 0);
	if (!epoch)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(epoch, params[2].memref.buffer, epoch_sz);

	sig_id_sz = params[3].memref.size;
	sig_id = TEE_Malloc(sig_id_sz, 0);
	if (!sig_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(sig_id, params[3].memref.buffer, sig_id_sz);

	res = retreive_secret_key(key_id, key_id_sz, &sk);

	if (sk->n != (attr_sz / zpByteSize())) {
		EMSG("Number of int attributes (%ld) missmatch with key attributes (%d)", 
			(attr_sz / zpByteSize()),
			sk->n
		);
		TEE_Free(key_id);
		TEE_Free(attr);
		dpabcSkFree(sk);
		return TEE_ERROR_BAD_PARAMETERS;
	}


	if (res == TEE_SUCCESS) {
		Zp * composedEpoch = zpFromBytes(epoch);
		Zp ** composedAttr = TEE_Malloc(sizeof(Zp*) * sk->n, TEE_MALLOC_FILL_ZERO);

		for (int i = 0; i < sk->n; i++)
			composedAttr[i] = zpFromBytes(attr + i*zpByteSize());

		signature *sig = sign(sk, composedEpoch, (const Zp **) composedAttr);

		if (!sig){
			EMSG("DPABC signature error\n");
			res = TEE_ERROR_GENERIC;
		}
		else {

			sig_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
					TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
					TEE_DATA_FLAG_ACCESS_WRITE_META;	/* we can later destroy or rename the object */

			flat_sig_sz = dpabcSignByteSize();
			flat_sig = TEE_Malloc(flat_sig_sz, 0); 
			dpabcSignToBytes(flat_sig, sig);

			res = create_raw_object(sig_data_flag, sig_id, sig_id_sz, flat_sig, flat_sig_sz);

			if (res != TEE_SUCCESS) {
				EMSG("Could not store signature\n");
			}
			else {
				DMSG("Signature written with size: %" PRIu32 "\n",  flat_sig_sz);
			}
		}

		zpFree(composedEpoch);
		for (size_t i = 0; i < sk->n; i++)
			zpFree(composedAttr[i]);
		dpabcSignFree(sig);
	}

	TEE_Free(key_id);
	TEE_Free(attr);
	TEE_Free(epoch);
	dpabcSkFree(sk);
	return res;

}

static TEE_Result dpabc_zktoken(uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT);

	TEE_Result res;
	char * pk;
	size_t pk_sz;
	char * sign_id;
	size_t sign_id_sz;
	char * msg;
	size_t msg_sz;
	char * attr;
	size_t attr_sz;
	int * indexReveal;
	size_t indexReveal_sz;
	char * epoch;
	size_t epoch_sz;

	uint32_t attr_n;
	publicKey * composedPk;
	signature * composedSign;

	char * parameters;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	// Struct of shared memory "parameters" (params[0]): 
	//   +---------+-------------+------------+------------------+---------+--------------+----------+----------------+
	//   |  pk_sz  |     pk      | sign_id_sz |     sign_id      | msg_sz  |     msg      | eopch_sz |     epoch      |
	//   +---------+-------------+------------+------------------+---------+--------------+----------+----------------+
	//   | 4 bytes | pk_sz bytes | 4 bytes    | sign_id_sz bytes | 4 bytes | msg_sz bytes | 4 bytes  | epoch_sz bytes |
	//   +---------+-------------+------------+------------------+---------+--------------+----------+----------------+

	parameters = (char *) params[0].memref.buffer;


	// pk_sz = (uint32_t) parameters[0];
	TEE_MemMove(&pk_sz, parameters, sizeof(uint32_t));
	pk = TEE_Malloc(pk_sz, 0);
	if (!pk)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(pk, parameters + sizeof(uint32_t), pk_sz);


	sign_id_sz = (uint32_t) parameters[sizeof(uint32_t) + pk_sz];
	sign_id = TEE_Malloc(sign_id_sz, 0);
	if (!sign_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(sign_id, parameters + 2 * sizeof(uint32_t) + pk_sz, sign_id_sz);

	msg_sz = (uint32_t) parameters[2 * sizeof(uint32_t) + pk_sz + sign_id_sz];
	msg = TEE_Malloc(msg_sz, 0);
	if (!msg)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(msg, parameters + 3 * sizeof(uint32_t) + pk_sz + sign_id_sz, msg_sz);


	epoch_sz = (uint32_t) parameters[3 * sizeof(uint32_t) + pk_sz + sign_id_sz + msg_sz];
	epoch = TEE_Malloc(epoch_sz, 0);
	if (!epoch)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(epoch, parameters + 4 * sizeof(uint32_t) + pk_sz + sign_id_sz + msg_sz, epoch_sz);


	attr_sz = params[1].memref.size;
	attr = TEE_Malloc(attr_sz, 0);
	if (!attr)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(attr, params[1].memref.buffer, attr_sz);

	attr_n = attr_sz / zpByteSize();


	indexReveal_sz = params[2].memref.size;
	indexReveal = TEE_Malloc(indexReveal_sz, 0);
	if (!indexReveal)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(indexReveal, params[2].memref.buffer, indexReveal_sz);

	res = retreive_signature(sign_id, sign_id_sz, &composedSign);

	if (res == TEE_SUCCESS) {

		composedPk = dpabcPkFromBytes(pk);

		Zp * composedEpoch = zpFromBytes(epoch);
		Zp ** composedAttr = TEE_Malloc(sizeof(Zp*) * attr_n, TEE_MALLOC_FILL_ZERO);

		for (int i = 0; i < attr_n; i++)
			composedAttr[i] = zpFromBytes(attr + i*zpByteSize());

                uint8_t tokenBufferLen = 128;
    	        char *tokenBuffer = TEE_Malloc(tokenBufferLen, TEE_MALLOC_FILL_ZERO);
		TEE_GenerateRandom(tokenBuffer, tokenBufferLen);
				zkToken * token = presentZkToken(composedPk, composedSign, composedEpoch, composedAttr, indexReveal, indexReveal_sz / sizeof(int), msg, msg_sz, tokenBuffer, tokenBufferLen);

		if (!token){
			EMSG("DPABC zkToken error\n");
			res = TEE_ERROR_GENERIC;
		}
		else {
			if (dpabcZkByteSize(token) > params[3].memref.size)
				res = TEE_ERROR_SHORT_BUFFER;
				else 
				dpabcZkToBytes(params[3].memref.buffer, token);
		}

		params[3].memref.size = dpabcZkByteSize(token);

		zpFree(composedEpoch);
		for (size_t i = 0; i < attr_n; i++)
			zpFree(composedAttr[i]);
		dpabcSignFree(composedSign);
		dpabcPkFree(composedPk);
		dpabcZkFree(token);
	}

	TEE_Free(pk);
	TEE_Free(msg);
	TEE_Free(sign_id);
	TEE_Free(attr);
	TEE_Free(indexReveal);
	TEE_Free(epoch);
	return res;

}

static TEE_Result dpabc_store_sign(uint32_t param_types,
	TEE_Param params[4])
{

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	TEE_Result res;
	char * sign_id;
	size_t sign_id_sz;
	char * flat_sign;
	size_t flat_sign_sz;
	uint32_t sign_data_flag;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	sign_id_sz = params[0].memref.size;
	sign_id = TEE_Malloc(sign_id_sz, TEE_MALLOC_FILL_ZERO);
	if (!sign_id) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(sign_id, params[0].memref.buffer, sign_id_sz);

	flat_sign_sz = params[1].memref.size;
	flat_sign = TEE_Malloc(flat_sign_sz, TEE_MALLOC_FILL_ZERO);
	if (!flat_sign) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(flat_sign, params[1].memref.buffer, flat_sign_sz);

	/*
	 * Writte keys to storage
	 */
	sign_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			TEE_DATA_FLAG_ACCESS_WRITE_META |	/* we can later destroy or rename the object */
			TEE_DATA_FLAG_OVERWRITE;

	res = create_raw_object(sign_data_flag, sign_id, sign_id_sz, flat_sign, flat_sign_sz);
	if (res != TEE_SUCCESS) {
		EMSG("Could not store signature\n");
	}
	else {
		DMSG("Signature written with size: %" PRIu32 "\n",  flat_sign_sz);
	}

	TEE_Free(sign_id);
	TEE_Free(flat_sign);

	return res;
}


static TEE_Result dpabc_verify_stored(uint32_t param_types,
	TEE_Param params[4])
{

	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT);

	TEE_Result res;
	char * pk;
	size_t pk_sz;
	char * sig_id;
	size_t sig_id_sz;
	char * attr;
	size_t attr_sz;
	char * epoch;
	size_t epoch_sz;

	publicKey * public_key;
	char * sig_bytes;
	signature * sig;

	res = TEE_SUCCESS;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	pk_sz = params[0].memref.size;
	pk = TEE_Malloc(pk_sz, 0);
	if (!pk)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(pk, params[0].memref.buffer, pk_sz);

	attr_sz = params[1].memref.size;
	attr = TEE_Malloc(attr_sz, 0);
	if (!attr)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(attr, params[1].memref.buffer, attr_sz);

	epoch_sz = params[2].memref.size;
	epoch = TEE_Malloc(epoch_sz, 0);
	if (!epoch)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(epoch, params[2].memref.buffer, epoch_sz);

	sig_id_sz = params[3].memref.size;
	sig_id = TEE_Malloc(sig_id_sz, 0);
	if (!sig_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(sig_id, params[3].memref.buffer, sig_id_sz);

	public_key = dpabcPkFromBytes(pk);

	if (public_key->n != (attr_sz / zpByteSize())) {
		EMSG("Number of int attributes (%ld) missmatch with key attributes (%d)", 
			(attr_sz / zpByteSize()),
			public_key->n
		);

		res = TEE_ERROR_BAD_PARAMETERS;
	}


	if (res == TEE_SUCCESS) {
		Zp * composedEpoch = zpFromBytes(epoch);
		Zp ** composedAttr = TEE_Malloc(sizeof(Zp*) * public_key->n, TEE_MALLOC_FILL_ZERO);

		for (int i = 0; i < public_key->n; i++)
			composedAttr[i] = zpFromBytes(attr + i*zpByteSize());

		res = retreive_signature(sig_id, sig_id_sz, &sig);

		if (res == TEE_SUCCESS) {
			if (!verify(public_key, sig, composedEpoch, (const Zp **)composedAttr)) {
				res = TEE_ERROR_SIGNATURE_INVALID;
			}
			else {
				res = TEE_SUCCESS;
			}
			dpabcSignFree(sig);
		}

		zpFree(composedEpoch);
		for (size_t i = 0; i < public_key->n; i++)
			zpFree(composedAttr[i]);
	}

	TEE_Free(pk);
	TEE_Free(attr);
	TEE_Free(epoch);
	TEE_Free(sig_id);
	dpabcPkFree(public_key);
	return res;

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
	(void)&sess_ctx; /* Unused parameter */

	//dpabcInit("SEEDRNG", 7); //TODO initialize this someware else

	switch (cmd_id) {
		case TA_DPABC_GENERATE_KEY:
			return dpabc_generate_key(param_types, params);
		case TA_DPABC_READ_KEY:
			return dpabc_read_key(param_types,params);
		case TA_DPABC_SIGN:
			return dpabc_sign(param_types, params);
		case TA_DPABC_ZKTOKEN:
			return dpabc_zktoken(param_types, params);
		case TA_DPABC_STORE_SIGN:
			return dpabc_store_sign(param_types, params);
		case TA_DPABC_SIGN_STORE:
			return dpabc_sign_store(param_types, params);
		case TA_DPABC_VERIFY_STORED:
			return dpabc_verify_stored(param_types, params);
		default:
			return TEE_ERROR_BAD_PARAMETERS;
	}
}
