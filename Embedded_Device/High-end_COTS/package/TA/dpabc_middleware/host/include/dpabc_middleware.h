#include <tee_client_api.h>
/* For the UUID (found in the TA's h-file(s)) */
#include <dpabc_ta.h>

#include <core.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <link.h>

#define HASH_SIZE	32

#define STATUS_GENERIC_ERROR 		0
#define STATUS_INITIALIZATION_ERROR	2
#define STATUS_KEY_GENERATION_ERROR	3
#define STATUS_KEY_READ_ERROR 		4
#define STATUS_UNIMPLEMENTED		5
#define STATUS_DUPLICATE_KEY		6
#define STATUS_WRITE_ERROR		7
#define STATUS_VERIFICATION_ERROR	8		
#define STATUS_BAD_PARAMETERS		9		
#define STATUS_DUPLICATE_SIGNATURE	10				

#define STATUS_OK 1

#define DEFAULT_BUFFER_SIZE 512

typedef uint32_t DPABC_status;

typedef struct {
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_UUID uuid;
} DPABC_session;

/**
 * @brief Initialize trusted application and get session information
 * the session param which needs to be allocated previously
 * 
 * @param session session information that needs to be provided
 * for every call to the TA
*/
DPABC_status DPABC_initialize(DPABC_session * session);

/**
 * @brief Generates private key and stores it in secure storage
 * 
 * @param session contains session information that needs to be provided for every call to the TA @param key_id Key id needed to reference the private key
 * @param nattr Number of attributes of the key scheme
*/
DPABC_status DPABC_generate_key(DPABC_session * session, char * key_id, int nattr);

/**
 * @brief Generates public key associated with the private key id provided
 * 
 * @param session contains session information that needs to be provided
 * for every call to the TA
 * @param key_id Key id needed to reference the private key
 * @param pk Private key reference where the public key will be placed,
 * memory will be allocated for it, after call, after use must be freed
 * @param pk_sz Size reference where the public key size in bytes will be
 * reference, can be set to NULL if not needed
*/
DPABC_status DPABC_get_key(DPABC_session * session, char * key_id, char ** pk, size_t * pk_sz);

/**
 * @brief Signs the provided attributes with the private key id provided
 * 
 * @param session contains session information that needs to be provided
 * for every call to the TA
 * @param key_id Key id needed to reference the private key
 * @param epoch in byte form
 * @param epoch_sz size of epoch in bytes
 * @param attr Array of ints of attributes to sign
 * @param attr_sz size of attribute array in bytes
 * @param sig Signature reference where the generater signature will be placed,
 * memory will be allocated for it, after call, after use must be freed
 * @param sig_sz Size reference where the signature size in bytes will be
 * reference, can be set to NULL if not needed
*/
DPABC_status DPABC_sign(DPABC_session * session, char * key_id, char * epoch, size_t epoch_sz, char * attr, size_t attr_sz, char ** sig, size_t * sig_sz);

/**
 * @brief Signs the provided attributes with the private key id provided and stores the signature generated
 * 
 * @param session contains session information that needs to be provided
 * for every call to the TA
 * @param key_id Key id needed to reference the private key
 * @param epoch in byte form
 * @param epoch_sz size of epoch in bytes
 * @param attr Array of ints of attributes to sign
 * @param attr_sz size of attribute array in bytes
 * @param sig_id Signature identifier string
*/
DPABC_status DPABC_signStore(DPABC_session * session, char * key_id, char * epoch, size_t epoch_sz, char * attr, size_t attr_sz, char * sig_id);

/**
 * @brief Verifies a stored signature against a set of attributes 
 * 
 * @param session contains session information that needs to be provided
 * for every call to the TA
 * @param pk signature's public key in bytes
 * @param pk_sz size in bytes of the public key
 * @param epoch in byte form
 * @param epoch_sz size of epoch in bytes
 * @param attr Array of ints of attributes to sign
 * @param attr_sz size of attribute array in bytes
 * @param sig_id Signature identifier string
*/
DPABC_status DPABC_verifyStored(DPABC_session * session, char * pk, size_t pk_sz, char * epoch, size_t epoch_sz, char * attr, size_t attr_sz, char * sig_id);

/**
 * @brief stores DPABC signature in secure storage
 * 
 * @param session contains session information that needs to be provided
 * for every call to the TA
 * @param sign_id id needed to reference the Signature
 * @param signatureBytes Array containing the signature to be stored
 * @param signatureBytes_sz Size of the array containing the signature to be stored
*/
DPABC_status DPABC_storeSignature(DPABC_session * session, char * sign_id, char * signatureBytes, size_t signatureBytes_sz);

/**
 * @brief Generates a zero knowledge token from a previously stored signature 
 * 
 * @param session contains session information that needs to be provided
 * for every call to the TA
 * @param pk Array with public key in byte form corresponding to the signature
 * @param pk_sz Size of pk
 * @param sign_id String id of previously storedsignature for which we are computing a zero knowledge proof
 * @param epoch Array with epoch in byte form
 * @param epoch_sz Size of epoch
 * @param attr Array of signed attributes in byte form
 * @param attr_sz Size of attributes array
 * @param indexReval indexes of the revealed attributes  (with respect to the whole set of attributes, starting from 0). Assumed to be in ascendent order
 * @param indexReval_sz Size of indexes array
 * @param msg Message that will be signed for generating the zero-knowldege token
 * @param msg_sz Size of the message to be signed
 * @param zkToken zero knowledge token reference where the token will be placed, memory will be allocated for it, after call, after use must be freed
 * @param zkToken_sz Size reference where the token size in bytes will be reference, can be set to NULL if not needed
*/
DPABC_status DPABC_generateZKtoken(DPABC_session * session, 
				   char * pk, size_t pk_sz, 
				   char * sign_id,
				   char * epoch, size_t epoch_sz, 
				   char * attr, size_t attr_sz, 
				   int * indexReveal, size_t indexReveal_sz, 
				   char * msg, size_t msg_sz,
				   char ** zkToken, size_t * zkToken_sz
);

DPABC_status DPABC_combineSignatures(DPABC_session * session, char * combined_id, char ** pks, uint32_t * pks_sz, char ** sig_ids, int nelements);
/**
 * @brief Finalize TA session
 * 
 * @param session contains session information, after
 * calling finalize must be freed
*/
DPABC_status DPABC_finalize(DPABC_session * session);
