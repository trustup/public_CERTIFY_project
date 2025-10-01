#include <pair.h>
#include "types.h"
#include <../lib/Miracl_Core/pair_BLS12381.h>


G3* pair(const G1 *a,const G2 *b){
    G3* result=malloc(sizeof(G3));
    result->z=malloc(sizeof(FP12_BLS12381));
    PAIR_BLS12381_ate(result->z,b->p,a->p);
    PAIR_BLS12381_fexp(result->z);
    return result;
}

G3* multipair(const G1 *a[], const G2 *b[], int n){
    //Error handling: Check array sizes
    FP12_BLS12381 r[ATE_BITS_BLS12381];
    PAIR_BLS12381_initmp(r);
    for(int i=0;i<n;i++){
        PAIR_BLS12381_another(r,b[i]->p,a[i]->p);
    }
    FP12_BLS12381 *res=malloc(sizeof(FP12_BLS12381));
    PAIR_BLS12381_miller(res,r);
    PAIR_BLS12381_fexp(res);
    G3* result=malloc(sizeof(G3));
    result->z=res;
    return result;
}

G3* doublepair(const G1 *a1,const G1* a2, const G2 *b1,const G2 *b2){
    //Error handling: Check array sizes
    FP12_BLS12381 *res=malloc(sizeof(FP12_BLS12381));
    PAIR_BLS12381_double_ate(res,b1->p,a1->p,b2->p,a2->p);
    PAIR_BLS12381_fexp(res);
    G3* result=malloc(sizeof(G3));
    result->z=res;
    return result;
}