#include <Dpabc_types.h>
#include "types_impl.h"
#include <stdlib.h>


publicKey *dpabcSkToPk(const secretKey *sk){
    publicKey* res= malloc(sizeof(publicKey)+sk->n*sizeof(G1*));
    res->vx=g1Generator();
    g1Mul(res->vx,sk->x);
    res->vy_m=g1Generator();
    g1Mul(res->vy_m,sk->y_m);
    res->vy_epoch=g1Generator();
    g1Mul(res->vy_epoch,sk->y_epoch);
    res->n=sk->n;
    for(int i=0;i<res->n;i++){
        res->vy[i]=g1Generator();
        g1Mul(res->vy[i],sk->y[i]);
    }  
    return res;  
}

void dpabcPkFree(publicKey *pk){
    g1Free(pk->vx);
    g1Free(pk->vy_epoch);
    g1Free(pk->vy_m);
    for(int i=0;i<pk->n;i++)
        g1Free(pk->vy[i]);
    free(pk);
}

int dpabcPkByteSize(const publicKey *pk){
    return g1ByteSize()*(pk->n+3)+1; // +1 for array size (uint8)
}

void dpabcPkToBytes(char *res, const publicKey *pk){
    int g1Bytes=g1ByteSize();
    char * aux=res;
    *aux=pk->n;
    aux=aux+1;
    g1ToBytes(aux,pk->vx);
    aux=aux+g1Bytes;
    g1ToBytes(aux,pk->vy_m);
    aux=aux+g1Bytes;
    g1ToBytes(aux,pk->vy_epoch);
    aux=aux+g1Bytes;
    for(int i=0;i<pk->n;i++){
        g1ToBytes(aux,pk->vy[i]);
        aux=aux+g1Bytes;
    }
}

publicKey * dpabcPkFromBytes(const char *bytes){
    int g1Bytes=g1ByteSize();
    char * aux=bytes;
    uint8_t n=bytes[0];
    publicKey *res= malloc(sizeof(publicKey)+n*sizeof(G1*));
    res->n=n;
    aux=aux+1;
    res->vx=g1FromBytes(aux);
    aux=aux+g1Bytes;
    res->vy_m=g1FromBytes(aux);
    aux=aux+g1Bytes;
    res->vy_epoch=g1FromBytes(aux);
    aux=aux+g1Bytes;
    for(int i=0;i<res->n;i++){
        res->vy[i]=g1FromBytes(aux);
        aux=aux+g1Bytes;
    }
    return res;
}

int dpabcPkEquals(const publicKey *pk1, const publicKey *pk2){
    if(pk1->n!=pk2->n)
        return 0;
    for(int i=0;i<pk1->n;i++){
        if(!g1Equals(pk1->vy[i],pk2->vy[i]))
            return 0;
    }
    return g1Equals(pk1->vx,pk2->vx) && g1Equals(pk1->vy_epoch,pk2->vy_epoch) && g1Equals(pk1->vy_m,pk2->vy_m);
}


void dpabcSkFree(secretKey *sk){
    zpFree(sk->x);
    zpFree(sk->y_epoch);
    zpFree(sk->y_m);
    for(int i=0;i<sk->n;i++)
        zpFree(sk->y[i]);
    free(sk);
}


int dpabcSkByteSize(const secretKey *sk){
    return zpByteSize()*(sk->n+3)+1;
}


void dpabcSkToBytes(char *res, const secretKey *sk){
    int zpBytes=zpByteSize();
    char * aux;
    aux=res;
    *aux=sk->n;
    aux=aux+1;
    zpToBytes(aux,sk->x);
    aux=aux+zpBytes;
    zpToBytes(aux,sk->y_m);
    aux=aux+zpBytes;
    zpToBytes(aux,sk->y_epoch);
    aux=aux+zpBytes;
    for(int i=0;i<sk->n;i++){
        zpToBytes(aux,sk->y[i]);
        aux=aux+zpBytes;
    }
}


secretKey * dpabcSkFromBytes(const char *bytes){
    int zpBytes=zpByteSize();
    char * aux=bytes;
    uint8_t n=bytes[0];
    secretKey *res= malloc(sizeof(secretKey)+n*sizeof(Zp*));
    res->n=n;
    aux=aux+1;
    res->x=zpFromBytes(aux);
    aux=aux+zpBytes;
    res->y_m=zpFromBytes(aux);
    aux=aux+zpBytes;
    res->y_epoch=zpFromBytes(aux);
    aux=aux+zpBytes;
    for(int i=0;i<res->n;i++){
        res->y[i]=zpFromBytes(aux);
        aux=aux+zpBytes;
    }
    return res; 
}

void dpabcSignFree(signature *sign){
    zpFree(sign->mprime);
    g2Free(sign->sigma1);
    g2Free(sign->sigma2);
    free(sign);
}

int dpabcSignByteSize(){
    return zpByteSize()+g2ByteSize()*2;
}

void dpabcSignToBytes(char *res, const signature *sig){
    int g2bytes=g2ByteSize();
    char *aux=res;
    g2ToBytes(aux,sig->sigma1);
    aux=aux+g2bytes;
    g2ToBytes(aux,sig->sigma2);
    aux=aux+g2bytes;
    zpToBytes(aux,sig->mprime);
}

signature * dpabcSignFromBytes(const char *bytes){
    int g2bytes=g2ByteSize();
    char *aux=bytes;
    signature *res=malloc(sizeof(signature));
    res->sigma1=g2FromBytes(aux);
    aux=aux+g2bytes;
    res->sigma2=g2FromBytes(aux);
    aux=aux+g2bytes;
    res->mprime=zpFromBytes(aux);
    return res;
}

void dpabcZkFree(zkToken *zk){
    g2Free(zk->sigma1);
    g2Free(zk->sigma2);   
    zpFree(zk->c);
    zpFree(zk->v_t);
    zpFree(zk->v_mprime);
    for(int i=0;i<zk->n;i++)
        zpFree(zk->v_mj[i]);
    free(zk);
}


int dpabcZkByteSize(zkToken *zk){
    return 2*g2ByteSize()+(3+zk->n)*zpByteSize()+1;

}

void dpabcZkToBytes(char *res, const zkToken *zk){
    int g2bytes=g2ByteSize();
    int zpBytes=zpByteSize();
    char *aux=res;
    *aux=zk->n;
    aux=aux+1;
    g2ToBytes(aux,zk->sigma1);
    aux=aux+g2bytes;
    g2ToBytes(aux,zk->sigma2);
    aux=aux+g2bytes;
    zpToBytes(aux,zk->c);
    aux=aux+zpBytes;
    zpToBytes(aux,zk->v_t);
    aux=aux+zpBytes;
    zpToBytes(aux,zk->v_mprime);
    aux=aux+zpBytes;
    for(int i=0;i<zk->n;i++){
        zpToBytes(aux,zk->v_mj[i]);
        aux=aux+zpBytes;
    }
}

zkToken * dpabcZkFromBytes(const char *bytes){
    int g2bytes=g2ByteSize();
    int zpBytes=zpByteSize();
    char *aux=bytes;
    uint8_t n=bytes[0];
    zkToken * res=malloc(sizeof(zkToken)+sizeof(Zp*[n]));
    res->n=n;
    aux=aux+1;
    res->sigma1=g2FromBytes(aux);
    aux=aux+g2bytes;
    res->sigma2=g2FromBytes(aux);
    aux=aux+g2bytes;
    res->c=zpFromBytes(aux);
    aux=aux+zpBytes;
    res->v_t=zpFromBytes(aux);
    aux=aux+zpBytes;
    res->v_mprime=zpFromBytes(aux);
    aux=aux+zpBytes;
    for(int i=0;i<res->n;i++){
        res->v_mj[i]=zpFromBytes(aux);
        aux=aux+zpBytes;
    }
    return res;
}