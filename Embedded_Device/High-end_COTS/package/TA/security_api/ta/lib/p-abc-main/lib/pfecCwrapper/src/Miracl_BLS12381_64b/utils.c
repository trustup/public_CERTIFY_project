#include <utils.h>
#include "types.h"



ranGen * rgInit(const char *seed,int n){
    ranGen * res=malloc(sizeof(ranGen));
    res->rg=malloc(sizeof(csprng));
    RAND_seed(res->rg,n,seed);
    return res;
}


char * rgGenBytes(ranGen * rg,int n){
    octet O;
    char * res=malloc(n*sizeof(char));
    O.len=n;
    O.max=n;
    O.val=res;
    OCT_rand(&O,rg->rg,n);
    return res;
}


void rgFree(ranGen* rg){
    free(rg->rg);
    free(rg);
}