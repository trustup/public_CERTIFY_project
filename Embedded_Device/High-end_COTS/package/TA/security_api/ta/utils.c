#include <utils.h>

void DBG_print_block(char * block) {
	printf("0x");
	for (uint8_t i = 0; i < 16; i++) printf("%02x ", block[i]);
	printf("\n");
}


void DBG_print_extended_block(char * block, size_t block_sz) {
	printf("0x");
	for (size_t i = 0; i < block_sz; i++) printf("%02x ", block[i]);
	printf("\n");
}

TEE_Result SHA_256_init(aes_session *session) {
	
	TEE_Result res;

	uint32_t key_size = AES128_KEY_BIT_SIZE;		/* AES key size in byte */
	uint32_t algo = TEE_ALG_SHA256;			/* AES flavour */	
	uint32_t mode = TEE_MODE_DIGEST;			/* Encode */

	res = TEE_AllocateOperation(&(session->op_handle),
				    algo,
				    mode,
				    0);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		session->op_handle = TEE_HANDLE_NULL;
		return res;
	}

	return TEE_SUCCESS;

}

static TEE_Result AES_init(char * key, aes_session * session, uint32_t key_size, uint32_t algo, char requires_iv, size_t mode) {

	TEE_Attribute attr;
	TEE_Result res;

	unsigned char iv[AES256_BLOCK_SIZE] = { 0 };

	res = TEE_AllocateOperation(&(session->op_handle),
				    algo,
				    mode,
				    key_size);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		session->op_handle = TEE_HANDLE_NULL;
		goto err;
	}

	res = TEE_AllocateTransientObject(TEE_TYPE_AES,
					  key_size,
					  &(session->key_handle));
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate transient object");
		session->key_handle = TEE_HANDLE_NULL;
		goto err;
	}

	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key, key_size / 8);

	res = TEE_PopulateTransientObject(session->key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, %x", res);
		goto err;
	}

	res = TEE_SetOperationKey(session->op_handle, session->key_handle);

	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey failed %x", res);
		goto err;
	}
	
	TEE_CipherInit(session->op_handle, requires_iv ? iv : NULL, requires_iv ? AES256_BLOCK_SIZE : 0); //IV not used for aes-ecb
	
	return TEE_SUCCESS;

err:
	if (session->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(session->op_handle);
	session->op_handle = TEE_HANDLE_NULL;

	if (session->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(session->key_handle);
	session->key_handle = TEE_HANDLE_NULL;

	return res;
}

TEE_Result AES_128_init(char * key, aes_session * session) {

	uint32_t key_size = AES128_KEY_BIT_SIZE;		/* AES key size in byte */
	uint32_t algo = TEE_ALG_AES_ECB_NOPAD;			/* AES flavour */	
	uint32_t mode = TEE_MODE_ENCRYPT;			/* Encode */
	return AES_init(key, session, key_size, algo, 0, mode);
}

TEE_Result AES_256_init(char * key, aes_session * session) {

	uint32_t key_size = AES256_KEY_BIT_SIZE;		/* AES key size in byte */
	uint32_t algo = TEE_ALG_AES_CBC_NOPAD;			/* AES flavour */	
	uint32_t mode = TEE_MODE_ENCRYPT;			/* Encode */
	return AES_init(key, session, key_size, algo, 1, mode);
}

TEE_Result AES_decode_256_init(char * key, aes_session * session) {

	uint32_t key_size = AES256_KEY_BIT_SIZE;		/* AES key size in byte */
	uint32_t algo = TEE_ALG_AES_CBC_NOPAD;			/* AES flavour */	
	uint32_t mode = TEE_MODE_DECRYPT;			/* Decode */
	return AES_init(key, session, key_size, algo, 1, mode);
}

TEE_Result AES_MAC_init(char * key, aes_session * session, uint32_t algo, uint32_t mode, size_t key_size, uint32_t object_type) {

	TEE_Attribute attr;
	TEE_Result res;

	IMSG("Allocate Operation");

	res = TEE_AllocateOperation(&(session->op_handle),
				    algo,
				    mode,
				    key_size);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		session->op_handle = TEE_HANDLE_NULL;
		goto err;
	}

	IMSG("Allocate Object");
	res = TEE_AllocateTransientObject(object_type,
					  key_size,
					  &(session->key_handle));
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate transient object");
		session->key_handle = TEE_HANDLE_NULL;
		goto err;
	}

	IMSG("Init attribute");
	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key, key_size / 8);

	IMSG("Populate");
	res = TEE_PopulateTransientObject(session->key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, %x", res);
		goto err;
	}

	IMSG("Set_key");
	res = TEE_SetOperationKey(session->op_handle, session->key_handle);

	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey failed %x", res);
		goto err;
	}
	
	IMSG("Mac init");
	TEE_MACInit(session->op_handle, NULL, 0); //IV not used for aes-ecb
	return TEE_SUCCESS;

err:
	if (session->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(session->op_handle);
	session->op_handle = TEE_HANDLE_NULL;

	if (session->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(session->key_handle);
	session->key_handle = TEE_HANDLE_NULL;

	return res;
}

TEE_Result AES_CMAC_128_init(char * key, aes_session * session) {

	uint32_t algo = TEE_ALG_AES_CMAC;			/* CMAC */	
	uint32_t mode = TEE_MODE_MAC;				/* MAC */
	uint32_t key_size = AES128_KEY_BIT_SIZE;		/* AES key size in byte */
	uint32_t object_type = TEE_TYPE_AES;			/* AES key type */

	return AES_MAC_init(key, session, algo, mode, key_size, object_type);
}

TEE_Result AES_HMAC_MD5_init(char * key, aes_session * session) {

	uint32_t algo = TEE_ALG_HMAC_MD5;			/* HMAC */	
	uint32_t mode = TEE_MODE_MAC;				/* MAC */
	uint32_t key_size = AES128_KEY_BIT_SIZE;		/* AES key size in byte */
	uint32_t object_type = TEE_TYPE_HMAC_MD5;		/* HMAC key type */ 

	return AES_MAC_init(key, session, algo, mode, key_size, object_type);
}

TEE_Result AES_terminate(aes_session * sess) {

	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);

	return TEE_SUCCESS;
}

TEE_Result AES_128_terminate(aes_session * sess) { return AES_terminate(sess); }

TEE_Result AES_256_terminate(aes_session * sess) { return AES_terminate(sess); }

TEE_Result AES_CMAC_128_terminate(aes_session * sess) {
	return AES_terminate(sess);
}

TEE_Result AES_HMAC_MD5_terminate(aes_session * sess) {
	return AES_terminate(sess);
}

TEE_Result SHA_256_terminate(aes_session * sess) {
	
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);

	return TEE_SUCCESS;
}

TEE_Result AES_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) {

	return TEE_CipherUpdate(op_handle,
				src, src_sz, 
				dst, dst_sz);
}

TEE_Result AES_128_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) { return AES_cipher(op_handle, src, src_sz, dst, dst_sz); }

TEE_Result AES_256_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) { return AES_cipher(op_handle, src, src_sz, dst, dst_sz); }

TEE_Result AES_MAC_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) {

	return TEE_MACComputeFinal(op_handle,
				   src, src_sz, 
				   dst, dst_sz);
}

TEE_Result AES_CMAC_128_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) { return AES_MAC_cipher(op_handle, src, src_sz, dst, dst_sz); }

TEE_Result AES_HMAC_MD5_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) { return AES_MAC_cipher(op_handle, src, src_sz, dst, dst_sz); }

TEE_Result AES_MAC_verify(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * mac, size_t macLen) {

	return TEE_MACCompareFinal(op_handle,
				   src, src_sz, 
				   mac, macLen);
}

TEE_Result AES_CMAC_128_verify(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * mac, size_t macLen) { return AES_MAC_verify(op_handle, src, src_sz, mac, macLen); }

TEE_Result AES_HMAC_MD5_verify(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * mac, size_t macLen) { return AES_MAC_verify(op_handle, src, src_sz, mac, macLen); }
		
TEE_Result SHA_256_digest(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz) {

	return TEE_DigestDoFinal(op_handle,
				 src, src_sz,
				 dst, dst_sz);
}

TEE_Result create_write(uint32_t obj_data_flag, char * obj_id, size_t obj_id_sz, char * data, size_t data_sz) {

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

	IMSG("Stored %.*s with size %ld", obj_id_sz, obj_id, data_sz);
	printf("0x");
	for (uint32_t i = 0; i < data_sz; i++) printf("%02x ", data[i]);
	printf("\n");

	return TEE_SUCCESS;
}

TEE_Result remove_object(uint32_t flags, char * obj_id, size_t obj_id_sz) {

	TEE_ObjectHandle object;
	TEE_Result res;

	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					flags,
					&object);

	if (res != TEE_SUCCESS) {
		EMSG("TEE_OpenPersistentObject (%.*s) failed 0x%08x", obj_id_sz, obj_id, res);
		return res;
	}

	res = TEE_CloseAndDeletePersistentObject1(object);

	if (res != TEE_SUCCESS) {
		EMSG("TEE_DeleteObject failed 0x%08x", res);
	} 

	IMSG("Deleted %.*s", obj_id_sz, obj_id);

	return TEE_SUCCESS;
}


TEE_Result read_raw_object(char * obj_id, size_t obj_id_sz, char * data, size_t data_sz, size_t * read_bytes, uint32_t flags) {

	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;

	/*
	 * Check the object exist and can be dumped into output buffer
	 * then dump it.
	 */
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					flags,
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
		EMSG("TEE_ReadObjectData failed 0x%08x, read %ls" PRIu32 " over %u",
				res, read_bytes, object_info.dataSize);
		TEE_CloseObject(object);
		return res;
	}

	IMSG("Retreived %.*s with size %ld", obj_id_sz, obj_id, *read_bytes);
	printf("0x");
	for (uint32_t i = 0; i < *read_bytes; i++) printf("%02x ", data[i]);
	printf("\n");

	TEE_CloseObject(object);
	return res;
}
