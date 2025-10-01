#ifndef TYPESPSMS_H
#define TYPESPSMS_H

#include <Zp.h>
#include <g1.h>
#include <g2.h>
#include <g3.h>
#include <stdint.h>

//We use uint8_t for number of attributes, setting a maximum of 255 
//TODO It would be interesting to configure uint8 or uint16 depending on compilation parameter (e.g. with preprocessor if)

struct secretKeyImpl{
    Zp *x;
    Zp *y_m;
    Zp *y_epoch;
    uint8_t n;   
    Zp *y[];
};

struct publicKeyImpl{
    G1 *vx;
    G1 *vy_m;
    G1 *vy_epoch;
    uint8_t n; 
    G1 *vy[];
};

struct signatureImpl{
    G2 *sigma1;
    G2 *sigma2;
    Zp *mprime;
};

struct zkTokenImpl{
    G2 *sigma1;
    G2 *sigma2;
    Zp *c;
    Zp *v_t;
    Zp *v_mprime;
    uint8_t n; 
    Zp *v_mj[];
};

#endif