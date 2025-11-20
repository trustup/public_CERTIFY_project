#include <ERA_agent.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define AES_256_CBC 1

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef PORT
#define PORT 5025
#endif

#if DEBUG
static void debug_dump_hex(const char* label, const unsigned char* buffer, int length) {
	if (!label) label = "(null)";
	printf("DEBUG: %s (%d bytes): ", label, length);
	for (int i = 0; i < length; i++) {
		printf("%02X ", buffer[i]);
	}
	printf("\n");
	fflush(stdout);
}
#endif

int32_t reconfigure(uint8_t opcode, char * data, char * data_len_str) {

	int32_t res;
	// csp_initialize();
	for (int i = 0; i < atoi(data_len_str); i++) data[i] -= 0x01; // Data is sended +1 to avoid sending zeros as a parameter to a program
	// printf("DEBUG: Entering reconfigure\n");
	printf("[HOST] New algorith = %s\n", data);
	// printf("DEBUG: data_len_str = %s\n", data_len_str);
	// printf("DEBUG: opcode = %d\n", opcode);
	res = csp_reconfigure(opcode, (unsigned char*)data, atoi(data_len_str));

    // res=1;

	// csp_terminate();

	return res;
}

int32_t decrypt_reconfiguration(char * data, char * data_len_str) {

	int32_t res;
	uint16_t data_len = atoi(data_len_str);
	// printf("DEBUG: data_len = %d\n", data_len);
	uint16_t final_data_len = data_len;
	uint16_t decrypted_len = 0;
	
	// printf("DEBUG: Allocating decrypted buffer\n");
	char * decrypted = (char *)malloc(data_len * 2);
	if (!decrypted) {
		printf("DEBUG: Failed to allocate decrypted buffer\n");
		return 0xffff;
	}
	
	// printf("DEBUG: Allocating final_data buffer\n");
	char * final_data = (char *)malloc(data_len);
	if (!final_data) {
		printf("DEBUG: Failed to allocate final_data buffer\n");
		free(decrypted);
		return 0xffff;
	}

	// printf("DEBUG: Starting ERA decoding\n");
	// csp_initialize();

	int j = 0;
	for (int i = 0; i < final_data_len; i++) {
		if (data[j] == 0xFF) {
			if (data[j+1] == 0xFF) final_data[i] = 0xFF;
			else if (data[j+1] == 0x01) final_data[i] = 0x00;
			else {
				printf("Error in ERA decoding\n");
				free(decrypted);
				free(final_data);
				return 0xffff;  // Error generic
			}
			j++;
			final_data_len--;
		}

		else final_data[i] = data[j];

		j++;
	}

	// printf("DEBUG: DECRIPTION_KEY_TAG: %s\n", DECRIPTION_KEY_TAG);
	// printf("DEBUG: data pointer: %p\n", (void*)final_data);
	// printf("DEBUG: data_len: %d\n", final_data_len);
	// printf("DEBUG: decrypted pointer: %p\n", (void*)decrypted);
	res = csp_decryptData(DECRIPTION_KEY_TAG, (unsigned char*)final_data, final_data_len, (unsigned char*)decrypted, &decrypted_len, AES_256_CBC);
	// printf("DEBUG: csp_decryptData returned: %d\n", res);

	// Copiar resultado descifrado de vuelta al buffer de entrada y actualizar longitud
	if (res == 0) {
		memcpy(data, decrypted, decrypted_len);
		snprintf(data_len_str, 6, "%u", (unsigned)decrypted_len);
		printf("[HOST] Copied plaintext back to input buffer");
	}

	// csp_terminate();

	free(decrypted);
	free(final_data);

	return res;

}

int run_reconfigure_server() {

	printf("[HOST] Starting reconfiguration server on port %d\n", PORT);

	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	unsigned char buffer[2048];

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		return 1;
	}
	if (DEBUG) { printf("Socket created, fd=%d\n", server_fd); fflush(stdout); }

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		close(server_fd);
		return 1;
	}
	if (DEBUG) { printf("Bind successful on 0.0.0.0:%d\n", PORT); fflush(stdout); }

	listen(server_fd, 1);
	printf("Server listening on port %d...\n", PORT);
	fflush(stdout);

	client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		perror("accept");
		close(server_fd);
		return 1;
	}

	if (DEBUG) {
		printf("Connection established. fd=%d from %s:%d\n", client_fd,
		       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		fflush(stdout);
	}

	while (1) {
		// printf("DEBUG: Waiting for data from client\n");
		int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
		if (bytes_received <= 0) {
			if (DEBUG) { printf("recv returned %d, closing connection\n", bytes_received); fflush(stdout); }
			break;
		}

		// printf("DEBUG: Received %d bytes from client\n", bytes_received);
		fflush(stdout);
#if DEBUG
		// debug_dump_hex("Raw received", buffer, bytes_received);
#endif
		unsigned char* decrypted_data;
		int decrypted_len;
		// printf("DEBUG: Calling call_ta_decrypt\n");
		if (!call_ta_decrypt(buffer, bytes_received, &decrypted_data, &decrypted_len)) {
			printf("DEBUG: call_ta_decrypt failed\n");
			unsigned char ko[] = {'K', 'O'};
			int sent = send(client_fd, ko, 2, 0);
			if (DEBUG) { printf("Sent KO (%d bytes)\n", sent); fflush(stdout); }
			continue;
		}

		// printf("DEBUG: call_ta_decrypt succeeded\n");
		if (DEBUG) {
			// printf("Received decrypted data (%d bytes):\n", decrypted_len);
			for (int i = 0; i < decrypted_len; i++) printf("%02X ", decrypted_data[i]);
			printf("\n");
			fflush(stdout);
		}

		// printf("DEBUG: Validating decrypted data\n");
		if (decrypted_len < 4 || decrypted_data[0] != 0x02 || decrypted_data[1] != 0x02) {
			printf("DEBUG: Invalid decrypted data format\n");
			unsigned char ko[] = {'K', 'O'};
			int sent = send(client_fd, ko, 2, 0);
			if (DEBUG) { printf("Sent KO (%d bytes) [invalid format]\n", sent); fflush(stdout); }
			free(decrypted_data);
			continue;
		}

		// printf("DEBUG: Processing valid decrypted data\n");
		int binLength = (decrypted_data[2] << 8) | decrypted_data[3];
		unsigned char* bin = &decrypted_data[4];
		if (binLength + 4 > decrypted_len) {
			printf("DEBUG: Invalid binLength: %d\n", binLength);
			printf("DEBUG: decrypted_len: %d\n", decrypted_len);
			unsigned char ko[] = {'K', 'O'};
			int sent = send(client_fd, ko, 2, 0);
			if (DEBUG) { printf("Sent KO (%d bytes) [binLength overflow]\n", sent); fflush(stdout); }
			free(decrypted_data);
			continue;
		}

		unsigned char opcode = bin[0];
		unsigned char* payload = &bin[1];
		// printf("DEBUG: Calling call_ta_reconfigure with opcode: %d\n", opcode);
		fflush(stdout);

		if (call_ta_reconfigure(opcode, payload, binLength - 1)) {
			unsigned char ok[] = {'O', 'K'};
			int sent = send(client_fd, ok, 2, 0);
			if (DEBUG) { printf("Sent OK (%d bytes)\n", sent); fflush(stdout); }
		} else {
			unsigned char ko[] = {'K', 'O'};
			int sent = send(client_fd, ko, 2, 0);
			if (DEBUG) { printf("Sent KO (%d bytes) [reconfigure failed]\n", sent); fflush(stdout); }
		}
		free(decrypted_data);
		// if (DEBUG) { printf("DEBUG: Finished processing one request\n"); fflush(stdout); }
	}
	close(client_fd);
	// if (DEBUG) { printf("Client disconnected.\n"); fflush(stdout); }

	close(server_fd);
	return 0;
}
