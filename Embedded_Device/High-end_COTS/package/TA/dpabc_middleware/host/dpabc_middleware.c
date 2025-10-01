
#include <dpabc_middleware.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tee_client_api.h>


int ld_callback(struct dl_phdr_info * info, size_t size, void * data) {
	hash256 * state = (hash256 *)data;
	for (int header_id = 0; header_id < info->dlpi_phnum; header_id++) {
		const ElfW(Phdr) *hdr = &info->dlpi_phdr[header_id];
		// Skip over headers that can not include read-only data that
		// possibly is also executable code:
		if (hdr->p_memsz == 0) {
			continue;
		}
		if ((hdr->p_flags & (PF_R | PF_X)) != (PF_R | PF_X)) {
			continue;
		}
		if (hdr->p_flags & PF_W) {
			continue;
		}

		char * bin_start = info->dlpi_addr + hdr->p_vaddr;

		for (int i = 0; i < hdr->p_memsz; i++) {
			HASH256_process(state, bin_start[i]);
		}
	}
	return 0;
}


char * auto_hash() {

	hash256 state; 
	HASH256_init(&state);

	char * res = malloc(HASH_SIZE);

	dl_iterate_phdr(ld_callback, &state);
	HASH256_hash(&state, res);


	return res;
}


DPABC_status DPABC_initialize(DPABC_session * session) {

	uint32_t err_origin;
	TEEC_Operation op;
	TEEC_Result res;

	TEEC_UUID session_uuid = TA_DPABC_UUID;
	session->uuid = session_uuid;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &(session->ctx));
	if (res != TEEC_SUCCESS)
		return STATUS_INITIALIZATION_ERROR;

	/*
	 * Open a session to the "hello world" TA, the TA will print "hello
	 * world!" in the log when the session is created.
	 */

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE,
					 TEEC_NONE, 
					 TEEC_NONE);

	char * self_check = auto_hash();

	op.params[0].tmpref.buffer = self_check;
	op.params[0].tmpref.size = HASH_SIZE;

	res = TEEC_OpenSession(&(session->ctx), &(session->sess), &(session->uuid),
			       TEEC_LOGIN_PUBLIC, NULL, &op, &err_origin);
	free(self_check);
	if (res != TEEC_SUCCESS)
		return STATUS_INITIALIZATION_ERROR;

	return STATUS_OK;
}

DPABC_status DPABC_generate_key(DPABC_session * session, char * key_id, int nattr) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;


	memset(&op, 0, sizeof(op));

	size_t key_id_sz = strlen(key_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * the remaining three parameters are unused.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, 
					 TEEC_VALUE_INPUT,
					 TEEC_NONE, 
					 TEEC_NONE);


	op.params[0].tmpref.buffer = key_id;
	op.params[0].tmpref.size = key_id_sz;

	op.params[1].value.a = nattr;


	printf("Invoking TA to generate key\n");
	res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_GENERATE_KEY, &op,
				 &err_origin);

	if (res == TEEC_ERROR_ACCESS_CONFLICT) {
		printf("Command TA_DPABC_GENERATE_KEY failed: 0x%x / %u\n", res, err_origin);
		return STATUS_DUPLICATE_KEY;
	}

	if (res != TEEC_SUCCESS) {
		printf("Command TA_DPABC_GENERATE_KEY failed: 0x%x / %u\n", res, err_origin);
		return STATUS_KEY_GENERATION_ERROR;
	}
	printf("TA generated key\n");


	return STATUS_OK;
}

DPABC_status DPABC_get_key(DPABC_session * session, char * key_id, char ** pk, size_t * pk_sz) {
	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	char pk_buffer[DEFAULT_BUFFER_SIZE];
	bool pk_initialized = false;

	memset(&op, 0, sizeof(op));

	size_t key_id_sz = strlen(key_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);


	op.params[0].tmpref.buffer = key_id;
	op.params[0].tmpref.size = key_id_sz;

	op.params[1].tmpref.buffer = pk_buffer;
	op.params[1].tmpref.size = DEFAULT_BUFFER_SIZE;

	do {
		printf("Invoking TA to retrieve key\n");
		res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_READ_KEY, &op,
					 &err_origin);
		if (res != TEEC_SUCCESS && res != TEEC_ERROR_SHORT_BUFFER) {
			printf("Command TA_DPABC_READ_KEY failed: 0x%x / %u\n", res, err_origin);
			return STATUS_KEY_READ_ERROR;
		}

		if (res == TEEC_ERROR_SHORT_BUFFER) {
			*pk = malloc(op.params[1].tmpref.size);
			pk_initialized = true;
			op.params[1].tmpref.buffer = *pk;
		}
	} while (res != TEEC_SUCCESS);

	printf("TA key retrieved\n");
	printf("Retrieved key size: %ld\n", op.params[1].tmpref.size);

	if (!pk_initialized) {
		*pk = malloc(op.params[1].tmpref.size);
		memcpy(*pk, pk_buffer, op.params[1].tmpref.size);
	}

	if (pk_sz)
		*pk_sz = op.params[1].tmpref.size;

	return STATUS_OK;
}



DPABC_status DPABC_sign(DPABC_session * session, char * key_id, char * epoch, size_t epoch_sz, char * attr, size_t attr_sz, char ** sig, size_t * sig_sz) {
	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;
	char sig_buffer[DEFAULT_BUFFER_SIZE];
	bool sig_initialized = false;

	memset(&op, 0, sizeof(op));

	size_t key_id_sz = strlen(key_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT);


	op.params[0].tmpref.buffer = key_id;
	op.params[0].tmpref.size = key_id_sz;

	op.params[1].tmpref.buffer = attr;
	op.params[1].tmpref.size = attr_sz;

	op.params[2].tmpref.buffer = epoch;
	op.params[2].tmpref.size = epoch_sz;

	op.params[3].tmpref.buffer = sig_buffer;
	op.params[3].tmpref.size = DEFAULT_BUFFER_SIZE;

	do {
		printf("Invoking TA to sign\n");
		res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_SIGN, &op,
					 &err_origin);
		if (res != TEEC_SUCCESS && res != TEEC_ERROR_SHORT_BUFFER) {
			printf("Command TA_DPABC_SIGN failed: 0x%x / %u\n", res, err_origin);
			return STATUS_KEY_READ_ERROR;
		}

		if (res == TEEC_ERROR_SHORT_BUFFER) {
			*sig = malloc(op.params[3].tmpref.size);
			sig_initialized = true;
			op.params[3].tmpref.buffer = *sig;
		}
	} while (res != TEEC_SUCCESS);

	printf("TA signed\n");
	printf("Generated signature size: %ld\n", op.params[3].tmpref.size);

	if (!sig_initialized) {
		*sig = malloc(op.params[3].tmpref.size);
		memcpy(*sig, sig_buffer, op.params[3].tmpref.size);
	}

	if (sig_sz)
		*sig_sz = op.params[3].tmpref.size;

	return STATUS_OK;
}


DPABC_status DPABC_signStore(DPABC_session * session, char * key_id, char * epoch, size_t epoch_sz, char * attr, size_t attr_sz, char * sig_id) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));

	size_t key_id_sz = strlen(key_id);
	size_t sig_id_sz = strlen(sig_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT);


	op.params[0].tmpref.buffer = key_id;
	op.params[0].tmpref.size = key_id_sz;

	op.params[1].tmpref.buffer = attr;
	op.params[1].tmpref.size = attr_sz;

	op.params[2].tmpref.buffer = epoch;
	op.params[2].tmpref.size = epoch_sz;

	op.params[3].tmpref.buffer = sig_id;
	op.params[3].tmpref.size = sig_id_sz;


	printf("Invoking TA to sign\n");
	res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_SIGN_STORE, &op,
	&err_origin);

	if (res == TEEC_ERROR_ACCESS_CONFLICT) {
		printf("Command TA_DPABC_SIGN_STORE failed: 0x%x / %u\n", res, err_origin);
		return STATUS_DUPLICATE_SIGNATURE;
	}

	if (res != TEEC_SUCCESS) {
		printf("Command TA_DPABC_SIGN_STORE failed: 0x%x / %u\n", res, err_origin);
		return STATUS_WRITE_ERROR;
	}

	printf("TA signed\n");
	printf("Generated signature with id: %s\n", sig_id);

	return STATUS_OK;
}


DPABC_status DPABC_verifyStored(DPABC_session * session, char * pk, size_t pk_sz, char * epoch, size_t epoch_sz, char * attr, size_t attr_sz, char * sig_id) {
	
	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));

	size_t sig_id_sz = strlen(sig_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT);


	op.params[0].tmpref.buffer = pk;
	op.params[0].tmpref.size = pk_sz;

	op.params[1].tmpref.buffer = attr;
	op.params[1].tmpref.size = attr_sz;

	op.params[2].tmpref.buffer = epoch;
	op.params[2].tmpref.size = epoch_sz;

	op.params[3].tmpref.buffer = sig_id;
	op.params[3].tmpref.size = sig_id_sz;


	printf("Invoking TA to verify stored signature\n");
	res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_VERIFY_STORED, &op,
	&err_origin);
	if (res == TEEC_ERROR_BAD_PARAMETERS) {
		printf("Command TA_DPABC_VERIFY_STORED failed (bad parameters): 0x%x / %u\n", res, err_origin);
		return STATUS_BAD_PARAMETERS;
	}
	else if (res != TEEC_SUCCESS) {
		printf("Command TA_DPABC_VERIFY_STORED failed: 0x%x / %u\n", res, err_origin);
		return STATUS_VERIFICATION_ERROR;
	}

	printf("TA verified\n");

	return STATUS_OK;
}


DPABC_status DPABC_storeSignature(DPABC_session * session, char * sign_id, char * signatureBytes, size_t signatureBytes_sz) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));

	size_t sign_id_sz = strlen(sign_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE,
					 TEEC_NONE);


	op.params[0].tmpref.buffer = sign_id;
	op.params[0].tmpref.size = sign_id_sz;

	op.params[1].tmpref.buffer = signatureBytes;
	op.params[1].tmpref.size = signatureBytes_sz;

	printf("Invoking TA to store signature\n");
	res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_STORE_SIGN, &op,
	&err_origin);

	if (res == TEEC_ERROR_ACCESS_CONFLICT) {
		printf("Command TA_DPABC_GENERATE_KEY failed: 0x%x / %u\n", res, err_origin);
		return STATUS_DUPLICATE_SIGNATURE;
	}

	if (res != TEEC_SUCCESS) {
		printf("Command TA_DPABC_STORE_SIGN failed: 0x%x / %u\n", res, err_origin);
		return STATUS_WRITE_ERROR;
	}

	printf("TA sign stored\n");

	return STATUS_OK;
}


DPABC_status DPABC_generateZKtoken(DPABC_session * session, 
				   char * pk, size_t pk_sz, 
				   char * sign_id,
				   char * epoch, size_t epoch_sz, 
				   char * attr, size_t attr_sz, 
				   int * indexReveal, size_t indexReveal_sz, 
				   char * msg, size_t msg_sz,
				   char ** zkToken, size_t * zkToken_sz) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;
	char token_buffer[DEFAULT_BUFFER_SIZE];
	bool token_initialized = false;
	char * parameters;
	size_t parameters_sz;
	char * parameters_ix;

	memset(&op, 0, sizeof(op));

	size_t sign_id_sz = strlen(sign_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT);

	// Struct of shared memory "parameters": 
	//   +---------+-------------+------------+------------------+---------+--------------+----------+----------------+
	//   |  pk_sz  |     pk      | sign_id_sz |     sign_id      | msg_sz  |     msg      | eopch_sz |     epoch      |
	//   +---------+-------------+------------+------------------+---------+--------------+----------+----------------+
	//   | 4 bytes | pk_sz bytes | 4 bytes    | sign_id_sz bytes | 4 bytes | msg_sz bytes | 4 bytes  | epoch_sz bytes |
	//   +---------+-------------+------------+------------------+---------+--------------+----------+----------------+


	parameters_sz = sizeof(uint32_t) + pk_sz  + sizeof(uint32_t) + sign_id_sz + sizeof(uint32_t) + msg_sz + sizeof(uint32_t) + epoch_sz;
	parameters = malloc(parameters_sz);
	parameters_ix = parameters;

	memcpy(parameters_ix, &pk_sz, sizeof(uint32_t));
	parameters_ix += sizeof(uint32_t);
	memcpy(parameters_ix, pk, pk_sz);
	parameters_ix += pk_sz;

	memcpy(parameters_ix, &sign_id_sz, sizeof(uint32_t));
	parameters_ix += sizeof(uint32_t);
	memcpy(parameters_ix, sign_id, sign_id_sz);
	parameters_ix += sign_id_sz;

	memcpy(parameters_ix, &msg_sz, sizeof(uint32_t));
	parameters_ix += sizeof(uint32_t);
	memcpy(parameters_ix, msg, msg_sz);
	parameters_ix += msg_sz;

	memcpy(parameters_ix, &epoch_sz, sizeof(uint32_t));
	parameters_ix += sizeof(uint32_t);
	memcpy(parameters_ix, epoch, epoch_sz);


	op.params[0].tmpref.buffer = parameters;
	op.params[0].tmpref.size = parameters_sz;

	op.params[1].tmpref.buffer = attr;
	op.params[1].tmpref.size = attr_sz;

	op.params[2].tmpref.buffer = indexReveal;
	op.params[2].tmpref.size = indexReveal_sz;

	op.params[3].tmpref.buffer = token_buffer;
	op.params[3].tmpref.size = DEFAULT_BUFFER_SIZE;

	do {
		printf("Invoking TA to generate zkToken\n");
		res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_ZKTOKEN, &op,
					 &err_origin);
		
		if (res != TEEC_SUCCESS && res != TEEC_ERROR_SHORT_BUFFER) {
			printf("Command TA_DPABC_ZKTOKEN failed: 0x%x / %u\n", res, err_origin);
			return STATUS_KEY_READ_ERROR;
		}
	
		if (res == TEEC_ERROR_SHORT_BUFFER) {
			*zkToken = malloc(op.params[3].tmpref.size);
			token_initialized = true;
			op.params[3].tmpref.buffer = *zkToken;
		}
	} while (res != TEEC_SUCCESS);

	printf("TA generated zkToken\n");
	printf("Generated zkToken size: %ld\n", op.params[3].tmpref.size);

	if (!token_initialized) {
		*zkToken = malloc(op.params[3].tmpref.size);
		memcpy(*zkToken, token_buffer, op.params[3].tmpref.size);
	}

	if (zkToken_sz)
		*zkToken_sz = op.params[3].tmpref.size;

	return STATUS_OK;

}

DPABC_status DPABC_combineSignatures(DPABC_session * session, char * combined_id, char ** pks, uint32_t * pks_sz, char ** sig_ids, int nelements) {

	uint32_t err_origin;
	TEEC_Result res;
	TEEC_Operation op;

	char * flat_pks;
	char * flat_pks_idx;
	size_t flat_pks_sz;
	char * flat_ids;
	char * flat_ids_idx;
	size_t flat_ids_sz;

	memset(&op, 0, sizeof(op));

	size_t combined_id_sz = strlen(combined_id);

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * And expect to receibe the key in the second.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE);

	flat_pks_sz = flat_ids_sz = 0;
	for (int i = 0; i < nelements; i++) {
		flat_ids_sz += strlen(sig_ids[i]);
		flat_pks_sz += pks_sz[i];
	}

	flat_pks = flat_pks_idx = malloc(flat_pks_sz);
	flat_ids = flat_ids_idx = malloc(flat_ids_sz);

	for (int i = 0; i < nelements; i++) {
		uint32_t id_sz = strlen(sig_ids[i]);

		memcpy(flat_pks_idx, &(pks_sz[i]), sizeof(uint32_t));
		flat_pks_idx += sizeof(uint32_t);
		memcpy(flat_pks_idx, pks[i], pks_sz[i]);

		memcpy(flat_ids_idx, &id_sz, sizeof(uint32_t));
		flat_ids_idx += sizeof(uint32_t);
		memcpy(flat_ids_idx, sig_ids[i], id_sz);
	}

	op.params[0].tmpref.buffer = flat_pks;
	op.params[0].tmpref.size = flat_pks_sz;

	op.params[1].tmpref.buffer = flat_ids;
	op.params[1].tmpref.size = flat_ids_sz;

	op.params[2].tmpref.buffer = combined_id;
	op.params[2].tmpref.size = combined_id_sz;

	printf("Invoking TA to combine signatures\n");
	res = TEEC_InvokeCommand(&(session->sess), TA_DPABC_COMBINE_SIGNATURES, &op,
	&err_origin);

	if (res != TEEC_SUCCESS) {
		printf("Command TA_DPABC_COMBINE_SIGNATURES failed: 0x%x / %u\n", res, err_origin);
		return STATUS_WRITE_ERROR;
	}

	printf("TA combined signatures\n");


}


DPABC_status DPABC_finalize(DPABC_session * session) {
	/*
	 * We're done with the TA, close the session and
	 * destroy the context.
	 *
	 * The TA will print "Goodbye!" in the log when the
	 * session is closed.
	 */

	TEEC_CloseSession(&(session->sess));
	TEEC_FinalizeContext(&(session->ctx));

	return STATUS_OK;
}
