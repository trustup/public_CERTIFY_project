#include <g3.h>
#include "types.h"
#include <stdio.h>
#include <stdlib.h>


G3* g3One(){
    G3 *r=malloc(sizeof(G3));
    r->z=malloc(sizeof(FP12_BLS12381));
    FP12_BLS12381_one(r->z);
    return r;
}

int g3ByteSize(){
    return 12*MODBYTES_384_29;
}

void g3ToBytes(char *res, const G3 *a){
    octet o={0,12*MODBYTES_384_29,res};
    FP12_BLS12381_toOctet(&o,a->z);
}

void g3Mul(G3* a, const G3* b){
    FP12_BLS12381_mul(a->z,b->z); 
}

void g3Exp(G3* a, const Zp* b){
    // Miracl core's pow does not modify second operand
    FP12_BLS12381_pow(a->z,a->z,b->z); 
}

int g3equals(const G3* a, const G3* b){
    return FP12_BLS12381_equals(a->z,b->z);
}

void g3Print(const G3 *e){
    FP12_BLS12381_output(e->z);
}

void g3Free(G3* e){
    free(e->z);
    free(e);
}