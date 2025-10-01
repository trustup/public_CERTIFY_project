#ifndef STPKCS11_COMMON_H
#define STPKCS11_COMMON_H

#include "st_p11.h"
#include "comclient.h"

#define P11(X) g_pFunctionList->X

extern CK_FUNCTION_LIST_PTR g_pFunctionList;

#include <stdio.h>
//#define PRINT_APP(format, args...)   (void)printf("" format, ## args);
#endif /* STPKCS11_COMMON_H */
