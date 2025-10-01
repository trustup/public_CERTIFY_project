#ifndef TYPES_H
#define TYPES_H

#include <../lib/Miracl_Core/big_384_29.h>
#include <../lib/Miracl_Core/ecp_BLS12381.h>
#include <../lib/Miracl_Core/ecp2_BLS12381.h>
#include <../lib/Miracl_Core/fp12_BLS12381.h>


struct ZpImpl{
    BIG_384_29 z;
};

struct G1Impl{
    ECP_BLS12381 *p;
};

struct G2Impl{
    ECP2_BLS12381 *p;
};

struct G3Impl{
    FP12_BLS12381 *z;
};

struct ranGen{
    csprng  *rg;
};

#endif