#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "ERA_agent.h"

#define DEBUG 1
#define HOST "0.0.0.0"
#define PORT 5025

char* eapp = NULL;
char* runtime = NULL;
char* loader = NULL;

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

// ---- Reconfigure ----
int call_ta_reconfigure(unsigned char opcode, unsigned char* data, int data_len) {

    if (DEBUG) {
        // printf("Calling TA for reconfiguration\n");
        // printf("Opcode: %d\n", opcode);
        fflush(stdout);
#if DEBUG
        debug_dump_hex("Input data", data, data_len);
#endif
    }

    // Seleccionar algoritmo según el payload recibido: 0x00 => HMAC_MD5 (9), otro => CMAC_AES (8)
    const char* selected_alg_plain = "CMAC_AES";
    int selected_len = 8;
    if (data_len >= 1) {
        // printf("DEBUG: data[0] = 0x%02X (%u)\n", data[0], data[0]);
    } else {
        // printf("DEBUG: data_len < 1, no data[0]\n");
    }
    if (data_len >= 1 && data[0] == 0x00) {
        selected_alg_plain = "HMAC_MD5";
        selected_len = 9;
    }

    unsigned char* mod_data_buf = (unsigned char*)malloc(selected_len);
    if (!mod_data_buf) {
        fprintf(stderr, "ERROR: failed to allocate mod_data_buf of size %d\n", selected_len);
        fflush(stdout);
        return 0;
    }
    for (int i = 0; i < selected_len; i++) {
        mod_data_buf[i] = (unsigned char)(selected_alg_plain[i] + 1);
    }
    char* mod_data = (char*)mod_data_buf;
    char len_str_buf[4];
    snprintf(len_str_buf, sizeof(len_str_buf), "%d", selected_len);
    char* len_str = len_str_buf;
    if (DEBUG) {
        printf("[HOST] Algorithm selected from payload: %s \n", selected_alg_plain);
#if DEBUG
        // debug_dump_hex("Encoded alg (+1)", (const unsigned char*)mod_data_buf, selected_len);
#endif
    }
    fflush(stdout);

    int32_t result = reconfigure(opcode, (char*)mod_data, (char*)len_str);
    free(mod_data_buf);
    // printf("DEBUG: reconfigure returned: %d\n", result);
    fflush(stdout);

    if (result != 0) {
        printf("Error during reconfiguration\n");
        fflush(stdout);
        return 0;
    } else {
        // printf("DEBUG: call_ta_reconfigure completed successfully\n");
        if (DEBUG) printf("Reconfiguration successful\n");
        fflush(stdout);
        return 1;
    }
}

// ---- Decrypt ----
int call_ta_decrypt(unsigned char* data, int data_len,
                    unsigned char** decrypted_output, int* output_len) {

    // if (DEBUG) printf("Calling TA for decryption\n");
    fflush(stdout);

#if DEBUG
    // debug_dump_hex("Decrypt input", data, data_len);
#endif

    // printf("DEBUG: Allocating mod_data\n");
    unsigned char* mod_data = (unsigned char*)malloc(data_len * 2);
    if (!mod_data) {
        printf("DEBUG: Failed to allocate mod_data\n");
        fflush(stdout);
        return 0;
    }

    int mod_len = 0;
    for (int i = 0; i < data_len; i++) {
        if (data[i] == 0x00) {
            mod_data[mod_len++] = 0xFF;
            mod_data[mod_len++] = 0x01;
        } else if (data[i] == 0xFF) {
            mod_data[mod_len++] = 0xFF;
            mod_data[mod_len++] = 0xFF;
        } else {
            mod_data[mod_len++] = data[i];
        }
    }

    // printf("DEBUG: mod_len = %d\n", mod_len);
    fflush(stdout);

#if DEBUG
    // debug_dump_hex("Escaped data (00->FF 01, FF->FF FF)", mod_data, mod_len);
#endif

    char mod_len_str[16];
    snprintf(mod_len_str, sizeof(mod_len_str), "%d", mod_len);

    decrypt_reconfiguration((char*)mod_data, mod_len_str);

    *decrypted_output = (unsigned char*)malloc(mod_len);
    if (!*decrypted_output) {
        printf("DEBUG: Memory allocation failed for decrypted_output\n");
        free(mod_data);
        fflush(stdout);
        return 0;
    }

    memcpy(*decrypted_output, mod_data, mod_len);
    *output_len = mod_len;

    free(mod_data);

    // printf("DEBUG: call_ta_decrypt completed successfully\n");
    if (DEBUG) {
        printf("[HOST] Decryption successful\n");
        // debug_dump_hex("Decrypted output", *decrypted_output, *output_len);
    }
    fflush(stdout);

    return 1;
}


// ---- Main server ----
int main(int argc, char** argv) {
    printf("[HOST] Starting (Normal World) ...\n");
    // Argumentos de línea de comandos para los binarios
    if (argc < 4) {
      fprintf(stderr, "Uso: %s <eapp> <runtime> <loader> [session]\n", argv[0]);
      return 1;
    }
    eapp = argv[1];
    runtime = argv[2];
    loader = argv[3];

    unsigned char psk[16] = { 0 };		
	unsigned char certificate[32] = { 0 };	
	char * mudUrl = "http://localhost:8091/MUD_Collins_Bootstrapping";

	csp_installPSK(psk);
	csp_installMudURL(mudUrl);
	csp_installCertificate(certificate, 32);
    bsa_sendJoinRequest((char *)"PSK", (char *)"MSK");
    bsa_sendJoinRequest((char *)"MSK", (char *)"EDK_01");
    // Bucle infinito: atender una sola sesión por iteración y retornar
    while (1) {
        run_reconfigure_server();
    }

//     int server_fd, client_fd;
//     struct sockaddr_in server_addr, client_addr;
//     socklen_t client_len = sizeof(client_addr);
//     unsigned char buffer[2048];

//     server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd == -1) {
//         perror("socket");
//         return 1;
//     }
//     if (DEBUG) { printf("Socket created, fd=%d\n", server_fd); fflush(stdout); }

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//     server_addr.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("bind");
//         return 1;
//     }
//     if (DEBUG) { printf("Bind successful on 0.0.0.0:%d\n", PORT); fflush(stdout); }

//     listen(server_fd, 5);
//     printf("Server listening on port %d...\n", PORT);
//     fflush(stdout);

//     while (1) {
//         client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
//         if (client_fd < 0) {
//             perror("accept");
//             continue;
//         }

//         if (DEBUG) {
//             printf("Connection established. fd=%d from %s:%d\n", client_fd,
//                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
//             fflush(stdout);
//         }

//         while (1) {
//             printf("DEBUG: Waiting for data from client\n");
//             int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
//             if (bytes_received <= 0) {
//                 if (DEBUG) { printf("recv returned %d, closing connection\n", bytes_received); fflush(stdout); }
//                 break;
//             }

//             printf("DEBUG: Received %d bytes from client\n", bytes_received);
//             fflush(stdout);
// #if DEBUG
//             debug_dump_hex("Raw received", buffer, bytes_received);
// #endif
//             unsigned char* decrypted_data;
//             int decrypted_len;
//             printf("DEBUG: Calling call_ta_decrypt\n");
//             if (!call_ta_decrypt(buffer, bytes_received, &decrypted_data, &decrypted_len)) {
//                 printf("DEBUG: call_ta_decrypt failed\n");
//                 unsigned char ko[] = {'K', 'O'};
//                 int sent = send(client_fd, ko, 2, 0);
//                 if (DEBUG) { printf("Sent KO (%d bytes)\n", sent); fflush(stdout); }
//                 continue;
//             }

//             printf("DEBUG: call_ta_decrypt succeeded\n");
//             if (DEBUG) {
//                 printf("Received decrypted data (%d bytes):\n", decrypted_len);
//                 for (int i = 0; i < decrypted_len; i++) printf("%02X ", decrypted_data[i]);
//                 printf("\n");
//                 fflush(stdout);
//             }

//             printf("DEBUG: Validating decrypted data\n");
//             if (decrypted_len < 4 || decrypted_data[0] != 0x02 || decrypted_data[1] != 0x02) {
//                 printf("DEBUG: Invalid decrypted data format\n");
//                 unsigned char ko[] = {'K', 'O'};
//                 int sent = send(client_fd, ko, 2, 0);
//                 if (DEBUG) { printf("Sent KO (%d bytes) [invalid format]\n", sent); fflush(stdout); }
//                 free(decrypted_data);
//                 continue;
//             }

//             printf("DEBUG: Processing valid decrypted data\n");
//             int binLength = (decrypted_data[2] << 8) | decrypted_data[3];
//             unsigned char* bin = &decrypted_data[4];
//             if (binLength + 4 > decrypted_len) {
//                 printf("DEBUG: Invalid binLength: %d\n", binLength);
//                 printf("DEBUG: decrypted_len: %d\n", decrypted_len);
//                 unsigned char ko[] = {'K', 'O'};
//                 int sent = send(client_fd, ko, 2, 0);
//                 if (DEBUG) { printf("Sent KO (%d bytes) [binLength overflow]\n", sent); fflush(stdout); }
//                 free(decrypted_data);
//                 continue;
//             }

//             unsigned char opcode = bin[0];
//             unsigned char* payload = &bin[1];
//             printf("DEBUG: Calling call_ta_reconfigure with opcode: %d\n", opcode);
//             fflush(stdout);

//             if (call_ta_reconfigure(opcode, payload, binLength - 1)) {
//                 unsigned char ok[] = {'O', 'K'};
//                 int sent = send(client_fd, ok, 2, 0);
//                 if (DEBUG) { printf("Sent OK (%d bytes)\n", sent); fflush(stdout); }
//             } else {
//                 unsigned char ko[] = {'K', 'O'};
//                 int sent = send(client_fd, ko, 2, 0);
//                 if (DEBUG) { printf("Sent KO (%d bytes) [reconfigure failed]\n", sent); fflush(stdout); }
//             }
//             free(decrypted_data);
//             if (DEBUG) { printf("DEBUG: Finished processing one request\n"); fflush(stdout); }
//         }
//         close(client_fd);
//         if (DEBUG) { printf("Client disconnected.\n"); fflush(stdout); }
//     }

//     close(server_fd);
    return 0;
}
