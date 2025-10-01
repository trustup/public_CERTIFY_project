#include <stdio.h>
#include <stdlib.h>
#include <Zp.h>
#include <Dpabc.h>
#include <time.h>


int main() {
    printf("Starting example\n");
    int nattr=20; // NOTE: The maximum number of attributes as of now is 256, as uint8 is used for representation
	int nkeys=2;
    char * seed="SeedForMainExampleBinary";
	char * msg="signedMessage";
	int msgLength=13;
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
	printf("Starting nattr %d, nreveal %d, nkeys %d\n",nattr,nIndexReveal,nkeys);
    changeNattr(nattr);
	seedRng(seed,seedLength);
	clock_t start_time = clock();
	for(int i=0;i<nattr;i++)
		attributes[i]=zpRandom(rng);
	clock_t current_time = clock();
	printf("attr %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time = current_time;
	for(int i=0;i<nkeys;i++)
		keyGen(&(sks[i]),&(pks[i]));
	current_time = clock();
	printf("keygen %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time = current_time;
	aggrKey=keyAggr((const publicKey **)pks,nkeys);
	current_time = clock();
	printf("kaggr %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	for(int i=0;i<nkeys;i++)
		partialSigns[i]=sign(sks[i],epoch,(const Zp **)attributes);
	combinedSignature=combine((const publicKey **)pks,(const signature **)partialSigns,nkeys);
	current_time = clock();
	printf("comb %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	printf("Verification result 0: %d\n",verify(pks[0],partialSigns[0],epoch,(const Zp **)attributes));
	current_time = clock();
	printf("verf %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	printf("Verification result 1: %d\n",verify(pks[1],partialSigns[1],epoch,(const Zp **)attributes));
	current_time = clock();
	printf("verf %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	printf("Verification result: %d\n",verify(aggrKey,combinedSignature,epoch,(const Zp **)attributes));
	current_time = clock();
	printf("verf %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	token=presentZkToken(aggrKey,combinedSignature,epoch,(const Zp **)attributes,indexReveal,nIndexReveal,msg,msgLength);
	current_time = clock();
	printf("zkpresent %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	revealedAttributes[0]=zpCopy(attributes[0]);
	revealedAttributes[1]=zpCopy(attributes[2]);
	printf("Zk verification result: %d\n",verifyZkToken(token,aggrKey,epoch,(const Zp **)revealedAttributes,indexReveal,nIndexReveal,msg,msgLength));
	current_time = clock();
	printf("zkverf %lf\n",(double)(current_time - start_time) / CLOCKS_PER_SEC);
	start_time=current_time;
	for(int i=0;i<nattr;i++)
		zpFree(attributes[i]);
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
    zpFree(epoch);
	rgFree(rng);
	free(pks);
	free(attributes);
	free(sks);
	free(revealedAttributes);
	free(partialSigns);
	dpabcFreeStateData();
    return 0;
}
