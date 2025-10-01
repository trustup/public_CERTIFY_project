#include <Zp.h>
#include "types.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define P CURVE_Order_BLS12381

extern const BIG_384_29 P;
//Operations with P discard const (though it is not modified). Happens with
//other qualifiers too

// TODO Not the most efficient. We could try to manage internally modules (e.g.,
// not applying modulo until it is necessary because of underlying
// representation, slothfull/lazy reduction)

int zpByteSize(){
    return MODBYTES_384_29;
}

void zpToBytes(char *res, const Zp *a){
    BIG_384_29_toBytes(res,a->z);
}

Zp * zpFromBytes(const char *bytes){
    Zp * r=malloc(sizeof(Zp));
    BIG_384_29_fromBytes(r->z,bytes);
    return r;
}

Zp *hashToZp(const char * bytes,int nBytes){
    Zp * r=malloc(sizeof(Zp));
    hash384 h;
    HASH384_init(&h);
    int hashSize=h.hlen;
    char * hashed=malloc(hashSize*sizeof(char));
    for(int i=0;i<nBytes;i++)
        HASH384_process(&h,bytes[i]);
    HASH384_hash(&h,hashed);
    BIG_384_29_fromBytesLen(r->z,hashed,hashSize);
    free(hashed);
    return r;
}

Zp* zpFromInt (int a){
    Zp * r=malloc(sizeof(Zp));
    if (r == NULL)
	    return NULL;
    if (a >= 0) {
	    BIG_384_29_zero(r->z);
	    BIG_384_29_inc(r->z,a);
    } else {
	    BIG_384_29_copy(r->z, P);
	    if (a == INT_MIN) {
		    BIG_384_29_dec(r->z, INT_MAX);
		    BIG_384_29_dec(r->z, -(INT_MAX+INT_MIN));
	    } else { /* Overflow can't happen here on C99 */
		    BIG_384_29_dec(r->z, -a);
	    }
    }
    return r;
}

Zp* zpRandom(ranGen *rg){
    Zp *r=malloc(sizeof(Zp));
    BIG_384_29_randomnum(r->z,P,rg->rg);
    return r;
}

void zpRandomValue(ranGen *rg, Zp *r){
    BIG_384_29_randomnum(r->z,P,rg->rg);
}

Zp* zpModulus(){
    Zp *r=malloc(sizeof(Zp));
    BIG_384_29_copy(r->z,P);
    return r;
}

void zpAdd (Zp *a, const Zp *b){
    // Have to be careful with this kind of call (add method supports
    // "overwriting", but need to investigate whether the whole library is
    // designed that way)
    BIG_384_29_modadd(a->z,a->z,b->z,P); 
}

void zpSub(Zp* a, const Zp* b){
    if(BIG_384_29_comp(a->z,b->z)<0){
        BIG_384_29 aux;
         // First ensure that LHS and RHS are reduce mod p
        BIG_384_29_mod(a->z,P);
        BIG_384_29_mod(b->z,P);
         // Reverse the LHS and RHS to get the absolute value of the subtraction
        BIG_384_29_sub(aux,b->z,a->z);
         // Finally subtract the absolute value from the modulus (p) to end up
         // with the corect result mod p
        BIG_384_29_rcopy(a->z,P);
        BIG_384_29_sub(a->z,a->z,aux);
    }else
        BIG_384_29_sub(a->z,a->z,b->z);
}

void zpMul (Zp *a, const Zp *b){
    // Have to be careful with this kind of call (mul method supports
    // "overwriting", but need to investigate whether others are designed that
    // way)
    BIG_384_29_modmul(a->z,a->z,b->z,P); 
}

void zpNeg(Zp* a){
    BIG_384_29_modneg(a->z,a->z,P);
}


int zpNbits(Zp* a){
    return BIG_384_29_nbits(a);
}

int zpParity(Zp* a){
    return BIG_384_29_parity(a);
}


void zpDouble(Zp* a){
    BIG_384_29_norm(a);
    BIG_384_29_fshl(a,1);
}


int zpHalf(Zp* a){
    BIG_384_29_norm(a);
    return BIG_384_29_fshr(a,1);
}

int zpEquals(const Zp* a, const Zp* b){
    BIG_384_29_mod(a->z,P);
    BIG_384_29_mod(b->z,P);
    return BIG_384_29_comp(a->z,b->z)==0;
}

int zpIsZero(const Zp* a){
    BIG_384_29_mod(a->z,P);
    return BIG_384_29_iszilch(a->z);
}

Zp* zpCopy(const Zp* a){
    Zp * r=malloc(sizeof(Zp));
    BIG_384_29_rcopy(r->z,a->z);
    return r;
}

void zpCopyValue(Zp* a, const Zp* b){
    BIG_384_29_rcopy(a->z,b->z);
}

void zpPrint(const Zp *e){
    BIG_384_29_output(e->z);
}

void zpFree(Zp* e){
    free(e);
}
