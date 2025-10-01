#include <stdio.h>
#include <Zp.h>
#include <g1.h>
#include <g2.h>
#include <g3.h>
#include <pair.h>
#include <time.h>
#include "../Miracl_BLS12381_64b/types.h"


static inline G1* pedersencommit(G1* res, G1* aux, G1* g, G1* h, Zp* x, Zp* z){
    g1CopyValue(res,g);
    g1CopyValue(aux,h);
    g1Mul(res,x);
    g1Mul(aux,z);
    g1Add(res,aux);
    return res;
}

long pedersenBenchmark(int n, ranGen * rng){
    Zp *aux=zpRandom(rng);
    Zp *aux2=zpRandom(rng);
    G1 *g=g1Generator();
    G1 *h=g1Generator();
    g1Mul(g,aux);
    zpRandomValue(rng,aux);
    g1Mul(h,aux);
    clock_t start_time = clock();
    G1 *aux1g=g1Copy(g);
    G1 *aux2h=g1Copy(h);
    for(int i=0;i<n;i++){
        zpRandomValue(rng,aux);
        g1Mul(g,aux);
        zpRandomValue(rng,aux2);
        g1Mul(h,aux2);
        pedersencommit(aux1g,aux2h,g,h,aux,aux2);
    }
    g1Free(g);
    g1Free(h);
    g1Free(aux1g);
    g1Free(aux2h);
    zpFree(aux);
    zpFree(aux2);
    return (long) clock()-start_time;
}

G1* naiveMuln(const G1* a[], const Zp* b[], int n){
    G1 *r=malloc(sizeof(G1));
    r->p=malloc(sizeof(ECP_BLS12381));
    ECP_BLS12381_copy(r->p,a[0]->p);
    ECP_BLS12381_mul(r->p,b[0]->z); 
    ECP_BLS12381 *aux=malloc(sizeof(ECP_BLS12381));
    for(int i=1;i<n;i++){
        ECP_BLS12381_copy(aux,a[i]->p);
        ECP_BLS12381_mul(aux,b[i]->z); 
        ECP_BLS12381_add(r->p,aux);
    }
    return r;
}

void checkMulnTimes(ranGen * rng, int start, int step, int max, int nrepeats){ //To be run with MULNBREAKPOINT 0 
    printf("Running test for start=%d,step=%d,max=%d,nrpeats=%d\n",start,step,max,nrepeats);
    G1 *res1, *res2;
    G1 *auxG1=g1Generator();
    Zp *auxZp=zpFromInt(0);
    G1 **bases;
    Zp **scalars;
    clock_t start_time, end_time;
    for(int n=start;n<max;n+=step){
        bases=malloc(n*sizeof(G1*));
        scalars=malloc(n*sizeof(Zp*));
        for(int i=0;i<n;i++){
            scalars[i]=zpRandom(rng);
            bases[i]=g1Generator();
            zpRandomValue(rng,auxZp);
            g1Mul(bases[i],auxZp);
        }
        start_time = clock();
        for(int j=0;j<nrepeats;j++){
            res1=naiveMuln(bases,scalars,n);
            g1Free(res1);
        }
        end_time=clock();
        printf("NAIVE: Done in %f seconds, for n=%d\n",(double) (end_time-start_time)/ CLOCKS_PER_SEC,n);
        res1=naiveMuln(bases,scalars,n);
        start_time = clock();
        for(int j=0;j<nrepeats;j++){
            res2=g1Muln(bases,scalars,n);
            g1Free(res2);
        }
        end_time=clock();
        printf("MULN: Done in %f seconds, for n=%d\n",(double) (end_time-start_time)/ CLOCKS_PER_SEC,n);
        res2=g1Muln(bases,scalars,n);
        printf("Comparison result %d\n",g1Equals(res1,res2));
        g1Free(res1);
        g1Free(res2);
        for(int i=0;i<n;i++){
            g1Free(bases[i]);
            zpFree(scalars[i]);
        }
        free(bases);
        free(scalars);
    }
    g1Free(auxG1);
    zpFree(auxZp);
}

int main() {
    char bytes[]="RandomBytes";
    ranGen *rng=rgInit(bytes,sizeof(bytes)/sizeof(bytes[0]));
    int n=10000;
    printf("Done in %f seconds\n",(double) pedersenBenchmark(n,rng)/ CLOCKS_PER_SEC);
    //checkMulnTimes(rng,400,1,401,10);
    rgFree(rng);
    return 0;
}
