// #include <stdlib.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#define AES128_BLOCK_SIZE		16
#define AES128_KEY_BYTE_SIZE		16
#define AES128_KEY_BIT_SIZE		128

#define AES256_BLOCK_SIZE		16
#define AES256_KEY_BYTE_SIZE		32
#define AES256_KEY_BIT_SIZE		256

typedef struct {
	TEE_OperationHandle op_handle;
	TEE_ObjectHandle key_handle; 
} aes_session;

void DBG_print_block(char * block);

void DBG_print_extended_block(char * block, size_t block_sz);

TEE_Result AES_128_init(char * key, aes_session * session); 

TEE_Result AES_256_init(char * key, aes_session * session);

TEE_Result AES_decode_256_init(char * key, aes_session * session);

TEE_Result SHA_256_init(aes_session * session);

TEE_Result AES_CMAC_128_init(char * key, aes_session * session);

TEE_Result AES_HMAC_MD5_init(char * key, aes_session * session);

TEE_Result AES_128_terminate(aes_session * sess); 

TEE_Result AES_256_terminate(aes_session * sess); 

TEE_Result AES_CMAC_128_terminate(aes_session * sess); 

TEE_Result AES_HMAC_MD5_terminate(aes_session * sess); 

TEE_Result SHA_256_terminate(aes_session * sess);

TEE_Result AES_128_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz); 

TEE_Result AES_256_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz); 

TEE_Result AES_CMAC_128_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz);

TEE_Result AES_HMAC_MD5_cipher(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz);

TEE_Result AES_HMAC_MD5_verify(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * mac, size_t macLen);

TEE_Result SHA_256_digest(TEE_OperationHandle op_handle, char * src, size_t src_sz, char * dst, size_t * dst_sz);

TEE_Result create_write(uint32_t obj_data_flag, char * obj_id, size_t obj_id_sz, char * data, size_t data_sz); 

TEE_Result read_raw_object(char * obj_id, size_t obj_id_sz, char * data, size_t data_sz, size_t * read_bytes, uint32_t flags); 

TEE_Result remove_object(uint32_t flags, char * obj_id, size_t obj_id_sz);
