#ifndef UTILSPSMS_H
#define UTILSPSMS_H

#include <Dpabc_types.h>

/**
 * @brief Hash0 in PSMS scheme
 * 
 * @param m Attributes
 * @param mSize Number of attributes
 * @param z Resulting Zp element
 * @param g Resulting G2 element
 */
void hash0(const Zp *m[], int mSize, Zp ** z, G2 ** g);

/**
 * @brief Hash1 in PSMS scheme
 * 
 * @param pks Public keys
 * @param nkeys Number of public keys
 * @param t Result of hash
 */
void hash1(const publicKey *pks[], int nkeys,Zp *t[]);

/**
 * @brief Hash2 in PSMS scheme
 * 
 * @param m Message signed
 * @param mLength Message size
 * @param pk Public key
 * @param sigma1 Sigma1 from signature
 * @param sigma2 Sigma2 from signature
 * @param g3El G3 element, product/pairing result from scheme
 * @param result Result of hash
 */
void hash2(const char * m, int mLength, const publicKey * pk, const G2 * sigma1, const G2 *sigma2, const G3 * g3El, Zp ** result);

#endif