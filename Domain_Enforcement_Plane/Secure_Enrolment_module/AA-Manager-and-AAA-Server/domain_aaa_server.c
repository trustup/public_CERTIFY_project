#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/poll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SUCCESS 0
#define FAILURE -1
#define CONTINUE 1

#define AAA_SERVER_IP "127.0.0.1"
#define AAA_SERVER_PORT 1815

// Definitions of EAP (Extensible Authentication Protocol) codes and flags
#define EAP_CODE_ACCESS_CHALLENGE 0x01
#define EAP_TYPE_PSK 0x2f
#define EAP_PSK_1_FLAG 0x00
#define EAP_PSK_2_FLAG (0x01 << 6)
#define EAP_PSK_3_FLAG (0x02 << 6)
#define EAP_PSK_4_FLAG (0x03 << 6)

#define SUCCESS 0
#define FAILURE -1
#define CONTINUE 1

#define BUFFER_SIZE 128

SSL_CTX *ctx = NULL;         // For incoming client connections
SSL_CTX *aaa_ctx = NULL;     // For outgoing AAA server connections
SSL *aaa_ssl = NULL;
int aaa_sock = FAILURE;

struct TimingMetrics {
    // Main timing
    struct timespec start_main, start_execution, end_main, end_init;
    long total_start_ns;
    long total_exec_ns;
    long total_time_ns;
    
    struct timespec start_connect_aaa, end_connect_aaa;
    long connect_latency_aaa_ns;

    struct timespec send_msg_aaa, end_msg_aaa;
    long msg_aaa_ns;

    struct timespec send_msg_aa, end_msg_aa;
    long msg_aa_ns;
};

struct TimingMetrics timing;

// Function to calculate time difference in nanoseconds
long time_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000L +
           (end.tv_nsec - start.tv_nsec);
}

void print_timing_report(struct TimingMetrics *t) {
    printf("\n===== Timing Report =====\n");

    printf("Initialization time           : %ld ns (%.3f ms)\n", t->total_start_ns, t->total_start_ns / 1e6);
    printf("AAA latency                   : %ld ns (%.3f ms)\n", t->connect_latency_aaa_ns, t->connect_latency_aaa_ns / 1e6);
    printf("Send message to AA Manager    : %ld ns (%.3f ms)\n", t->msg_aa_ns, t->msg_aa_ns / 1e6);
    printf("Send message to AAA Server    : %ld ns (%.3f ms)\n", t->msg_aaa_ns, t->msg_aaa_ns / 1e6);
    printf("Total execution time          : %ld ns (%.3f ms)\n", t->total_exec_ns, t->total_exec_ns / 1e6);

    printf("===========================\n");
}

// Function to print a byte array
void print_hex(const char *label, const uint8_t *data, size_t length) {
        printf("%s: ", label);
        for (size_t i = 0; i < length; i++) {
            printf("%02x", data[i]);
        }
        printf("\n");
}

// Function to validate the length of a message
int validate_message_length(size_t recv_len) {
    if (recv_len < 21) {
        printf("AAA Server: Message too short to process\n");
        return FAILURE;
    }
    return SUCCESS;
}

// Function to extract data from the EAP header
void extract_eap_header(const uint8_t *message, uint8_t *eap_code, uint8_t *eap_id, uint16_t *eap_length, uint8_t *eap_type, uint8_t *eap_flags) {
    *eap_code = message[0];
    *eap_id = message[1];
    *eap_length = (message[3] << 8) | message[2];
    *eap_type = message[4];
    *eap_flags = message[5];
}

// Function to validate data extracted from the EAP header
int validate_eap_header(uint16_t eap_length, size_t recv_len, uint8_t eap_type, uint8_t eap_code) {
    if (eap_type != 0x2f || eap_code != 0x02) {
        printf("AAA Server: Invalid EAP message type or code\n");
        return FAILURE;
    }
    return SUCCESS;
}

int connect_to_aaa_server() {
    struct sockaddr_in aaa_server;

    // Create socket
    aaa_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (aaa_sock == -1) {
        printf("AAA Server: ERROR - Could not create socket\n");
        return FAILURE;
    }

    aaa_server.sin_family = AF_INET;
    aaa_server.sin_port = htons(AAA_SERVER_PORT);
    if (inet_pton(AF_INET, AAA_SERVER_IP, &aaa_server.sin_addr) <= 0) {
        printf("AAA Server: ERROR - Invalid server address\n");
        close(aaa_sock);
        return FAILURE;
    }

    // Connect to server
    if (connect(aaa_sock, (struct sockaddr *)&aaa_server, sizeof(aaa_server)) < 0) {
        printf("AAA Server: ERROR - Connection failed\n");
        close(aaa_sock);
        return FAILURE;
    }

    // TLS Setup
    SSL_library_init();
    SSL_load_error_strings();
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        printf("AAA Server: ERROR - Failed to create SSL context\n");
        close(aaa_sock);
        return FAILURE;
    }

    aaa_ssl = SSL_new(ctx);
    if (!aaa_ssl) {
        printf("AAA Server: ERROR - Failed to create SSL object\n");
        SSL_CTX_free(ctx);
        close(aaa_sock);
        return FAILURE;
    }

    SSL_set_fd(aaa_ssl, aaa_sock);

    if (SSL_connect(aaa_ssl) <= 0) {
        printf("AAA Server: ERROR - TLS handshake failed\n");
        ERR_print_errors_fp(stderr);
        SSL_free(aaa_ssl);
        aaa_ssl = NULL;
        SSL_CTX_free(ctx);
        close(aaa_sock);
        return FAILURE;
    }

    printf("AAA Server: TLS connection established \n");

    return SUCCESS;
}

int forward_to_aaa(const uint8_t *eap_response, size_t recv_len, SSL *client_ssl) {
    uint8_t aaa_response_message[BUFFER_SIZE];
    ssize_t aaa_recv_len;
    char buffer[BUFFER_SIZE];
    ssize_t recv_len_response;

    // Measure TCP connect time
    clock_gettime(CLOCK_MONOTONIC, &timing.start_connect_aaa);

    if (!aaa_ssl) {
        if (connect_to_aaa_server() == FAILURE) {
            return FAILURE;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &timing.end_connect_aaa);
    timing.connect_latency_aaa_ns = time_diff_ns(timing.start_connect_aaa, timing.end_connect_aaa);

    clock_gettime(CLOCK_MONOTONIC, &timing.send_msg_aaa);

    // Send message to AAA
    if (SSL_write(aaa_ssl, eap_response, recv_len) <= 0) {
        printf("AAA Server: Failed to send to AAA Server\n");
        ERR_print_errors_fp(stderr);
        SSL_free(aaa_ssl);
        aaa_ssl = NULL;
        return FAILURE;
    }
    printf("AAA Server: Message sent, waiting for response...\n");

    // Receive from AAA
    aaa_recv_len = SSL_read(aaa_ssl, aaa_response_message, sizeof(aaa_response_message));
    if (aaa_recv_len <= 0) {
        printf("AAA Server: ERROR - No response or receive failed, reconnecting...\n");
        ERR_print_errors_fp(stderr);
        SSL_free(aaa_ssl);
        aaa_ssl = NULL;
        return FAILURE;
    }

    printf("AAA Server: Response received from AAA server\n");

    clock_gettime(CLOCK_MONOTONIC, &timing.end_msg_aaa);
    timing.msg_aaa_ns = time_diff_ns(timing.send_msg_aaa, timing.end_msg_aaa);

    uint8_t eap_code = aaa_response_message[0];

    if (eap_code == 0x03) {
        printf("AAA Server: Authentication successful\n");

        // Forward to EAP client
        if (SSL_write(client_ssl, aaa_response_message, aaa_recv_len) <= 0) {
            printf("AAA Server: Failed to send response to EAP client\n");
            ERR_print_errors_fp(stderr);
            return FAILURE;
        }
        printf("AAA Server: Success message forwarded to client\n");
        printf("AAA Server: Authentication finished!\n");

        SSL_free(aaa_ssl);
        aaa_ssl = NULL;

        return SUCCESS;
    } else {

        clock_gettime(CLOCK_MONOTONIC, &timing.send_msg_aa);

        // Forward to EAP client
        if (SSL_write(client_ssl, aaa_response_message, aaa_recv_len) <= 0) {
            printf("AAA Server: Failed to send response to EAP client\n");
            ERR_print_errors_fp(stderr);
            return FAILURE;
        }
        printf("AAA Server: Response forwarded successfully to client\n");

        clock_gettime(CLOCK_MONOTONIC, &timing.end_msg_aa);
        timing.msg_aa_ns = time_diff_ns(timing.send_msg_aa, timing.end_msg_aa);
    }
    return SUCCESS;
}


int handle_key_selection(const char *recv_message, size_t recv_len, SSL *client_ssl) {
    if (recv_message == NULL) {
        printf("AAA Server: handle_key_selection: received NULL pointer!\n");
        return FAILURE;
    }
    printf("AAA Server: Received message: EAP Key Selection\n");
    printf("AAA Server: Valid EAP-message received, forwarding to AAA Server\n");

    if (forward_to_aaa(recv_message, recv_len, client_ssl) != SUCCESS) {
        printf("AAA Server: ERROR - Failed to forward message to AAA Server\n");
        return FAILURE;
    }

    return SUCCESS;
}


int handle_request_tls(SSL *client_ssl) {
    uint8_t recv_message[1024];
    ssize_t recv_len;

    // Set timeout using the underlying socket
    int sock = SSL_get_fd(client_ssl);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // === Step 1: Receive and Validate EAP Identity ===
    recv_len = SSL_read(client_ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: No message received or receive failed (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }
    printf("AAA Server: [EAP] Received EAP message (length: %zd)\n", recv_len);

    // Call the key selection handler
    if (handle_key_selection(recv_message, recv_len, client_ssl) != SUCCESS) {
        printf("AAA Server: Error: failed to handle key selection\n");
        return FAILURE;
    }

    recv_len = SSL_read(client_ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: [EAP] No response received (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }

    uint8_t eap_code = recv_message[0];
    uint8_t eap_type = recv_message[4];
    uint8_t eap_flags = recv_message[5];

    if (eap_code == 0x02 && eap_type == 0x01) {
        printf("AAA Server: [EAP] Received Access Request (EAP-Response/ID)\n");

        printf("AAA Server: Valid EAP-message received, forwarding to AAA Server\n");
        if (forward_to_aaa(recv_message, recv_len, client_ssl) != SUCCESS) {
            printf("AAA Server: ERROR - Failed to forward message to AAA Server\n");
            return FAILURE;
        }

    } else {
        printf("AAA Server: [EAP] Invalid EAP Identity message\n");
        return FAILURE;
    }

    // === Step 2: Receive and Validate EAP PSK2 ===
    recv_len = SSL_read(client_ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: [EAP] No response received for PSK2 (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }

    eap_code = recv_message[0];
    eap_type = recv_message[4];
    eap_flags = recv_message[5];

    if (eap_code == 0x02 && eap_type == 0x2f && eap_flags == 0x40) {
        printf("AAA Server: [EAP] Received Access Challenge Response (EAP PSK 2)\n");

        printf("AAA Server: Valid EAP-message received, forwarding to AAA Server\n");
        if (forward_to_aaa(recv_message, recv_len, client_ssl) != SUCCESS) {
            printf("AAA Server: ERROR - Failed to forward message to AAA Server\n");
            return FAILURE;
        }
    } else {
        printf("AAA Server: [EAP] Invalid response for PSK2 (Type: %02x, Flags: %02x)\n", eap_type, eap_flags);
        return FAILURE;
    }

    // === Step 3: Receive and Validate EAP PSK3 ===
    recv_len = SSL_read(client_ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: [EAP] No response received for PSK3 (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }

    eap_code = recv_message[0];
    eap_type = recv_message[4];
    eap_flags = recv_message[5];

    if (eap_code == 0x02 && eap_type == 0x2f && eap_flags == 0xc0) {
        printf("AAA Server: [EAP] Received Access Challenge Response (EAP PSK 3)\n");

        printf("AAA Server: Valid EAP-message received, forwarding to AAA Server\n");
        if (forward_to_aaa(recv_message, recv_len, client_ssl) != SUCCESS) {
            printf("AAA Server: ERROR - Failed to forward message to AAA Server\n");
            return FAILURE;
        }
    } else {
        printf("AAA Server: [EAP] Invalid response for PSK3 (Type: %02x, Flags: %02x)\n", eap_type, eap_flags);
        return FAILURE;
    }

    printf("AAA Server: Authentication process completed successfully.\n");
    return SUCCESS;
}


int main() {
    // Start measuring main
    clock_gettime(CLOCK_MONOTONIC, &timing.start_main);

    int aaa_server, aa_manager;
    struct sockaddr_in server, aa;
    socklen_t c = sizeof(struct sockaddr_in);

    printf("Domain AAA Server:\n");

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("AAA Server: Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    // Load cert and private key
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    // TLS client context (for backend AAA connection)
    const SSL_METHOD *client_method = TLS_client_method();
    aaa_ctx = SSL_CTX_new(client_method);
    if (!aaa_ctx) {
        fprintf(stderr, "AAA Server: Unable to create SSL client context\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 1;
    }

    // Create listening socket
    aaa_server = socket(AF_INET, SOCK_STREAM, 0);
    if (aaa_server == -1) {
        perror("AAA Server: Could not create listening socket");
        SSL_CTX_free(ctx);
        SSL_CTX_free(aaa_ctx);
        return 1;
    }
    printf("AAA Server: Socket created\n");

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(1813);

    int opt = 1;
    setsockopt(aaa_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(aaa_server, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("AAA Server: Bind failed");
        close(aaa_server);
        SSL_CTX_free(ctx);
        SSL_CTX_free(aaa_ctx);
        return 1;
    }
    printf("AAA Server: Bind done\n");

    if (listen(aaa_server, 3) < 0) {
        perror("AAA Server: Listen failed");
        close(aaa_server);
        SSL_CTX_free(ctx);
        SSL_CTX_free(aaa_ctx);
        return 1;
    }
    printf("AAA Server: Ready, waiting for incoming connections...\n");

    clock_gettime(CLOCK_MONOTONIC, &timing.end_init);
    timing.total_start_ns = time_diff_ns(timing.start_main, timing.end_init);

    struct pollfd pfd;
    pfd.fd = aaa_server;
    pfd.events = POLLIN;

    while (1) {
        int ret = poll(&pfd, 1, 1000);

        if (ret < 0) {
            perror("AAA Server: poll() error");
            break;
        } else if (ret == 0) {
            continue;
        }

        // Start measuring main
        clock_gettime(CLOCK_MONOTONIC, &timing.start_execution);

        aa_manager = accept(aaa_server, (struct sockaddr *)&aa, &c);
        if (aa_manager < 0) {
            perror("AAA Server: Accept connection failed");
            continue;
        }

        SSL *client_ssl = SSL_new(ctx);
        SSL_set_fd(client_ssl, aa_manager);

        if (SSL_accept(client_ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(client_ssl);
            close(aa_manager);
            continue;
        }

        printf("AAA Server: TLS connection established\n");

        while (1) {
            int req_status = handle_request_tls(client_ssl);

            if (req_status == FAILURE) {
                printf("AAA Server: Connection ended due to failure\n");
                break;
            } else if (req_status == SUCCESS) {
                printf("AAA Server: Connection ended successfully\n");
                break;
            }
        }

        SSL_shutdown(client_ssl);
        SSL_free(client_ssl);
        close(aa_manager);

        clock_gettime(CLOCK_MONOTONIC, &timing.end_main);
        timing.total_exec_ns = time_diff_ns(timing.start_execution, timing.end_main);
        timing.total_time_ns = time_diff_ns(timing.start_main, timing.end_main);

        print_timing_report(&timing);

        printf("AAA Server: Waiting for the next connection...\n");
    }

    close(aaa_server);
    SSL_CTX_free(ctx);
    SSL_CTX_free(aaa_ctx);
    EVP_cleanup();
    return 0;
}
