#include "custom_se_pkcs11.h"
#include <err.h>
#include <setjmp.h>
#include <aceunit.h>

/* Beware:
 *	this file is not compiled directly by
 *	cmake, but with a different makefile
 *	used by aceunit, all include directories
 *	should be specified in that makefile
 */

#include <checksum.h>
#include <network_manager.h>
#include <unistd.h>

extern jmp_buf *AceUnit_env;

#include <bootstrapping_agent.h>

void beforeAll() { csp_initialize(); }

void testBootstrapping() {
	unsigned char psk[16] = { 0 };		
	unsigned char certificate[32] = { 0 };	
	char * mudUrl = "http://localhost:8091/MUD_Collins_Bootstrapping";

	assert(!csp_installPSK(psk));
	assert(!csp_installMudURL(mudUrl));
	assert(!csp_installCertificate(certificate, 32));
	assert(!bsa_sendJoinRequest("PSK", "MSK"));

}

// void testSign() {
// 	unsigned char data[64] = { 0 };
// 	unsigned char hash[16] = { 0 };
// 	assert(!csp_sign("PSK", data, 64, hash));
// }

// void testBootstrappingIncorrecKey() {
// 	unsigned char psk[16] = { 0 };		
// 	unsigned char certificate[32] = { 0 };	
// 	char * mudUrl = "http://13.39.16.149:5000/mud/6578616d706c652e636f6d";
//
// 	psk[0] = 'M';
//
// 	assert(!csp_installPSK(psk));
// 	assert(!csp_installMudURL(mudUrl));
// 	assert(!csp_installCertificate(certificate, 32));
// 	assert(bsa_sendJoinRequest("PSK", "MSK"));
//
// }


void testSecondaryKeyDerivation() {
	// sleep(10);
	assert(!bsa_sendJoinRequest("MSK", "EDK_01"));
	// assert(!bsa_sendJoinRequest("MSK", "EDK_07"));
}
//
// void testSecureComunication() {
//
//
// 	uint16_t crc = 0x0000;
//
// 	int cipherText_size = strlen(mudUrlST) + 16 - (strlen(mudUrlST) % 16);
// 	unsigned char * cipherText = malloc(cipherText_size);
// 	unsigned char * decoded = malloc(cipherText_size);
// 	uint16_t decoded_sz;
//
// 	crc = crc_buypass((unsigned char *)mudUrlST, strlen(mudUrlST));
//
//
// 	printf("url bytes: ");
// 	for (int i = 0; i < strlen(mudUrlST); i++) {
// 		printf("%02x, ", mudUrlST[i]);
// 	}
// 	printf("\n");
// 	printf("Generated CRC: %04x\n", crc);
//
// 	assert(!csp_encryptData("EDK_01", (unsigned char *)mudUrlST, strlen(mudUrlST), cipherText, 0x0003));
//
// 	printf("ciphertext bytes: ");
// 	for (int i = 0; i < cipherText_size; i++) {
// 		printf("%02x, ", cipherText[i]);
// 	}
// 	printf("\n");
//
// 	printf("Cipher text size: %u\n", cipherText_size);
// 	assert(!csp_decryptData("EDK_01", cipherText, cipherText_size, decoded, &decoded_sz, 0x0003));
//
// 	printf("decoded bytes: ");
// 	for (int i = 0; i < decoded_sz; i++) {
// 		printf("%02x, ", decoded[i]);
// 	}
// 	printf("\n");
//
// 	printf("Decoded string: \n%.*s\n", decoded_sz, decoded);
//
// 	assert(!strcmp((char*)decoded, mudUrlST));
// }

// void testRebootstrap() {
// 	
// 	unsigned char psk[16] = { 0 };		
// 	unsigned char certificate[32] = { 0 };	
// 	char * mudUrl = "http://13.39.16.149:5000/mud/6578616d706c652e636f6d";
//
// 	assert(!csp_reconfigure(0x04, NULL)); // Wipe bootstrapping
//
// 	assert(!csp_installPSK(psk));
// 	assert(!csp_installMudURL(mudUrl));
// 	assert(!csp_installCertificate(certificate, 32));
// 	assert(!bsa_sendJoinRequest("PSK", "MSK"));
// }

// void testNetwork() {
//
// 	assert(!send_bytes("192.168.178.63", 6969, mudUrlST, strlen(mudUrlST), 0));
// 	assert(!send_bytes_encrypted_signed("EDK_01", "192.168.178.63", 6969, mudUrlST, strlen(mudUrlST), 0));
//
// }

void afterAll() { csp_terminate(); }
