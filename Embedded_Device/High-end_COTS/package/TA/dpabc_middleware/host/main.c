#include "Dpabc_types.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <aceunit.h>

extern jmp_buf *AceUnit_env;

#include <Zp.h>
#include <Dpabc.h>
#include <dpabc_middleware.h>

DPABC_session session;
char * pk;
size_t pk_sz;
zkToken * token;
char * sig;
size_t sig_sz;
char * tokenBytes;
size_t zksize;

char * keyID = "test_generate_key";
char * keyID2 = "test_generate_duplicate_key";
char * sign_id = "test_signature";
char * sign_id_2 = "test_signature_two";

int nattr = 6;

void beforeAll() { DPABC_initialize(&session); }

void testGenerateKey() {

	assert(DPABC_generate_key(&session, keyID, 6) == STATUS_OK);

}

void testGenerateDuplicateKey() {

	assert(DPABC_generate_key(&session, keyID2, 6) == STATUS_OK);
	assert(DPABC_generate_key(&session, keyID2, 6) == STATUS_DUPLICATE_KEY);

}

void testGetPublicKey() {

	assert(DPABC_get_key(&session, keyID, &pk, &pk_sz) == STATUS_OK);
}

void testSign() {

	Zp * attr[] = {zpFromInt(0), zpFromInt(1), zpFromInt(-1),
				zpFromInt(0), zpFromInt(1), zpFromInt(-1)};

	char * binaryAttr = malloc(nattr*zpByteSize());
	for (int i = 0; i < nattr; i++)
		zpToBytes(binaryAttr + i*zpByteSize(), attr[i]);

	char * epoch = malloc(zpByteSize());
	zpToBytes(epoch, zpFromInt(3));
 
	assert(DPABC_sign(&session, keyID, epoch, zpByteSize(), binaryAttr, nattr*zpByteSize(), &sig, NULL) == STATUS_OK);

	assert(DPABC_storeSignature(&session, sign_id, sig, dpabcSignByteSize()) == STATUS_OK);
	assert(DPABC_storeSignature(&session, sign_id, sig, dpabcSignByteSize()) == STATUS_DUPLICATE_SIGNATURE);

	free(binaryAttr);
	free(epoch);
}

void testSignStore() {

	Zp * attr[] = {zpFromInt(2), zpFromInt(1), zpFromInt(-1),
				zpFromInt(2), zpFromInt(1), zpFromInt(-1)};

	char * binaryAttr = malloc(nattr*zpByteSize());
	for (int i = 0; i < nattr; i++)
		zpToBytes(binaryAttr + i*zpByteSize(), attr[i]);

	char * epoch = malloc(zpByteSize());
	zpToBytes(epoch, zpFromInt(3));
 
	assert(DPABC_signStore(&session, keyID, epoch, zpByteSize(), binaryAttr, nattr*zpByteSize(), sign_id_2) == STATUS_OK);
	assert(DPABC_signStore(&session, keyID, epoch, zpByteSize(), binaryAttr, nattr*zpByteSize(), sign_id_2) == STATUS_DUPLICATE_SIGNATURE);

	free(binaryAttr);
	free(epoch);
}

void testVerify() {

	Zp * attr[] = {zpFromInt(0), zpFromInt(1), zpFromInt(-1),
				zpFromInt(0), zpFromInt(1), zpFromInt(-1)};
	publicKey * dpabc_pk = dpabcPkFromBytes(pk);
	signature * dpabc_sig = dpabcSignFromBytes(sig);
	assert(verify(dpabc_pk, dpabc_sig, zpFromInt(3), (const Zp **)attr) == 1);
	dpabcPkFree(dpabc_pk);
	dpabcSignFree(dpabc_sig);
}

void testVerifyStored() {
	
	Zp * attr[] = {zpFromInt(2), zpFromInt(1), zpFromInt(-1),
				zpFromInt(2), zpFromInt(1), zpFromInt(-1)};

	char * binaryAttr = malloc(nattr*zpByteSize());
	for (int i = 0; i < nattr; i++)
		zpToBytes(binaryAttr + i*zpByteSize(), attr[i]);

	char * epoch = malloc(zpByteSize());
	zpToBytes(epoch, zpFromInt(3));

	assert(DPABC_verifyStored(&session, pk, pk_sz, epoch, zpByteSize(), binaryAttr, nattr * zpByteSize(), sign_id_2) == STATUS_OK);
	assert(DPABC_verifyStored(&session, pk, pk_sz, epoch, zpByteSize(), binaryAttr, nattr * zpByteSize(), sign_id) != STATUS_OK);
}

void testZkToken() {

	char * msg="signedMessage";
	int msgLength=13;
	uint32_t indexReveal[]={0,2};
	Zp * attr[] = {zpFromInt(2), zpFromInt(1), zpFromInt(-1),
				zpFromInt(2), zpFromInt(1), zpFromInt(-1)};

	char * binaryAttr = malloc(nattr*zpByteSize());
	for (int i = 0; i < nattr; i++)
		zpToBytes(binaryAttr + i*zpByteSize(), attr[i]);

	char * epoch = malloc(zpByteSize());
	zpToBytes(epoch, zpFromInt(3));

	assert(DPABC_generateZKtoken(&session, 
			      pk, pk_sz,
			      sign_id_2, 
			      epoch, zpByteSize(), 
		              binaryAttr, nattr * zpByteSize(), 
			      indexReveal, sizeof(indexReveal), 
		              msg, msgLength, 
			      &tokenBytes, &zksize) == STATUS_OK);

	free(binaryAttr);
	free(epoch);
}

void testverifyZkToken() {

	char * msg="signedMessage";
	int msgLength=13;
	int nIndexReveal=2;
	int indexReveal[]={0,2};
	Zp * revealedAttributes[] = {zpFromInt(2), zpFromInt(-1)};
	publicKey * dpabc_pk = dpabcPkFromBytes(pk);
	zkToken * token = dpabcZkFromBytes(tokenBytes);
	int res = verifyZkToken(token, dpabc_pk, zpFromInt(3), (const Zp **)revealedAttributes, indexReveal, nIndexReveal, msg, msgLength);
	assert(res != 0);
}

void afterAll() { 
	free(pk);
	free(sig);
	DPABC_finalize(&session); 
}





