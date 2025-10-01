#include "include/rfc4764.h"
#include "include/network_manager.h"
#include "network_manager.h"
#include <rfc4764.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <checksum.h>
#include <stdlib.h>
#include <string.h>



static void DBG_print_block(char * block) {
	printf("0x");
	for (uint8_t i = 0; i < 16; i++) printf("%02x ", block[i]);
	printf("\n");
}


static uint32_t generate_auth(char * cmac, char * id_p, size_t id_p_sz, char * id_s, size_t id_s_sz, char * rand_s, char * rand_p, char * psk_id) {

	char * conc;
	char * conc_it;
	size_t conc_sz = id_p_sz + id_s_sz + AES128_BLOCK_SIZE * 2;
	uint32_t res;

	// Compose MAC input

	conc = malloc(conc_sz);
	if (!conc) {
		return OUT_OF_MEMORY;
	}

	conc_it = conc;
	memcpy(conc_it, id_p, id_p_sz);
	conc_it += id_p_sz;

	memcpy(conc_it, id_s, id_s_sz);
	conc_it += id_s_sz;

	memcpy(conc_it, rand_s, AES128_BLOCK_SIZE);
	conc_it += AES128_BLOCK_SIZE;

	memcpy(conc_it, rand_p, AES128_BLOCK_SIZE);
	
	res = csp_sign(psk_id, (unsigned char *)conc, conc_sz, (unsigned char *)cmac);

	return res;
}

static uint32_t verify_server(char * s_cmac, char * id_s, size_t id_s_sz, char * rand_p, char * psk_id) {

	char * conc;
	char * conc_it;
	size_t conc_sz = id_s_sz + AES128_BLOCK_SIZE;
	uint32_t res;
	

	// Compose MAC input

	conc = malloc(conc_sz);
	if (!conc) {
		return OUT_OF_MEMORY;
	}

	conc_it = conc;
	memcpy(conc_it, id_s, id_s_sz);
	conc_it += id_s_sz;

	memcpy(conc_it, rand_p, AES128_BLOCK_SIZE);
	
	char * generated_mac_s = malloc(AES128_BLOCK_SIZE);
	if (!generated_mac_s) {
		free(conc);
		return OUT_OF_MEMORY;
	}
	res = csp_sign(psk_id, (unsigned char *)conc, conc_sz, (unsigned char *)generated_mac_s);
	if (res != SUCCESS) {
		printf("AES CMAC failed : 0x%08x\n", res);
		goto exit;
	}

	// check server cmac with ours
	for (uint8_t i = 0; i < AES128_BLOCK_SIZE; i++) {
		if (generated_mac_s[i] != s_cmac[i]) {
			res = INVALID_SERVER_CMAC;
			break;
		}		
	}

exit:
	free(conc);
	free(generated_mac_s);
	return res;
}

static uint32_t check_header(eap_psk_header header, uint8_t header_id) {

	if (header.code != 1) {
		printf("Incorrect first EAP-PSK message code: 0x%02x\n", header.code);
		return BAD_HEADER_FORMAT;	
	}

	if (header.type != TYPE_EAP_PSK) {
		printf("Incorrect first EAP-PSK message type: 0x%02x\n", header.type);
		return BAD_HEADER_FORMAT;	
	}
	switch (header_id) {
		case 0:
			if (header.flags != FLAG_FIRST_MESSAGE) {
				printf("Incorrect first EAP-PSK message flags: 0x%02x\n", header.flags);
				return BAD_HEADER_FORMAT;	
			}
			break;
		case 1:
			if (header.flags != FLAG_THIRD_MESSAGE) {
				printf("Incorrect first EAP-PSK message flags: 0x%02x\n", header.flags);
				return BAD_HEADER_FORMAT;	
			}
			break;
		default:
			printf("Invalid EAP-MSK header id\n");
			return BAD_HEADER_FORMAT;
	}


	return SUCCESS;
}

static uint8_t * marshal_header(eap_psk_header header) {

	uint8_t * header_bytes = malloc(HEADER_SIZE);

	header_bytes[0] = header.code;
	header_bytes[1] = header.id;
	header_bytes[2] = (htons(header.length) >> 8) & 0xFF;
	header_bytes[3] = htons(header.length) & 0xFF;
	header_bytes[4] = header.type;
	header_bytes[5] = header.flags;

	return header_bytes;

}


static uint32_t send_second_msg(int sockfd, char * rand_s, char * rand_p, char * cmac, char * id_p, size_t id_p_sz, char * s_cmac, char * psk_id, char ** pchannel, size_t * pchannel_sz) {

	size_t rec_size;
	char rec_buff[DEFAULT_BUFFER_SZ];
	third_message * response;
	char * conc;
	char * conc_it;
	char * id_s = S_ID;
	size_t id_s_sz = S_ID_SZ;
	size_t conc_sz = id_s_sz + AES128_BLOCK_SIZE;
	uint32_t res;

	uint16_t second_message_sz = sizeof(eap_psk_header) + AES128_BLOCK_SIZE * 3 + id_p_sz;
	eap_psk_header dummy_header = {.code = 2, 
					.id = 0,
					.length = second_message_sz, 
					.type = TYPE_EAP_PSK,
					.flags = FLAG_SECOND_MESSAGE};

	
	second_message * message = malloc(second_message_sz);

	if (!message) {
		return 1;
	}

	message->header = dummy_header;
	memcpy(message->rand_s, rand_s, AES128_BLOCK_SIZE);
	memcpy(message->rand_p, rand_p, AES128_BLOCK_SIZE);
	memcpy(message->mac_p, cmac, AES128_BLOCK_SIZE);
	memcpy(message->id_p, id_p, id_p_sz);

	// We can't send the struct directly as the compiler
	// may introduce padding between fields

	uint8_t * marshaled_header = marshal_header(message->header);
	uint8_t * marshal_second_message = malloc(second_message_sz - sizeof(eap_psk_header) + HEADER_SIZE);

	uint8_t * msg_idx = marshal_second_message;
	memcpy(msg_idx, marshaled_header, HEADER_SIZE);
	msg_idx += HEADER_SIZE;
	memcpy(msg_idx, message->rand_s, AES128_BLOCK_SIZE);
	msg_idx += AES128_BLOCK_SIZE;
	memcpy(msg_idx, message->rand_p, AES128_BLOCK_SIZE);
	msg_idx += AES128_BLOCK_SIZE;
	memcpy(msg_idx, message->mac_p, AES128_BLOCK_SIZE);
	msg_idx += AES128_BLOCK_SIZE;
	memcpy(msg_idx, message->id_p, id_p_sz);

	res = send_bytes_socket(sockfd, marshal_second_message, second_message_sz - sizeof(eap_psk_header) + HEADER_SIZE);
	if (res != SUCCESS) {
		printf("Error sending identity response\n");
		return res;
	}

	// TODO send message to server and wait for response
	// For now we create our own response
	// uint16_t third_message_sz = sizeof(eap_psk_header) + AES128_BLOCK_SIZE * 2;
	// eap_psk_header dummy_header_res = {.code = 1, 
	// 				.id = 0,
	// 				.length = third_message_sz, 
	// 				.type = TYPE_EAP_PSK,
	// 				.flags = FLAG_THIRD_MESSAGE};
	//
	// 
	// third_message * response = malloc(third_message_sz);
	//
	// if (!response) {
	// 	res = OUT_OF_MEMORY;
	// 	goto exit;
	// }
	//
	// response->header = dummy_header_res;
	// memcpy(response->rand_s, rand_s, AES128_BLOCK_SIZE);

	
	// This should be implemented by getting the credentials from the server
	// following EAP-PSK but for now, for testing pourpuses we'll give ourselves
	// the response
	
	rec_size = DEFAULT_BUFFER_SZ;
	receive_bytes_socket(sockfd, rec_buff, &rec_size);
	response = (third_message*) rec_buff;

	// Compose MAC input

	conc = malloc(conc_sz);
	if (!conc) {
		return OUT_OF_MEMORY;
	}

	conc_it = conc;
	memcpy(conc_it, id_s, id_s_sz);
	conc_it += id_s_sz;

	memcpy(conc_it, rand_p, AES128_BLOCK_SIZE);
	
	res = csp_sign(psk_id, (unsigned char *)conc, conc_sz, (unsigned char *)response->mac_s);
	if (res) {
		printf("AES CMAC failed : 0x%08x\n", res);
		goto exit_error_sign;
	
	}

	if ((res = check_header(response->header, 1)) != SUCCESS) {
		printf("EAP-PSK header format error\n");
		goto exit_error_sign;
	}

	response->header.length = (response->header.length >> 8) | (response->header.length << 8);
	*pchannel_sz = response->header.length - HEADER_SIZE - AES128_BLOCK_SIZE;
	*pchannel = malloc(*pchannel_sz);

	memcpy(s_cmac, response->mac_s, AES128_BLOCK_SIZE);
	memcpy(*pchannel, response->pchannel, *pchannel_sz);

exit_error_sign:
	free(conc);
	free(message);
	return res;

}

static uint32_t send_fourth_msg(int sockfd, char * rand_s, char * pchannel, size_t pchannel_sz) {

	uint32_t res;

	uint16_t fourth_message_sz = sizeof(eap_psk_header) + AES128_BLOCK_SIZE + pchannel_sz;
	eap_psk_header dummy_header = {.code = 2, 
					.id = 0,
					.length = fourth_message_sz, 
					.type = TYPE_EAP_PSK,
					.flags = FLAG_FOURTH_MESSAGE};

	
	fourth_message * message = malloc(fourth_message_sz);

	if (!message) {
		return 1;
	}

	message->header = dummy_header;
	memcpy(message->rand_s, rand_s, AES128_BLOCK_SIZE);
	memcpy(message->pchannel, pchannel, pchannel_sz);

	// We can't send the struct directly as the compiler
	// may introduce padding between fields

	uint8_t * marshaled_header = marshal_header(message->header);
	uint8_t * marshal_fourth_message = malloc(fourth_message_sz - sizeof(eap_psk_header) + HEADER_SIZE);

	uint8_t * msg_idx = marshal_fourth_message;
	memcpy(msg_idx, marshaled_header, HEADER_SIZE);
	msg_idx += HEADER_SIZE;
	memcpy(msg_idx, message->rand_s, AES128_BLOCK_SIZE);
	msg_idx += AES128_BLOCK_SIZE;
	memcpy(msg_idx, message->pchannel, pchannel_sz);

	res = send_bytes_socket(sockfd, marshal_fourth_message, fourth_message_sz - sizeof(eap_psk_header) + HEADER_SIZE);
	if (res != SUCCESS) printf("Error sending fourth message\n");

	free(pchannel);

	return res;
}

static uint32_t initiate_authentication(int sockfd, char ** rand_s, char ** id_s, size_t * id_s_sz) {

	uint32_t res;
        size_t rec_size;
        char * rec_buff;
        first_message * message;

        // Should be generated by server
        // char * id_s_example = S_ID;
        // uint16_t first_message_sz = sizeof(eap_psk_header) + AES128_BLOCK_SIZE + S_ID_SZ;
        // eap_psk_header dummy_header = {.code = 1, 
        //                              .id = 0,
        //                              .length = first_message_sz, 
        //                              .type = TYPE_EAP_PSK,
        //                              .flags = FLAG_FIRST_MESSAGE};
        //
        // first_message * dummy_first = malloc(first_message_sz);
        //
        // dummy_first->header = dummy_header;
        // memcpy(dummy_first->id_s, id_s_example, S_ID_SZ);

        rec_size = DEFAULT_BUFFER_SZ;
        rec_buff = malloc(rec_size);
        res = receive_bytes_socket(sockfd, rec_buff, &rec_size);
        if (res != SUCCESS || !rec_size) {
                printf("Error receiving authentication message\n");
                return INVALID_SERVER_RESPONSE;
        }
	printf("Size of received first EAP message: %zd\n", rec_size);
	printf("Contents of first 16 bytes of received buffer (first EAP message):\n");
	DBG_print_block(rec_buff);
	
	message = (first_message*) rec_buff;
	// Rest is client

	if ((res = check_header(message->header, 0))) {
		printf("EAP-PSK header format error\n");
		goto exit;
	}

	printf("EAP-PSK first message: received header:\n");
	printf("Code: %x\n", message->header.code);
	printf("Id: %x\n", message->header.id);
	printf("Length: %02x\n", message->header.length);
	printf("Type: %x\n", message->header.type);
	printf("Flags: %x\n", message->header.flags);

	*rand_s = malloc(AES128_BLOCK_SIZE);
	if (!*rand_s) {
		res = 1;
		goto exit;
	}

	memcpy(*rand_s, message->rand_s, AES128_BLOCK_SIZE);

	DBG_print_block((char*)&(message->rand_s));

	message->header.length = (message->header.length >> 8) | (message->header.length << 8);
	*id_s_sz = message->header.length - sizeof(eap_psk_header) - AES128_BLOCK_SIZE;
	*id_s = malloc(*id_s_sz);
	if (!id_s) {
		res = 1;
		free(rand_s);
		goto exit;
	}

	memcpy(*id_s, message->id_s, *id_s_sz);

	res = SUCCESS;

exit:
	free(message);
	return res;
}

static uint32_t send_hello(char * url, size_t url_size, int * sockfd, uint8_t key_code) {

	uint32_t res;
	char rec_buff[DEFAULT_BUFFER_SZ] = { 0 };
	size_t rec_size;

	char * hello_message = malloc(4 + url_size);
	hello_message[0] = 0x01; // Payload type (hello message)
	hello_message[1] = key_code; // Bootstrapping type (high end)
	hello_message[2] = htons(url_size) & 0xFF00;
	hello_message[3] = (htons(url_size) >> 8) & 0x00FF; // Url size big endian
	memcpy(hello_message + 4, url, url_size);

	// res = send_bytes("155.54.95.211", 33333, (uint8_t*)hello_message, 4 + url_size, sockfd);
	res = send_bytes("155.54.95.211", 33333, (uint8_t*)hello_message, 4 + url_size, sockfd);
	if (res != SUCCESS) {
		printf("Error sending hello message\n");
		return res;
	}
	
	rec_size = 2; // OK or KO
	res = receive_bytes_socket(*sockfd, rec_buff, &rec_size);
	if (res != SUCCESS || !rec_size) {
		printf("Error receiving hello response\n");
		close(*sockfd);
		if (!rec_size) return INVALID_SERVER_RESPONSE; 
		return res;
	}

	if (strcmp(rec_buff, "OK")) {
		close(*sockfd);
		return INVALID_SERVER_RESPONSE; 
	}

	return SUCCESS;
}

static uint32_t handle_success(int sockfd) {

	uint32_t res;
	char rec_buff[DEFAULT_BUFFER_SZ];
	char snd_buff[2] = "OK";
	size_t rec_size = DEFAULT_BUFFER_SZ;

	res = receive_bytes_socket(sockfd, rec_buff, &rec_size);
	if (res != SUCCESS || !rec_size) {
		printf("Error receiving success message\n");
		if (!rec_size) return INVALID_SERVER_RESPONSE;
		return res;
	}

	uint8_t eap_code = rec_buff[0];  // First byte is the EAP code
	
	if (eap_code != 0x03) {
		res = csp_reconfigure(0x04, NULL, 0); // Wipe MSK if domain bootstrapp fails, its no longer valid
		if (res != SUCCESS) {
			printf("Could not wipe obsolete credentials, WARNING: device is in inconsistent state\n");
			return CATASTROFIC_FAILURE;
		}
		printf("Invalid success message\n");
		return INVALID_SERVER_RESPONSE;
	}

	// Send the response back to the AA Manager
	res = send_bytes_socket(sockfd, (uint8_t*)snd_buff, 2);
	if (res != SUCCESS) {
		printf("Error sending identity response\n");
	}

	return res;
}

static uint32_t handle_id_request(int * sockfd, int * self_socket) {

	uint32_t res;
	char rec_buff[DEFAULT_BUFFER_SZ];
	char snd_buff[DEFAULT_BUFFER_SZ];
	char mac_address[6];
	size_t rec_size = DEFAULT_BUFFER_SZ;

	res = reset_connection(sockfd, self_socket, 4444, rec_buff, &rec_size);
	if (res != SUCCESS || !rec_size) {
		printf("Error receiving id request\n");
		return res;
	}

	uint8_t eap_code = rec_buff[0];  // First byte is the EAP code
	uint8_t eap_id = rec_buff[1];    // Second byte is the EAP ID
	// uint16_t eap_length = (rec_buff[2] << 8) | rec_buff[3];  // Convert length to host byte order
	// uint8_t eap_type = rec_buff[4];  // Fifth byte is the EAP type
	
	if (eap_code != 0x01) {
		printf("Invalid id request\n");
		return INVALID_SERVER_RESPONSE;
	}
	
	uint16_t response_length = 5 + 17; // mac string size (17)
	
	snd_buff[0] = 0x02;  // EAP Code: Access-Challenge
	snd_buff[1] = eap_id;                   // EAP ID
	snd_buff[2] = (response_length >> 8) & 0xFF;    // EAP length high byte
	snd_buff[3] = response_length & 0xFF;           // EAP length low byte
	snd_buff[4] = 0x01;             // EAP Type: PSK

	if ((res = get_mac(mac_address))) {
		printf("Error while retreiving mac address: %04x\n", res);
		return ERROR_GENERIC;
	}
	snprintf(&snd_buff[5], 19,
		     "%02X:%02X:%02X:%02X:%02X:%02X:",
		     mac_address[0], mac_address[1], mac_address[2],
		     mac_address[3], mac_address[4], mac_address[5]);
	// memcpy(&snd_buff[5], P_ID, P_ID_SZ);
	printf("Client: Identity: %s\n", &snd_buff[5]);
	printf("Client: EAP-Response length: %d\n", response_length);

	// Send the response back to the AA Manager
	res = send_bytes_socket(*sockfd, (uint8_t*)snd_buff, response_length);
	if (res != SUCCESS) {
		printf("Error sending identity response\n");
		return res;
	}

	return SUCCESS;
}

uint32_t join_request(char * url, char * psk_id, char * msk_id) {
	
	uint32_t res;
	char rand_p[AES128_BLOCK_SIZE] = { 0 };
	char * id_p = P_ID;
	size_t id_p_sz = P_ID_SZ;
	char * cmac;
	char * pchannel;
	size_t pchannel_sz;
	uint8_t bootstrapp_code;

	int serv_sock;
	int self_sock;
	
	char * id_s;
	size_t id_s_sz;
	char * rand_s;
	char s_cmac[AES128_BLOCK_SIZE] = { 0 };

	if (!strcmp(psk_id, PSK_KEY_ID)) 
		bootstrapp_code = 0x02; // To generate PSK -> MSK we use primary key derivation in server-side
	else 
		bootstrapp_code = 0x03; // To generate MSK -> EDK_N we use secondary key derivation in server-side
	res = send_hello(url, strlen(url), &serv_sock, bootstrapp_code);

	if (res) {
		printf("Failed to send hello message\n");
		return res;
	}

	res = handle_id_request(&serv_sock, &self_sock);

	if (res) {
		printf("Failed handle id request\n");
		goto close_sock_exit;
	}

	res = initiate_authentication(serv_sock, &rand_s, &id_s, &id_s_sz);
	DBG_print_block(rand_s);

	if (res) {
		printf("Failed to initiate authentication with bootstrapping server (0x%08x)\n", res);
		goto close_sock_exit;
	}

	res = csp_generateRandom((unsigned char *)rand_p, AES128_BLOCK_SIZE);
	if (res != SUCCESS) {
		printf("Failed to generate random buffer\n");	
		goto close_sock_exit;
	}

	cmac = malloc(AES128_BLOCK_SIZE);
	if (!cmac) {
		res = OUT_OF_MEMORY;
		goto close_sock_exit;
	}

	res = generate_auth(cmac, id_p, id_p_sz, id_s, id_s_sz, rand_s, rand_p, psk_id);
	if (res) {
		printf("Failed to generate authentication: (0x%08x)\n", res);
		goto close_sock_exit;
	}

	printf("CMAC: ");
	DBG_print_block(cmac);


	res = send_second_msg(serv_sock, rand_s, rand_p, cmac, id_p, id_p_sz, s_cmac, psk_id, &pchannel, &pchannel_sz); 

	if (res) {
		printf("Failed key exchange with bootstrapping server: (0x%08x)\n", res);
		goto close_sock_exit;
	}

	printf("Server CMAC: ");
	DBG_print_block(s_cmac);
	
	res = verify_server(s_cmac, id_s, id_s_sz, rand_p, psk_id);
	if (res) {
		printf("Failed to generate authentication: (0x%08x)\n", res);
		goto close_sock_exit;
	}
	else {
		printf("Server authenticated successfully\n");
	}

	res = csp_deriveKey(psk_id, msk_id, (unsigned char *)rand_p, NULL, 0, AES128);
	if (res != SUCCESS) {
		printf("Error during key derivation\n");
		return res;
	}
	else {
		printf("Successfully derived MSK\n");
	}

	res = send_fourth_msg(serv_sock, rand_s, pchannel, pchannel_sz);
	if (res) {
		printf("Failed to send fourth EAP message (0x%08x)\n", res);
		goto close_sock_exit;
	}
	else {
		printf("Successfully sent fourth EAP message\n");
	}

	res = handle_success(serv_sock);

close_sock_exit:
	close(serv_sock);
	close(self_sock);
	return res;

}
