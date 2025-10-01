#ifndef RFC_4764_H
#define RFC_4764_H


#include <custom_se_pkcs11.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <network_manager.h>

#define DEFAULT_BUFFER_SZ		512

#define AES128_KEY_BIT_SIZE		128
#define AES128_KEY_BYTE_SIZE		(AES128_KEY_BIT_SIZE / 8)
#define AES128_BLOCK_SIZE		16

#define P_ID				"usera1"
#define P_ID_SZ				6	

#define S_ID				"aaa-server"
#define S_ID_SZ				10

#define TYPE_EAP_PSK			47	
#define FLAG_FIRST_MESSAGE		0x00
#define FLAG_SECOND_MESSAGE		(0x01 << 6)
#define FLAG_THIRD_MESSAGE		(0x02 << 6)
#define FLAG_FOURTH_MESSAGE		(0x03 << 6)

#define SUCCESS				0x0000
#define OUT_OF_MEMORY			0x0001
#define BAD_HEADER_FORMAT		0x0002
#define INVALID_SERVER_CMAC		0x0003
#define INVALID_SERVER_RESPONSE		0x0004
#define	CATASTROFIC_FAILURE		0x0005
#define ERROR_GENERIC			0xffff

#define HEADER_SIZE			6

#define	PSK_KEY_ID			"PSK"

typedef struct  __attribute__((__packed__)) {
	uint8_t code;
	uint8_t id;
	uint16_t length;
	uint8_t type;
	uint8_t flags;
} eap_psk_header;

typedef struct  __attribute__((__packed__)) {
	eap_psk_header header;
	uint8_t rand_s[16];
	uint8_t id_s[];
} first_message;

typedef struct  __attribute__((__packed__)) {
	eap_psk_header header;
	uint8_t rand_s[16];
	uint8_t rand_p[16];
	uint8_t mac_p[16];
	uint8_t id_p[];
} second_message;

typedef struct __attribute__((__packed__)) {
	eap_psk_header header;
	uint8_t rand_s[16];
	uint8_t mac_s[16];
	uint8_t pchannel[];
} third_message;

typedef struct __attribute__((__packed__)) {
	eap_psk_header header;
	uint8_t rand_s[16];
	uint8_t pchannel[];
} fourth_message;

/** 
 * @brief Performs the first operations of the eap-psk protocol for MSK generation
 * @param url
 * @param psk_id Id of the PSK stored in the TEE
 * @param msk_id Id of the MSK that will be stored in the TEE after the call
 * @retval status of the execution (0 means OK)
 */
uint32_t join_request(char * url, char * psk_id, char * msk_id);

#endif // RFC_4764_H

