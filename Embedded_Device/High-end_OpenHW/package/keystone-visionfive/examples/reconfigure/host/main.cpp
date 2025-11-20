//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
// //------------------------------------------------------------------------------
// #include "security_api_interface.h"

// /***
//  * An example call that will be exposed to the enclave application as
//  * an "ocall". This is performed by an edge_wrapper function (below,
//  * print_string_wrapper) and by registering that wrapper with the
//  * enclave object (below, main).
//  ***/

// char* eapp = NULL;
// char* runtime = NULL;
// char* loader = NULL;

// int main(int argc, char** argv) {
//   // Argumentos de línea de comandos para los binarios
//   if (argc < 4) {
//     fprintf(stderr, "Uso: %s <eapp> <runtime> <loader> [session]\n", argv[0]);
//     return 1;
//   }
//   eapp = argv[1];
//   runtime = argv[2];
//   loader = argv[3];

//   // // 1. Enclave para instalar PSK
//   // csp_installPSK(NULL);

//   // // 2. Enclave para derivar clave (PSK)
//   // {
//   //   unsigned char random_buf[32];
//   //   for (int i = 0; i < 32; ++i) random_buf[i] = rand() & 0xFF;
//   //   char baseKeyID[] = "PSK";
//   //   csp_deriveKey((char *)baseKeyID, NULL, random_buf, NULL, 0, 0);
//   // }

//   // // 3. Enclave para derivar clave (MSK)
//   // {
//   //   unsigned char random_buf[32];
//   //   for (int i = 0; i < 32; ++i) random_buf[i] = rand() & 0xFF;
//   //   char baseKeyID[] = "MSK";
//   //   csp_deriveKey((char *)baseKeyID, NULL, random_buf, NULL, 0, 0);
//   // }

//   unsigned char psk[16] = { 0 };		
// 	unsigned char certificate[32] = { 0 };	
// 	char * mudUrl = "http://localhost:8091/MUD_Collins_Bootstrapping";

// 	csp_installPSK(psk);
// 	csp_installMudURL(mudUrl);
// 	csp_installCertificate(certificate, 32);
//   bsa_sendJoinRequest((char *)"PSK", (char *)"MSK");
//   // bsa_sendJoinRequest((char *)"MSK", (char *)"EDK_01");
//   uint8_t opcode = 1;
//   uint16_t algo_len= 4;
//   // csp_reconfigure(opcode, (unsigned char*)"CMAC_AES", 8);
//   // csp_reconfigure(opcode, (unsigned char*)"HMAC_MD5", 9);
//   bsa_sendJoinRequest((char *)"MSK", (char *)"EDK_01");

//   //csp_installCertificate(certificate, 32);
//   //csp_installMudURL(mudUrl);

//   return 0;
// }