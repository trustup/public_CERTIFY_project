#include <g2.h>
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#define CEIL(a,b) (((a)-1)/(b)+1)

// Methods for hashing from AMCL, following
// https://datatracker.ietf.org/doc/draft-irtf-cfrg-hash-to-curve/,
// until they are fully integrated/standardized
/*
1. u = hash_to_field(msg, 2)
2. Q0 = map_to_curve(u[0])
3. Q1 = map_to_curve(u[1])
4. R = Q0 + Q1              # Point addition
5. P = clear_cofactor(R)
6. return P
*/

static void hash_to_field_BLS12381_G2(
	int hash,int hlen,FP2_BLS12381 *u,octet *DST,octet *M, int ctr)
{
    int i,j,L,k,m;
    BIG_384_29 q,w1,w2,r;
    DBIG_384_29 dx;
    char okm[1024],fd[256];
    octet OKM = {0,sizeof(okm),okm};
    BIG_384_29_rcopy(q, Modulus_BLS12381);
    k=BIG_384_29_nbits(q);
    BIG_384_29_rcopy(r, CURVE_Order_BLS12381);
    m=BIG_384_29_nbits(r);
    L=CEIL(k+CEIL(m,2),8);
    XMD_Expand(hash,hlen,&OKM,2*L*ctr,DST,M);
    for (i=0;i<ctr;i++)
    {
        for (j=0;j<L;j++)
            fd[j]=OKM.val[2*i*L+j];     
        BIG_384_29_dfromBytesLen(dx,fd,L);
        BIG_384_29_dmod(w1,dx,q);
        for (j=0;j<L;j++)
            fd[j]=OKM.val[(2*i+1)*L+j];
        BIG_384_29_dfromBytesLen(dx,fd,L);
        BIG_384_29_dmod(w2,dx,q);
        FP2_BLS12381_from_BIGs(&u[i],w1,w2);
    }
}

static int htp_BLS12381_G2(const char *mess, int n, ECP2_BLS12381 *P)
{
 
    int res=0;
    BIG_384_29 r;
    FP2_BLS12381 u[2];
    ECP2_BLS12381 P1;
    char dst[100];
    char msg[2000];
    octet MSG = {0,sizeof(msg),msg};
    octet DST = {0,sizeof(dst),dst};

    BIG_384_29_rcopy(r, CURVE_Order_BLS12381);
    OCT_jbytes(&MSG,mess,n);
    OCT_jstring(
	    &DST,(char *)"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_");
    hash_to_field_BLS12381_G2(MC_SHA2,HASH_TYPE_BLS12381,u,&DST,&MSG,2);
    ECP2_BLS12381_map2point(P,&u[0]);
    ECP2_BLS12381_map2point(&P1,&u[1]);
    ECP2_BLS12381_add(P,&P1);
    ECP2_BLS12381_cfp(P);
    ECP2_BLS12381_affine(P);
    return res;
}

//Header methods


G2* g2Generator(){
    G2 *r=malloc(sizeof(G2));
    r->p=malloc(sizeof(ECP2_BLS12381));
    ECP2_BLS12381_generator(r->p);
    return r;
}

G2* g2Identity(){
    G2 *r=malloc(sizeof(G2));
    r->p=malloc(sizeof(ECP2_BLS12381));
    ECP2_BLS12381_inf(r->p);
    return r;
}

int g2ByteSize(){
    return 4*MODBYTES_384_29+1; //0x04|x|y with x,y in FP2 serialized
				//as a|b from FP
}

void g2ToBytes(char *res, const G2 *a){
	if (g2IsIdentity(a)) {
		memset(res, 0x6c, g2ByteSize());
	} else {
		octet o={0,4*MODBYTES_384_29+1,res};
		ECP2_BLS12381_toOctet(&o,a->p,false);
	}
}

G2* hashToG2(const char *bytes, int n){
    G2 *r=malloc(sizeof(G2));
    r->p=malloc(sizeof(ECP2_BLS12381));
    htp_BLS12381_G2(bytes,n,r->p);
    return r;
}

G2 * g2FromBytes(const char *bytes){
	if (bytes[0] == 0x6c) {
		return g2Identity();
	} else {
		G2 *r=malloc(sizeof(G2));
		r->p=malloc(sizeof(ECP2_BLS12381));
		octet o={0,4*MODBYTES_384_29+1,bytes};
		ECP2_BLS12381_fromOctet(r->p,&o);
		return r;
	}
}

void g2Add(G2* a, const G2* b){
    ECP2_BLS12381_add(a->p,b->p); 
}

void g2Sub(G2* a, const G2* b){
    ECP2_BLS12381_sub(a->p,b->p); 
}

void g2Mul(G2* a, const Zp* b){
    ECP2_BLS12381_mul(a->p,b->z); 
}

void g2InvMul(G2* a, const Zp* b){
    Zp aux;
    BIG_384_29_rcopy(aux.z,b->z);
    zpNeg(&aux);
    ECP2_BLS12381_mul(a->p,aux.z); 
}

int g2IsIdentity(const G2* a){
    return ECP2_BLS12381_isinf(a->p);
}

int g2Equals(const G2* a, const G2* b){
    return ECP2_BLS12381_equals(a->p,b->p);
}

G2* g2Copy(const G2* a){
    G2 * res=malloc(sizeof(G2));
    res->p=malloc(sizeof(ECP2_BLS12381));
    ECP2_BLS12381_copy(res->p,a->p);
    return res;
}

void g2CopyValue(G2* a, const G2* b){
    ECP2_BLS12381_copy(a->p,b->p);
}

void g2Print(const G2* e){
    ECP2_BLS12381_outputxyz(e->p);
}

void g2Free(G2* e){
    free(e->p);
    free(e);
}
