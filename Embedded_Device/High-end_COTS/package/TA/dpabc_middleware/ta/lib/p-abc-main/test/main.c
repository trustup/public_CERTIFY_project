#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>
#include <Zp.h>
#include <Dpabc.h>



static void test_hidden_indexes_computation(void **state)
{
    int n=7;
    int nIndexReveal=3;
    int indexReveal[]={0,1,4};
    int nhidden=n-nIndexReveal;
    int *hidden=malloc(nhidden*sizeof(int));
    int k,i,h;
    k=i=h=0;
    while(i<n && k<nIndexReveal){
        while(i<indexReveal[k]){
            hidden[h++]=i++;
        }
        i++;
        k++;
    }
    while (i<n){
        hidden[h++]=i++;
    }
    //for(int j=0;j<nhidden;j++)
    //   print_message("%d ", hidden[j]);
    assert_int_equal(hidden[0],2);
    assert_int_equal(hidden[1],3);
    assert_int_equal(hidden[2],5);
    assert_int_equal(hidden[3],6);
	free(hidden);
}

static void test_simple_complete_flow(void **state)
{
	int nattr=3;
	int nkeys=2;
    char * seed="SeedForTheTest_test_simple_complete_flow";
	char * msg="signedMessage_flow";
	int msgLength=18;
	int seedLength=40;
	Zp **attributes=malloc(nattr*sizeof(Zp*));
	ranGen * rng=rgInit(seed,seedLength);
	publicKey **pks=malloc(nkeys*sizeof(publicKey*));
	publicKey *aggrKey;
	secretKey **sks=malloc(nkeys*sizeof(secretKey*));
	signature **partialSigns=malloc(nkeys*sizeof(signature*));
	signature *combinedSignature;
	zkToken* token;
	int nIndexReveal=2;
    int indexReveal[]={0,2};
	Zp **revealedAttributes=malloc(nIndexReveal*sizeof(Zp*));
	Zp *epoch=zpFromInt(12034);
	changeNattr(nattr);
	seedRng(seed,seedLength);
	for(int i=0;i<nattr;i++)
		attributes[i]=zpRandom(rng);
	for(int i=0;i<nkeys;i++)
		keyGen(&sks[i],&pks[i],seed,seedLength);
	aggrKey=keyAggr((const publicKey **)pks,nkeys);
	for(int i=0;i<nkeys;i++)
		partialSigns[i]=sign(sks[i],epoch,(const Zp **)attributes);
	combinedSignature=combine((const publicKey **)pks,(const signature **)partialSigns,nkeys);
	assert_true(verify(aggrKey,combinedSignature,epoch,(const Zp **)attributes));
	token=presentZkToken(aggrKey,combinedSignature,epoch,(const Zp **)attributes,indexReveal,nIndexReveal,msg,msgLength,seed,seedLength);
	revealedAttributes[0]=zpCopy(attributes[0]);
	revealedAttributes[1]=zpCopy(attributes[2]);
	assert_true(verifyZkToken(token,aggrKey,epoch,(const Zp **)revealedAttributes,indexReveal,nIndexReveal,msg,msgLength));
	for(int i=0;i<nattr;i++)
		zpFree(attributes[i]);
	zpFree(epoch);
	for(int i=0;i<nkeys;i++){
		dpabcPkFree(pks[i]);
		dpabcSkFree(sks[i]);
		dpabcSignFree(partialSigns[i]);
	}
	dpabcSignFree(combinedSignature);
	dpabcPkFree(aggrKey);
	for(int i=0;i<nIndexReveal;i++)
		zpFree(revealedAttributes[i]);
	dpabcZkFree(token);
	rgFree(rng);
	free(pks);
	free(attributes);
	free(sks);
	free(revealedAttributes);
	free(partialSigns);
	dpabcFreeStateData();
}

static void test_fraudulent_modifications_flow(void **state)
{
	int nattr=3;
	int nkeys=2;
    char * seed="SeedForTheTest_test_fraudulent_modifications";
	char * msg="signedMessage_fraud";
	int msgLength=19;
	int seedLength=44;
	Zp **attributes=malloc(nattr*sizeof(Zp*));
	Zp **modifiedAttr=malloc(nattr*sizeof(Zp*));
	ranGen * rng=rgInit(seed,seedLength);
	publicKey **pks=malloc(nkeys*sizeof(publicKey*));
	publicKey *aggrKey;
	secretKey **sks=malloc(nkeys*sizeof(secretKey*));
	signature **partialSigns=malloc(nkeys*sizeof(signature*));
	signature *combinedSignature;
	zkToken* token;
	int nIndexReveal=2;
    int indexReveal[]={0,2};
	Zp **revealedAttributes=malloc(nIndexReveal*sizeof(Zp*));
	Zp **modifiedRevealedAttributes=malloc(nIndexReveal*sizeof(Zp*));
	Zp **fewerRevealedAttributes=malloc((nIndexReveal-1)*sizeof(Zp*));
	Zp *epoch=zpFromInt(12034);
	Zp *modifiedEpoch=zpFromInt(15034);
	changeNattr(nattr);
	seedRng(seed,seedLength);
	for(int i=0;i<nattr;i++)
		attributes[i]=zpRandom(rng);
	for(int i=0;i<nattr;i++)
		modifiedAttr[i]=zpCopy(attributes[i]);
	zpAdd(attributes[0],epoch);
	for(int i=0;i<nkeys;i++)
		keyGen(&sks[i],&pks[i],seed,seedLength);
	aggrKey=keyAggr((const publicKey **)pks,nkeys);
	for(int i=0;i<nkeys;i++)
		partialSigns[i]=sign(sks[i],epoch,(const Zp **)attributes);
	combinedSignature=combine((const publicKey **)pks,(const signature **)partialSigns,nkeys);
	assert_true(verify(aggrKey,combinedSignature,epoch,(const Zp **)attributes));
	assert_false(verify(aggrKey,combinedSignature,modifiedEpoch,(const Zp **)attributes));
	assert_false(verify(aggrKey,combinedSignature,epoch,(const Zp **)modifiedAttr));
	token=presentZkToken(aggrKey,combinedSignature,epoch,(const Zp **)attributes,indexReveal,nIndexReveal,msg,msgLength,seed,seedLength);
	revealedAttributes[0]=zpCopy(attributes[0]);
	revealedAttributes[1]=zpCopy(attributes[2]);
	modifiedRevealedAttributes[0]=zpCopy(attributes[0]);
	modifiedRevealedAttributes[1]=zpCopy(attributes[2]);
	fewerRevealedAttributes[0]=zpCopy(attributes[0]);
	zpAdd(modifiedRevealedAttributes[1],modifiedRevealedAttributes[0]);
	assert_true(verifyZkToken(token,aggrKey,epoch,(const Zp **)revealedAttributes,indexReveal,nIndexReveal,msg,msgLength));
	assert_false(verifyZkToken(token,aggrKey,epoch,(const Zp **)modifiedRevealedAttributes,indexReveal,nIndexReveal,msg,msgLength));
	assert_false(verifyZkToken(token,aggrKey,epoch,(const Zp **)fewerRevealedAttributes,indexReveal,nIndexReveal-1,msg,msgLength));
	assert_false(verifyZkToken(token,aggrKey,modifiedEpoch,(const Zp **)revealedAttributes,indexReveal,nIndexReveal,msg,msgLength));
	for(int i=0;i<nattr;i++){
		zpFree(attributes[i]);
		zpFree(modifiedAttr[i]);
	}
	zpFree(epoch);
	zpFree(modifiedEpoch);
	for(int i=0;i<nkeys;i++){
		dpabcPkFree(pks[i]);
		dpabcSkFree(sks[i]);
		dpabcSignFree(partialSigns[i]);
	}
	dpabcSignFree(combinedSignature);
	dpabcPkFree(aggrKey);
	for(int i=0;i<nIndexReveal;i++){
		zpFree(revealedAttributes[i]);
		zpFree(modifiedRevealedAttributes[i]);
	}	
	zpFree(fewerRevealedAttributes[0]);
	dpabcZkFree(token);
	rgFree(rng);
	free(pks);
	free(attributes);
	free(sks);
	free(partialSigns);
	free(revealedAttributes);
	free(modifiedAttr);
	free(modifiedRevealedAttributes);
	free(fewerRevealedAttributes);
	dpabcFreeStateData();
}

static void test_flow_with_serialization(void **state)
{
	int nattr=15;
	int nkeys=2;
    char * seed="SeedForTheTest_test_flow_with_serialization";
	char * msg="signedMessage_serial";
	int msgLength=20;
	int seedLength=44;
	Zp **attributes=malloc(nattr*sizeof(Zp*));
	ranGen * rng=rgInit(seed,seedLength);
	publicKey **pks=malloc(nkeys*sizeof(publicKey*));
	publicKey *aggrKey;
	publicKey *aggrKeyRegenerated;
	secretKey **sks=malloc(nkeys*sizeof(secretKey*));
	signature **partialSigns=malloc(nkeys*sizeof(signature*));
	signature *combinedSignature;
	signature *combinedSignatureRegenerated;
	zkToken* token;
	zkToken* tokenRegenerated;
	int nIndexReveal=2;
    int indexReveal[]={0,2};
	Zp **revealedAttributes=malloc(nIndexReveal*sizeof(Zp*));
	Zp *epoch=zpFromInt(12034);
	changeNattr(nattr);
	seedRng(seed,seedLength);
	for(int i=0;i<nattr;i++)
		attributes[i]=zpRandom(rng);
	for(int i=0;i<nkeys;i++)
		keyGen(&sks[i],&pks[i],seed,seedLength);
	aggrKey=keyAggr((const publicKey **)pks,nkeys);
	char *aggrKeySerial=malloc(dpabcPkByteSize(aggrKey)*sizeof(char));
	dpabcPkToBytes(aggrKeySerial,aggrKey);
	aggrKeyRegenerated=dpabcPkFromBytes(aggrKeySerial);
	assert_true(dpabcPkEquals(aggrKey,aggrKeyRegenerated));
	for(int i=0;i<nkeys;i++)
		partialSigns[i]=sign(sks[i],epoch,(const Zp **)attributes);
	combinedSignature=combine((const publicKey **)pks,(const signature **)partialSigns,nkeys); 
	char *signSerial=malloc(dpabcSignByteSize()*sizeof(char));
	dpabcSignToBytes(signSerial,combinedSignature);
	combinedSignatureRegenerated=dpabcSignFromBytes(signSerial);
	assert_true(verify(aggrKeyRegenerated,combinedSignature,epoch,(const Zp **)attributes));
	token=presentZkToken(aggrKeyRegenerated,combinedSignatureRegenerated,epoch,(const Zp **)attributes,indexReveal,nIndexReveal,msg,msgLength,seed,seedLength);
	char *tokenSerial=malloc(dpabcZkByteSize(token)*sizeof(char));
	dpabcZkToBytes(tokenSerial,token);
	tokenRegenerated=dpabcZkFromBytes(tokenSerial);
	revealedAttributes[0]=zpCopy(attributes[0]);
	revealedAttributes[1]=zpCopy(attributes[2]);
	assert_true(verifyZkToken(tokenRegenerated,aggrKeyRegenerated,epoch,(const Zp **)revealedAttributes,indexReveal,nIndexReveal,msg,msgLength));
	for(int i=0;i<nattr;i++)
		zpFree(attributes[i]);
	zpFree(epoch);
	for(int i=0;i<nkeys;i++){
		dpabcPkFree(pks[i]);
		dpabcSkFree(sks[i]);
		dpabcSignFree(partialSigns[i]);
	}
	dpabcSignFree(combinedSignature);
	dpabcSignFree(combinedSignatureRegenerated);
	dpabcPkFree(aggrKey);
	dpabcPkFree(aggrKeyRegenerated);
	for(int i=0;i<nIndexReveal;i++)
		zpFree(revealedAttributes[i]);
	dpabcZkFree(token);
	dpabcZkFree(tokenRegenerated);
	rgFree(rng);
	free(pks);
	free(attributes);
	free(sks);
	free(revealedAttributes);
	free(partialSigns);
	free(aggrKeySerial);
	free(signSerial);
	free(tokenSerial);
	dpabcFreeStateData();
}

static void test_public_key(void **state)
{
	int nattr=15;
    char * seed="SeedForTheTest_test_public_key";
	int msgLength=18;
	int seedLength=31;
	secretKey *sk;
	publicKey *pk, *pk2;
	changeNattr(nattr);
	seedRng(seed,seedLength);
	keyGen(&sk,&pk,seed,seedLength);
	pk2=dpabcSkToPk(sk);
	assert_true(dpabcPkEquals(pk,pk2));
	dpabcSkFree(sk);
	dpabcPkFree(pk);
	dpabcPkFree(pk2);
	dpabcFreeStateData();
}

int main()
{
    const struct CMUnitTest dpabctests[] =
    {
        cmocka_unit_test(test_hidden_indexes_computation),
		cmocka_unit_test(test_simple_complete_flow),
		cmocka_unit_test(test_fraudulent_modifications_flow),
		cmocka_unit_test(test_flow_with_serialization),
		cmocka_unit_test(test_public_key)
    };
	//cmocka_set_message_output(CM_OUTPUT_XML);
	// Define environment variable CMOCKA_XML_FILE=testresults/libc.xml 
    return cmocka_run_group_tests(dpabctests, NULL, NULL);
}
