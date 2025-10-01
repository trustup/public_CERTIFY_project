#include <Dpabc.h>
#include "types_impl.h"
#include "Dpabc_utils.h"
#include <pair.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>


static int nattr=NATTRINI; // Instead we could simply put an extra argument at keyGen (as keys are always used in other methods and have the number of attributes stored)
static ranGen *rng=NULL;
//TODO Change comments to additive notation

void changeNattr(int n){
    nattr=n;
}

void seedRng(const char* seed,int n){
    if(rng!=NULL)
        rgFree(rng);
    rng=rgInit(seed,n);
}

void createSeed(char * seed, char* length){ //TODO Crypto safe seed generation 
    //srand(time(NULL));
    for (int i=0;i<length;i++){
        seed[i]=rand();
    }
}


void keyGen(secretKey ** sk, publicKey ** pk){
    if(rng==NULL){
        int seedLength=128;
        char * newSeed=malloc(seedLength*sizeof(char));
        createSeed(newSeed,seedLength);
        seedRng(newSeed,seedLength);
    }
    //Error handling: Check random generator ready
    secretKey * newsk;
    publicKey * newpk;
    //Generate random Zp elements for the secret key sk.
    *sk= malloc(sizeof(secretKey)+nattr*sizeof(Zp*));
    newsk=*sk;
    newsk->x=zpRandom(rng);
    newsk->y_m=zpRandom(rng);
    newsk->y_epoch=zpRandom(rng);
    newsk->n=nattr;
    for(int i=0;i<nattr;i++)
        newsk->y[i]=zpRandom(rng);
    //Generate the corresponding verification key through exponentiation of generator by the sk members.
    *pk= malloc(sizeof(publicKey)+nattr*sizeof(G1*));
    newpk=*pk;
    newpk->vx=g1Generator();
    g1Mul(newpk->vx,newsk->x);
    newpk->vy_m=g1Generator();
    g1Mul(newpk->vy_m,newsk->y_m);
    newpk->vy_epoch=g1Generator();
    g1Mul(newpk->vy_epoch,newsk->y_epoch);
    newpk->n=nattr;
    for(int i=0;i<nattr;i++){
        newpk->vy[i]=g1Generator();
        g1Mul(newpk->vy[i],newsk->y[i]);
    }    
}


publicKey* keyAggr(const publicKey *pks[], int nkeys){
    //Error handling:Check sizes match for keys
    //Error handling: Check nkeys value
    uint8_t n=pks[0]->n;
    Zp **t=malloc(nkeys*sizeof(Zp*));
    publicKey * avk=malloc(sizeof(publicKey)+n*sizeof(G1*));
    G1 **auxArrayG1=malloc(nkeys*sizeof(G1*)); //Will just hold pointers so we can use the Muln method properly
    //Generate t<-H1(Verification keys)
    hash1(pks,nkeys,t);
    avk->n=n;
    // Multiplication+exponentiation of member X of the verification keys (Getting X member of Avk).
    for(int i=0;i<nkeys;i++){
        auxArrayG1[i]=pks[i]->vx;
    }
    avk->vx=g1Muln(auxArrayG1,t,nkeys);
    // Multiplication+exponentiation of member Y_epoch of the verification keys (Getting Y_epoch member of Avk).
    for(int i=0;i<nkeys;i++){
        auxArrayG1[i]=pks[i]->vy_epoch;
    }
    avk->vy_epoch=g1Muln(auxArrayG1,t,nkeys);
    // Multiplication+exponentiation of member Y_m' of the verification keys (Getting Y_m' member of Avk).
    for(int i=0;i<nkeys;i++){
        auxArrayG1[i]=pks[i]->vy_m;
    }
    avk->vy_m=g1Muln(auxArrayG1,t,nkeys);
    // Multiplication+exponentiation of members Y_i of the verification keys (Getting Y_i members of Avk).
    for(int j=0;j<n;j++){
        for(int i=0;i<nkeys;i++){
            auxArrayG1[i]=pks[i]->vy[j];
        }
        avk->vy[j]=g1Muln(auxArrayG1,t,nkeys);
    }
    for(int i=0;i<nkeys;i++)
        zpFree(t[i]);
    free(t);
    free(auxArrayG1);
    return avk;
}


signature* sign(const secretKey *sk, const Zp *epoch, const Zp *attributes[]){
    //Error handling: Check attributes/key sizes
    signature *result=malloc(sizeof(signature));
    Zp *exp, *aux;
    uint8_t n=sk->n;
    //Obtain (m',h) <- H0(m)
    hash0(attributes,n,&result->mprime,&result->sigma1);
    //Generate exponent of third member of the signature sigma2
    exp=zpCopy(sk->x);
    aux=zpCopy(result->mprime);
    zpMul(aux,sk->y_m);
    zpAdd(exp,aux);
    zpCopyValue(aux,epoch);
    zpMul(aux,sk->y_epoch);
    zpAdd(exp,aux);
    for(int i=0;i<n;i++){
        zpCopyValue(aux,attributes[i]);
        zpMul(aux,sk->y[i]);
        zpAdd(exp,aux);
    }
    result->sigma2=g2Copy(result->sigma1);
    g2Mul(result->sigma2,exp);
    zpFree(aux);
    zpFree(exp);
    return result;
}


signature* combine(const publicKey *pks[], const signature *signs[], int nkeys){
    //Error handling: Number of signatures/keys is the same
    //Error handling: Check same mprime/sigma1?
    signature *result=malloc(sizeof(signature));
    Zp **t=malloc(nkeys*sizeof(Zp*));
    G2 *aux;
    //Get t<-H1(Verification keys)
    hash1(pks,nkeys,t);
    //Multiplication+exponentiation of sigma 2 of the signature shares.
    result->mprime=zpCopy(signs[0]->mprime);
    result->sigma1=g2Copy(signs[0]->sigma1);
    result->sigma2=g2Copy(signs[0]->sigma2);
    g2Mul(result->sigma2,t[0]);
    for(int i=1;i<nkeys;i++){
        if(i==1)
            aux=g2Copy(signs[i]->sigma2);
        else
            g2CopyValue(aux,signs[i]->sigma2);
        g2Mul(aux,t[i]);
        g2Add(result->sigma2,aux);
    }
    for(int i=0;i<nkeys;i++)
        zpFree(t[i]);
    free(t);
    g2Free(aux);
    return result;
}


int verify(const publicKey *pk, const signature* sign, const Zp *epoch, const Zp *attributes[]){
    //Error handling: Check number of attributes
    G1 *el2, *aux, *generator;
    G1 **auxArray=malloc(pk->n*sizeof(G1*));
    G3 *lh, *rh;
    int result;
    //Check sigma1!=1G
    if(g2IsIdentity(sign->sigma1))
        return 0;
    //Obtain X * (Y_m')^m'* Prod (Y_i)^m_i
    el2=g1Copy(pk->vx);
    aux=g1Copy(pk->vy_m);
    g1Mul(aux,sign->mprime);
    g1Add(el2,aux);
    g1CopyValue(aux,pk->vy_epoch);
    g1Mul(aux,epoch);
    g1Add(el2,aux);
    for(int i=0;i<pk->n;i++){
        auxArray[i]=pk->vy[i];
    }
    g1Free(aux);
    aux=g1Muln(auxArray,attributes,pk->n);
    g1Add(el2,aux);
    //Check pairing condition e(sigma1, X * (Y_m')^m'* Prod (Y_i)^m_i )=e(sigma2,Group1generator)
    generator=g1Generator();
    lh=pair(el2, sign->sigma1);
    rh=pair(generator, sign->sigma2);
    result=g3equals(lh,rh);
    g1Free(aux);
    g1Free(el2);
    g1Free(generator);
    g3Free(lh);
    g3Free(rh);
    free(auxArray);
    return result;
}

void computeHidden(int hidden[],const int *indexReveal,int nIndexReveal,int n,int nhidden){
    int k,i,h;
        k=i=h=0;
        while(i<n && k<nIndexReveal){
            while(i<indexReveal[k]){
                hidden[h++]=i++;
            }
            i++;
            k++;
        }
        while (i<n)
            hidden[h++]=i++;
}

zkToken* presentZkToken(const publicKey * pk, const signature *sign, const Zp *epoch, const Zp *attributes[], 
        const int indexReveal[], int nIndexReveal, const char *message, int messageSize){
    if(rng==NULL){
        int seedLength=128;
        char * newSeed=malloc(seedLength*sizeof(char));
        createSeed(newSeed,seedLength);
        seedRng(newSeed,seedLength);
    }
    //Error handling: Check random generator ready
    //Error handling: Check number of attributes
    //Error handling: Consistent and ordered revealed attributes
    Zp *r,*t, *auxZp;
    int nhidden=pk->n-nIndexReveal;
    G2 *auxG2;
    G1 *auxG1, *aux2G1;
    G1** auxArray;
    if(nhidden>0)
        auxArray=malloc(nhidden*sizeof(G1*));
    G3 *pairRes;
    zkToken  *token=malloc(sizeof(zkToken)+nhidden*sizeof(Zp*));
    token->n=nhidden;
    //Hidden attributes
    int *hidden;
    if(nhidden>0){
        hidden=malloc(nhidden*sizeof(int));
        computeHidden(hidden,indexReveal,nIndexReveal,pk->n,nhidden);
    }
    //Generate random Zp elements and sigma1', sigma2'
    r=zpRandom(rng);
    t=zpRandom(rng);
    token->sigma1=g2Copy(sign->sigma1); //sigma1^r
    g2Mul(token->sigma1,r);
    token->sigma2=g2Copy(sign->sigma2); //(sigma2*sigma1^t)^r
    auxG2=g2Copy(sign->sigma1);
    g2Mul(auxG2,t);
    g2Add(token->sigma2,auxG2);
    g2Mul(token->sigma2,r);
    //Generate random exponents for t, m' and hidden attributes
    token->v_t=zpRandom(rng);
    token->v_mprime=zpRandom(rng);
    for(int j=0;j<nhidden;j++)
        token->v_mj[j]=zpRandom(rng);
    //Calculate c
    auxG1=g1Generator();
    g1Mul(auxG1,token->v_t);
    aux2G1=g1Copy(pk->vy_m);
    g1Mul(aux2G1,token->v_mprime);
    g1Add(auxG1,aux2G1);
    if(nhidden>0){
        for(int j=0;j<nhidden;j++){
            auxArray[j]=pk->vy[hidden[j]];
        }
        g1Free(aux2G1);
        aux2G1=g1Muln(auxArray,token->v_mj,nhidden);
        g1Add(auxG1,aux2G1);
    }
    pairRes=pair(auxG1,token->sigma1);
    hash2(message,messageSize,pk,token->sigma1,token->sigma2,pairRes, &token->c); 
    //Calculate v_i= ran_i - c * i
    auxZp=zpCopy(token->c);
    zpMul(auxZp,t);
    zpSub(token->v_t,auxZp);
    zpCopyValue(auxZp,token->c);
    zpMul(auxZp,sign->mprime);
    zpSub(token->v_mprime,auxZp);
    for(int j=0;j<nhidden;j++){
        zpCopyValue(auxZp,token->c); 
        zpMul(auxZp,attributes[hidden[j]]);
        zpSub(token->v_mj[j],auxZp);
    }
    zpFree(t);
    zpFree(r);
    zpFree(auxZp);
    g1Free(auxG1);
    g2Free(auxG2); 
    g1Free(aux2G1);
    g3Free(pairRes);
    if(nhidden>0){
        free(hidden);
        free(auxArray);
    }
    return token;
}

int verifyZkToken(const zkToken *token, const publicKey * pk, const Zp *epoch, const Zp *revealed[],
        const int indexReveal[], int nReveal, const char *message, int messageSize){
    //Error handling: Consistent and ordered revealed/hidden/total attributes
    if(token->n+nReveal!=pk->n)
        return 0;
    G1 *auxEl, *auxG1;
    G2 *auxG2;
    Zp *auxZp, *aux2Zp;
    G1** auxArray;
    int *hidden;
    if(token->n>0) 
        auxArray=malloc(token->n*sizeof(G1*));
    //G1 *el1Pair[2];
    //G2 *el2Pair[2];
    G3 *pairRes;
    int result;
    if(g2IsIdentity(token->sigma1) || g2IsIdentity(token->sigma2))
        return 0;
    //Compute hidden
    if(token->n>0){
        hidden=malloc(token->n*sizeof(int));
        computeHidden(hidden,indexReveal,nReveal,pk->n,token->n);
    } 
    //Compute the necessary pairings/operations
    auxEl=g1Generator();
    g1Mul(auxEl,token->v_t);
    auxG1=g1Copy(pk->vy_m);
    g1Mul(auxG1,token->v_mprime);
    g1Add(auxEl,auxG1);
    g1CopyValue(auxG1,pk->vx);
    g1InvMul(auxG1,token->c);
    g1Add(auxEl,auxG1);
    g1CopyValue(auxG1,pk->vy_epoch);
    auxZp=zpCopy(token->c);
    zpMul(auxZp,epoch);
    g1InvMul(auxG1,auxZp);
    g1Add(auxEl,auxG1);
    if(token->n>0){
        for (int j=0;j<token->n;j++){
            auxArray[j]=pk->vy[hidden[j]];
        }
        g1Free(auxG1);
        auxG1=g1Muln(auxArray,token->v_mj,token->n);
        g1Add(auxEl,auxG1);
    }
    for (int j=0;j<nReveal;j++){
        g1CopyValue(auxG1,pk->vy[indexReveal[j]]);      //TODO join into one n-multiplication
        zpCopyValue(auxZp,token->c);
        zpMul(auxZp,revealed[j]);
        g1InvMul(auxG1,auxZp);
        g1Add(auxEl,auxG1);
    }
    auxG2=g2Copy(token->sigma2);
    g2Mul(auxG2,token->c);
    g1Free(auxG1);
    auxG1=g1Generator(); 
    /* Using multiPair method
    el1Pair[0]=auxEl;
    el1Pair[1]=auxG1;
    el2Pair[0]=token->sigma1;
    el2Pair[1]=auxG2;
    pairRes=multipair(el1Pair,el2Pair,2);
    */
    pairRes=doublepair(auxEl,auxG1,token->sigma1,auxG2); //Using doublePair method
    hash2(message,messageSize,pk,token->sigma1,token->sigma2,pairRes,&aux2Zp);
    result=zpEquals(token->c,aux2Zp);
    zpFree(auxZp);
    zpFree(aux2Zp);
    g1Free(auxG1);
    g1Free(auxEl);
    g2Free(auxG2);
    g3Free(pairRes);
    if(token->n>0){
        free(hidden);
        free(auxArray);
    }
    return result;
}




void dpabcFreeStateData(){
    if(rng!=NULL){
        rgFree(rng);
        rng=NULL;
    }
}




