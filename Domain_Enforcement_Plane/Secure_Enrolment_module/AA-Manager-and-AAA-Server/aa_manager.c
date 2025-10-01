#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <uuid/uuid.h>
#include <time.h>
#include <pthread.h>
#include <microhttpd.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SUCCESS 0
#define FAILURE -1
#define CONTINUE 1

#define AAA_SERVER_IP "127.0.0.1"
#define AAA_SERVER_PORT 1813
#define BUFFER_SIZE 1024
#define PORT 8098

int aaa_sock = FAILURE;
SSL *aaa_ssl = NULL;
SSL_CTX *aaa_ssl_ctx = NULL;

#define ID_LEN 17
#define MSK_LEN 64
#define IV_LEN 12
#define TAG_LEN 16 
#define AES_KEYLEN 16

static int notified = 0;

uint8_t psk[AES_KEYLEN] = {0};  // PSK of all zeros
uint8_t first_message[BUFFER_SIZE];
ssize_t first_message_len;

char *ext_auth = NULL;
char *ext_mud = NULL;
char *ext_device = NULL;
char *ext_ip = NULL;
char *ext_port = NULL;

//char uuid_str[37];
char uuid_str[] = "Raspberrypi-1";

unsigned char *decrypted_id;
unsigned char *decrypted_key;

struct MHD_Daemon *global_daemon = NULL;
pthread_mutex_t valid_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t valid_cond = PTHREAD_COND_INITIALIZER;
bool received_valid_post = false;

// Variable para indicar que el servidor HTTP está listo
pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  ready_cond  = PTHREAD_COND_INITIALIZER;
bool server_ready = false;

struct ConnectionInfo {
        char *post_data;
        size_t post_data_size;
};

struct TimingMetrics {
    // Main timing
    struct timespec start_main, end_main, end_init;
    long total_start_ns;
    long total_exec_ns;

    // DDM connection
    struct timespec start_connect_dm, end_connect_dm;
    long connect_latency_dm_ns;

    // Data message to DDM
    struct timespec start_send_dm, end_send_dm;
    long total_msg_dm_ns;

    // AAA server connection
    struct timespec start_connect_aaa, end_connect_aaa;
    long connect_latency_aaa_ns;

    // Forwarded message to AAA
    struct timespec send_msg_aaa, end_msg_aaa;
    long total_smsg_aaa_ns;

    // EAP AA server connect
    struct timespec start_connect_aa, end_connect_aa;
    long connect_latency_aa_ns;

    // Generic message send
    struct timespec start_send_msg, end_send_msg;
    long total_sfirst_msg_ns;
};

struct TimingMetrics timing;

// Function to calculate time difference in nanoseconds
long time_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000L +
           (end.tv_nsec - start.tv_nsec);
}

void print_timing_report(struct TimingMetrics *t) {
    printf("\n===== Timing Report =====\n");

    printf("Initialization time          : %ld ns (%.3f ms)\n", t->total_start_ns, t->total_start_ns / 1e6);

    printf("AA-Device latency: %ld ns (%.3f ms)\n", t->connect_latency_aa_ns, t->connect_latency_aa_ns / 1e6);
    printf("Send message to device: %ld ns (%.3f ms)\n", t->total_sfirst_msg_ns, t->total_sfirst_msg_ns / 1e6);

    printf("AA-AAA latency          : %ld ns (%.3f ms)\n", t->connect_latency_aaa_ns, t->connect_latency_aaa_ns / 1e6);
    printf("Send message to AAA          : %ld ns (%.3f ms)\n", t->total_smsg_aaa_ns, t->total_smsg_aaa_ns / 1e6);

    printf("DDM connect latency          : %ld ns (%.3f ms)\n", t->connect_latency_dm_ns, t->connect_latency_dm_ns / 1e6);
    printf("Send message to DDM          : %ld ns (%.3f ms)\n", t->total_msg_dm_ns, t->total_msg_dm_ns / 1e6);

    printf("Total execution time         : %ld ns (%.3f ms)\n", t->total_exec_ns, t->total_exec_ns / 1e6);

    printf("===========================\n");
}

static enum MHD_Result request_handler(void *cls,
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method,
                                       const char *version,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls) {
    struct ConnectionInfo *con_info = *con_cls;

    if (con_info == NULL) {
        con_info = calloc(1, sizeof(struct ConnectionInfo));
        if (!con_info)
            return MHD_NO;
        *con_cls = con_info;
        return MHD_YES;
    }

    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            char *tmp = realloc(con_info->post_data, con_info->post_data_size + *upload_data_size + 1);
            if (tmp == NULL) {
                free(con_info->post_data);
                free(con_info);
                return MHD_NO;
            }
            con_info->post_data = tmp;
            memcpy(con_info->post_data + con_info->post_data_size, upload_data, *upload_data_size);
            con_info->post_data_size += *upload_data_size;
            con_info->post_data[con_info->post_data_size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            // Attempt to parse JSON (optional, can be removed if even that check is not needed)
            cJSON *json = cJSON_Parse(con_info->post_data);
            if (json) {
                // Signal valid POST received unconditionally
                pthread_mutex_lock(&valid_mutex);
                received_valid_post = true;
                pthread_cond_signal(&valid_cond);
                pthread_mutex_unlock(&valid_mutex);

                cJSON_Delete(json);
            } else {
                // Optionally still signal valid even if JSON is bad
                pthread_mutex_lock(&valid_mutex);
                received_valid_post = true;
                pthread_cond_signal(&valid_cond);
                pthread_mutex_unlock(&valid_mutex);

                printf("AA Server: Warning! Invalid JSON received but processing continues.\n");
            }

            const char *response_str = "OK\n";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_str),
                                                                             (void *)response_str,
                                                                             MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);

            free(con_info->post_data);
            free(con_info);
            *con_cls = NULL;
            return ret;
        }
    }

    return MHD_NO;
}

unsigned char* decrypt_gcm(const unsigned char *key, const unsigned char *input, int input_len, int *out_len) {
    if (input_len < TAG_LEN + IV_LEN) {
        fprintf(stderr, "Input too short for decryption\n");
        return NULL;
    }

    const unsigned char *tag = input;
    const unsigned char *iv = input + TAG_LEN;
    const unsigned char *ciphertext = input + TAG_LEN + IV_LEN;
    int ciphertext_len = input_len - TAG_LEN - IV_LEN;

    EVP_CIPHER_CTX *ctx;
    unsigned char *plaintext = malloc(ciphertext_len);
    int len, plaintext_len;

    if (!plaintext) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);

    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;

    // Set expected tag value for authentication
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, (void*)tag);

    // Finalize decryption: returns 0 if authentication fails
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) <= 0) {
        fprintf(stderr, "Decryption failed: tag mismatch or corrupted data\n");
        free(plaintext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    plaintext_len += len;
    *out_len = plaintext_len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

int wait_for_ddm_post(const char *uuid_to_match) {

    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                              &request_handler, NULL,
                              MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "AA Server: Failed to start HTTP server\n");
        return -1;
    }

    // Start server, wait for post
    pthread_mutex_lock(&ready_mutex);
    server_ready = true;
    pthread_cond_signal(&ready_cond);
    pthread_mutex_unlock(&ready_mutex);

    //printf("AA Server: Server listening on port %d...\n", PORT);

    return 0;
}

void *wait_for_ddm_post_thread(void *arg) {
    const char *uuid_string = (const char *)arg;
    wait_for_ddm_post(uuid_string);
    return NULL;
}


char *to_hex(const unsigned char *in, int inlen) {
    // two hex digits per byte, plus NUL terminator
    char *hex = malloc(inlen * 2 + 1);
    if (!hex) return NULL;
    for (int i = 0; i < inlen; i++) {
        // each byte → two chars
        sprintf(hex + i*2, "%02x", in[i]);
    }
    hex[inlen*2] = '\0';
    return hex;
}

int get_ddm(const char *device, const char *ip_address, const char *mud_url, char *uuid_str, const char *decrypted_key) {
        //printf("AA Server: Generated UUID: %s\n", uuid_str);

        // Create JSON payload
        cJSON *payload = cJSON_CreateObject();
        if (!payload) {
        fprintf(stderr, "AA Server: Failed to create JSON object\n");
        return -1;
        }

        cJSON_AddStringToObject(payload, "uuid", uuid_str);
        cJSON_AddStringToObject(payload, "device", device);
        cJSON_AddStringToObject(payload, "ip_address", ip_address);
        cJSON_AddStringToObject(payload, "mud-url", mud_url);
        cJSON_AddStringToObject(payload, "edk", decrypted_key);

        char *json_string = cJSON_Print(payload);
        if (!json_string) {
        fprintf(stderr, "AA Server: Failed to print JSON object\n");
        cJSON_Delete(payload);
        return -1;
        }

        // Print the JSON payload for debugging
        printf("Payload:\n%s\n", json_string);

        // Set up libcurl
        CURL *curl = curl_easy_init();
        if (!curl) {
        fprintf(stderr, "AA Server: Failed to initialize libcurl\n");
        free(json_string);
        cJSON_Delete(payload);
        return -1;
        }

        CURLcode res;
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:4321/boostrapping");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);

        // Measure TCP connect time
        clock_gettime(CLOCK_MONOTONIC, &timing.start_connect_dm);
       
        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
        fprintf(stderr, "AA Server: Request failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(json_string);
        cJSON_Delete(payload);
        return -1;
        }

        clock_gettime(CLOCK_MONOTONIC, &timing.end_connect_dm);

        timing.connect_latency_dm_ns = time_diff_ns(timing.start_connect_dm, timing.end_connect_dm);

        printf("AA Server: Data sent to DDM\n");

        // Cleanup
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(json_string);
        cJSON_Delete(payload);

        return SUCCESS;
}

// Function to respond with Authentication Finished - Success
int eap_success(const uint8_t *aaa_response_message, size_t recv_len) {
    if (recv_len < 5) {  // Minimum length for EAP-Response/Challenge is 54 bytes (header + rand_s + rand_p + mac_p)
        printf("AA Manager: Message too short to process\n");
        return -1;  // Failure due to short message
    }
    printf("AA Manager: Message received\n");

    // Extract the EAP-Response fields
    uint8_t eap_code = aaa_response_message[0];  // First byte is the EAP code
    uint8_t eap_id = aaa_response_message[1];       // Second byte is the EAP ID
    uint16_t eap_length = (aaa_response_message[2] << 8) | aaa_response_message[3];  // Convert length to host byte order
    uint8_t eap_type = aaa_response_message[4];

    printf("[EAP] Success:\n");
    printf("        - Code: %02x\n", eap_code);  // EAP-Response code
    printf("        - ID: %02x\n", eap_id); // EAP ID
    printf("        - Length: %d\n", eap_length);           // Response length
    printf("        - Type: %02x\n", eap_type);

    const uint8_t *payload = &aaa_response_message[5];

    // Assuming identity is a null-terminated string and msk has fixed length
    char received_identity[ID_LEN + 1];  // or fixed length if known
    memcpy(received_identity, payload, ID_LEN);
    received_identity[ID_LEN] = '\0';
    
    const uint8_t *received_msk = payload + ID_LEN;

    //printf("AA Manager: Received ID: %s\n", received_identity);

    //printf("AA Manager: Received Key: ");
    //for (size_t i = 0; i < MSK_LEN; i++) {
    //    printf("%02x", received_msk[i]);
    //}
    //printf("\n");

    //printf("AA Manager: Successfully processed plain response.\n");

    char *hex_key = to_hex(received_msk, MSK_LEN);
    if (!hex_key) {
        fprintf(stderr, "AA Manager: Out of memory!\n");
        return -1;
    }

    printf("AA Server: Sending data to DDM\n");

    // Start measuring main
    clock_gettime(CLOCK_MONOTONIC, &timing.start_send_dm);

    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, wait_for_ddm_post_thread, uuid_str) != 0) {
        fprintf(stderr, "AA Server: Failed to start wait_for_ddm_post thread\n");
        return FAILURE;
    }

    pthread_detach(server_thread);

    // Waiting to HTTP server
    pthread_mutex_lock(&ready_mutex);
    while (!server_ready) {
        pthread_cond_wait(&ready_cond, &ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);

    // Send data to DDM
    int result_get = get_ddm(ext_device, ext_ip, ext_mud, uuid_str, hex_key);
    if (result_get != 0) {
        fprintf(stderr, "AA Server: Error: Failed to send data to DDM\n");
        return FAILURE;
    }

    clock_gettime(CLOCK_MONOTONIC, &timing.end_send_dm);

    timing.total_msg_dm_ns = time_diff_ns(timing.start_send_dm, timing.end_send_dm);

    // Wait for valid POST
    pthread_mutex_lock(&valid_mutex);
        while (!received_valid_post) {
            pthread_cond_wait(&valid_cond, &valid_mutex);
    }
    pthread_mutex_unlock(&valid_mutex);

    MHD_stop_daemon(global_daemon);
    printf("AA Server: POST received from DDM, process successful!\n");

    return CONTINUE;
}

int connect_to_aaa_server() {
        struct sockaddr_in aaa_server;

        // --- Initialize OpenSSL ---
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        const SSL_METHOD *method = TLS_client_method();
        aaa_ssl_ctx = SSL_CTX_new(method);
        if (!aaa_ssl_ctx) {
            printf("AA Manager: ERROR - Failed to create SSL_CTX\n");
            ERR_print_errors_fp(stderr);
            return FAILURE;
        }

        // Disable certificate verification
        SSL_CTX_set_verify(aaa_ssl_ctx, SSL_VERIFY_NONE, NULL);

        // Crear socket para el servidor AAA
        aaa_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (aaa_sock == -1) {
            printf("AA Manager: ERROR - Could not create socket\n");
            SSL_CTX_free(aaa_ssl_ctx);
            return FAILURE;
        }

        // Configurar dirección del servidor AAA
        aaa_server.sin_family = AF_INET;
        aaa_server.sin_port = htons(AAA_SERVER_PORT);
        if (inet_pton(AF_INET, AAA_SERVER_IP, &aaa_server.sin_addr) <= 0) {
        printf("AA Manager: ERROR - Invalid server address\n");
            close(aaa_sock);
            SSL_CTX_free(aaa_ssl_ctx);
            return FAILURE;
        }

        // Measure TCP connect time
        clock_gettime(CLOCK_MONOTONIC, &timing.start_connect_aaa);
       
        // Intentar conectar al servidor AAA
        if (connect(aaa_sock, (struct sockaddr *)&aaa_server, sizeof(aaa_server)) < 0) {
            printf("AA Manager: ERROR - Connection failed\n");
            close(aaa_sock);
            aaa_sock = FAILURE;
            SSL_CTX_free(aaa_ssl_ctx);
            return FAILURE;
        }

        // --- TLS Connect ---
        aaa_ssl = SSL_new(aaa_ssl_ctx);
        SSL_set_fd(aaa_ssl, aaa_sock);

        if (SSL_connect(aaa_ssl) <= 0) {
            printf("AA Manager: ERROR - TLS handshake failed\n");
            ERR_print_errors_fp(stderr);
            SSL_free(aaa_ssl);
            SSL_CTX_free(aaa_ssl_ctx);
            close(aaa_sock);
            return FAILURE;
        }

        printf("AA Manager: TLS connection established with AAA server %s:%d\n", AAA_SERVER_IP, AAA_SERVER_PORT);

        clock_gettime(CLOCK_MONOTONIC, &timing.end_connect_aaa);
        timing.connect_latency_aaa_ns = time_diff_ns(timing.start_connect_aaa, timing.end_connect_aaa);

        return SUCCESS;
}

int notify_aaa(const char *ext_auth, int aaa_sock) {
        const char *msg = "NULL";
        uint8_t aaa_response_message[BUFFER_SIZE];
        int aaa_recv_len;

        // Decide what to send
        if (strcmp(ext_auth, "eap_msk") == 0) {
        msg = "eap_msk";
        } else {
        msg = "eap_psk";
        }

        // --- 1) Send the message + the null terminator ---
        size_t msg_len = strlen(msg);
        if (SSL_write(aaa_ssl, msg, msg_len + 1) <= 0) {
            printf("AA Manager: ERROR - Failed to send TLS message\n");
            ERR_print_errors_fp(stderr);
            SSL_shutdown(aaa_ssl);
            SSL_free(aaa_ssl);
            SSL_CTX_free(aaa_ssl_ctx);
            close(aaa_sock);
            aaa_sock = FAILURE;
            return FAILURE;
        }
        printf("AA Manager: Authentication message sent (%s), waiting for response...\n", msg);

        // --- 2) Receive response, up to buffer_size - 1 bytes ---
        aaa_recv_len = SSL_read(aaa_ssl, aaa_response_message, sizeof(aaa_response_message) - 1);
        if (aaa_recv_len <= 0) {
            int ssl_err = SSL_get_error(aaa_ssl, aaa_recv_len);
            if (ssl_err == SSL_ERROR_ZERO_RETURN) {
                printf("AA Manager: TLS connection closed by server.\n");
            } else {
                printf("AA Manager: ERROR - SSL_read failed (recv_len: %d, error: %d)\n", aaa_recv_len, ssl_err);
                ERR_print_errors_fp(stderr);  // Optional
            }
            SSL_shutdown(aaa_ssl);
            SSL_free(aaa_ssl);
            SSL_CTX_free(aaa_ssl_ctx);
            close(aaa_sock);
            aaa_sock = FAILURE;
            return FAILURE;
        }

        // --- 3) Null-terminate the response we just received ---
        aaa_response_message[aaa_recv_len] = '\0';

        // Print the response
        printf("AA Manager: Received confirmation from AAA server: '%s'\n", (char *)aaa_response_message);

        // Check if the response is OK
        if (strcmp((char *)aaa_response_message, "OK") == 0) {
            return SUCCESS;
        } else {
            printf("AA Manager: ERROR - AAA server returned error: %s\n", aaa_response_message);
            return FAILURE;
        }
}

int send_to_device(int device_sock, const uint8_t *msg, size_t msg_len) {
    if (send(device_sock, msg, msg_len, 0) < 0) {
        printf("AA Manager: ERROR - Failed to send to client\n");
        return FAILURE;
    }
    return SUCCESS;
}

int send_to_aaa(const uint8_t *eap_response, size_t recv_len, uint8_t *aaa_response_message, ssize_t *aaa_recv_len) {
    if (aaa_sock == FAILURE) {
        if (connect_to_aaa_server() == FAILURE)
            return FAILURE;
    }

    if (!notified) {
        if (notify_aaa(ext_auth, aaa_sock) == FAILURE) {
            printf("AA Manager: ERROR - Could not create socket with AAA\n");
            return FAILURE;
        }
        notified = 1;
    }

    if (SSL_write(aaa_ssl, eap_response, recv_len) <= 0) {
        printf("AA Manager: ERROR - TLS write failed\n");
        ERR_print_errors_fp(stderr);
        SSL_shutdown(aaa_ssl);
        SSL_free(aaa_ssl);
        SSL_CTX_free(aaa_ssl_ctx);
        close(aaa_sock);
        aaa_sock = FAILURE;
        return FAILURE;
    }

    int len = SSL_read(aaa_ssl, aaa_response_message, BUFFER_SIZE);
    if (len <= 0) {
        printf("AA Manager: ERROR - TLS read failed\n");
        ERR_print_errors_fp(stderr);
        SSL_shutdown(aaa_ssl);
        SSL_free(aaa_ssl);
        SSL_CTX_free(aaa_ssl_ctx);
        close(aaa_sock);
        aaa_sock = FAILURE;
        return FAILURE;
    }

    *aaa_recv_len = len;
    
    return SUCCESS;
}


int forward_to_aaa(const uint8_t *eap_response, size_t recv_len, int device_sock) {
    uint8_t buffer[BUFFER_SIZE];
    ssize_t len;
    uint8_t eap_code, eap_id, eap_type;
    uint16_t eap_length;

    // Step 1: Send first EAP-Response to AAA
    eap_code = eap_response[0];
    eap_id = eap_response[1];
    eap_length = (eap_response[2] << 8) | eap_response[3];
    eap_type = (recv_len > 4) ? eap_response[4] : 0;

    printf("AA Manager: Received EAP message from client (Code: %02X, ID: %02X, Length: %u, Type: %02X)\n",
           eap_code, eap_id, eap_length, eap_type);
    printf("AA Manager: Valid EAP-message, forwarding to AAA Server\n");

    // Measure TCP connect time
    clock_gettime(CLOCK_MONOTONIC, &timing.send_msg_aaa);

    if (send_to_aaa(eap_response, recv_len, buffer, &len) == FAILURE) {
        return FAILURE;
    }

    clock_gettime(CLOCK_MONOTONIC, &timing.end_msg_aaa);

    timing.total_smsg_aaa_ns = time_diff_ns(timing.send_msg_aaa, timing.end_msg_aaa);

    while (1) {
        // Step 2/4/6: AAA ➜ Device
        eap_code = buffer[0];
        eap_id = buffer[1];
        //eap_length = (buffer[2] << 8) | buffer[3];
        eap_length = ((uint16_t)buffer[3] << 8) | (uint16_t)buffer[2];
        eap_type = (len > 4) ? buffer[4] : 0;

        // If EAP Success
        if (eap_code == 0x03) {
            printf("AA Manager: Authentication successful\n");

            if (strcmp(ext_auth, "eap_msk") == 0) {
                eap_success(buffer, len);
            }

            if (send(device_sock, buffer, len, 0) < 0) {
                    printf("AA Manager: ERROR - Failed to send success message to client\n");
                    return FAILURE;
            }

            printf("AA Manager: Success message forwarded to client\n");

            // Esperar confirmación del cliente
            len = recv(device_sock, buffer, sizeof(buffer) - 1, 0);
            if (len < 0) {
                  printf("AA Manager: ERROR - Failed to receive response from client\n");
                  return FAILURE;
            }

            buffer[len] = '\0';

            if (strcmp(buffer, "OK") == 0) {
                  printf("AA Manager: Received 'OK' response from client\n");
            } else {
                  printf("AA Manager: ERROR - Unexpected response from client: %s\n", buffer);
                  return FAILURE;
            }

            printf("AA Manager: Authentication finished!\n");
            return SUCCESS;
        }

        printf("AA Manager: Response received from AAA Server, forwarding to client...\n");
        if (send_to_device(device_sock, buffer, len) == FAILURE) {
            return FAILURE;
        }

        // Step 3/5: Client ➜ AAA
        len = recv(device_sock, buffer, sizeof(buffer), 0);
        if (len <= 0) {
            printf("AA Manager: ERROR - Failed to receive response from client\n");
            return FAILURE;
        }

        eap_code = buffer[0];
        eap_id = buffer[1];
        //eap_length = (buffer[2] << 8) | buffer[3];
        eap_length = ((uint16_t)buffer[3] << 8) | (uint16_t)buffer[2];
        eap_type = (len > 4) ? buffer[4] : 0;

        printf("AA Manager: Received EAP message from client (Code: %02X, ID: %02X, Length: %u, Type: %02X)\n",
               eap_code, eap_id, eap_length, eap_type);
        printf("AA Manager: Valid EAP-message, forwarding to AAA Server\n");

        if (send_to_aaa(buffer, len, buffer, &len) == FAILURE) {
            return FAILURE;
        }
    }

    return SUCCESS;
}

// Función para manejar la solicitud EAP
int handle_eap_request(int sock) {

        /* 2) Parse Code, Identifier and Length */
        uint8_t eap_code = first_message[0];
        uint8_t eap_id = first_message[1];
        uint16_t eap_length = (first_message[2] << 8) | first_message[3];
        uint8_t eap_type = first_message[4];

        if (eap_length < 4) {
            fprintf(stderr, "AA Manager: Invalid EAP length field: %u\n", eap_length);
            return -1;
        }

        printf("AA Manager: Received EAP message (Code: %02x, ID: %02x, Length: %u, Type: %02x)\n",
        eap_code, eap_id, eap_length, eap_type);

        // Procesar EAP-Response/Identity (EAP Code: 0x02, EAP Type: 0x01)
        if (eap_code == 0x02) {
        printf("AA Manager: Valid EAP-message received, forwarding to AAA Server\n");
        if (forward_to_aaa(first_message, first_message_len, sock) < 0) {
                printf("AA Manager: ERROR - Failed to forward message to AAA Server\n");
                return FAILURE;
        }
        return CONTINUE;
        }

        // Mensaje EAP no válido
        printf("AA Manager: ERROR - Invalid or unsupported EAP message received (Code: %02x, Type: %02x)\n",
        eap_code, eap_type);
        return FAILURE;
}

// Function to create the EAP request identity
uint8_t* create_eap_request_identity(size_t *eap_request_len) {
        uint8_t eap_code = 0x01;   // EAP-Request code (0x01)
        uint8_t eap_id = 0x01;  // EAP-Identity ID (example: 0x01, increment per request)
        uint8_t eap_type = 0x01;   // EAP-Type: Identity (0x01)
        uint16_t eap_length = 7;  // EAP length: 5 bytes + 2 bytes for MID

        *eap_request_len = eap_length;  // Total length of the payload (EAP length: EAP structure + MID)
        uint8_t *eap_request = (uint8_t *)malloc(*eap_request_len);

        if (!eap_request) {
                printf("Failed to allocate memory for EAP payload\n");
                return NULL;
        }

        // Construct the EAP request
        eap_request[0] = eap_code;
        eap_request[1] = eap_id;
        eap_request[2] = (eap_length >> 8) & 0xFF;              // EAP length high byte
        eap_request[3] = eap_length & 0xFF;             // EAP length low byte
        eap_request[4] = eap_type;

        printf("AA Manager: Construted EAP request identity...\n");

        // Print constructed EAP request
        printf("[EAP] request identity:\n");
        printf("        - Code: %02x\n", eap_code);
        printf("        - ID: %02x\n", eap_id);
        printf("        - Length: %d\n", (eap_length));
        printf("        - Type: %02x\n", eap_type);

        return eap_request;  // Return the constructed request payload
}

// Función para conectar y enviar la solicitud EAP
int connect_and_send_request() {
        int device_sock;
        struct sockaddr_in server_addr;
        struct sockaddr_in local_addr;
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        size_t eap_request_len;

         // Crear socket TCP
        device_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (device_sock == -1) {
            perror("AA Manager: ERROR - Failed to create socket");
            return FAILURE;
        }

        // Definir la dirección del servidor
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(ext_port));
        if (inet_pton(AF_INET, ext_ip, &server_addr.sin_addr) <= 0) {
            perror("AA Manager: ERROR - Invalid server address");
            close(device_sock);
            return FAILURE;
        }

        // Measure TCP connect time
        clock_gettime(CLOCK_MONOTONIC, &timing.start_connect_aa);

        // Conectar con el dispositivo
        if (connect(device_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("AA Manager: ERROR - Connection failed");
            close(device_sock);
            return FAILURE;
        } else {
            printf("AA Manager: Connection established.\n");
        }

        clock_gettime(CLOCK_MONOTONIC, &timing.end_connect_aa);

        timing.connect_latency_aa_ns = time_diff_ns(timing.start_connect_aa, timing.end_connect_aa);

        // Obtener información local (device)
        if (getsockname(device_sock, (struct sockaddr*)&local_addr, &addr_len) == 0 &&
            getpeername(device_sock, (struct sockaddr*)&peer_addr, &addr_len) == 0) {

            char local_ip[INET_ADDRSTRLEN];
            char remote_ip[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip));
            inet_ntop(AF_INET, &peer_addr.sin_addr, remote_ip, sizeof(remote_ip));

            printf("Local  (device): %s:%d\n", local_ip, ntohs(local_addr.sin_port));
            printf("Remote (server): %s:%d\n", remote_ip, ntohs(peer_addr.sin_port));
        } else {
            perror("AA Manager: ERROR - Unable to get socket info");
        }

        // Crear y enviar la solicitud de identidad EAP
        uint8_t* eap_request = create_eap_request_identity(&eap_request_len);

        // Start measuring main
        clock_gettime(CLOCK_MONOTONIC, &timing.start_send_msg);
        ssize_t sent = send(device_sock, eap_request, eap_request_len, 0);

        if (sent < 0) {
            perror("send() failed");
            return -1;
        }
        if ((size_t)sent != eap_request_len) {
            fprintf(stderr, "Partial send: only %zd of %zd bytes sent\n", sent, eap_request_len);
            return -1;
        }

        printf("AA Manager: Sent EAP Request identity to client...\n");

        // Loop until a valid EAP packet is received
        while (1) {
            first_message_len = recv(device_sock, first_message, sizeof(first_message), 0);

            if (first_message_len < 0) {
                perror("AA Manager: recv() failed");
                printf("AA Manager: Received raw EAP packet (%zd bytes)\n", first_message_len);
                return -1;
            }

            printf("AA Manager: Received raw EAP packet (%zd bytes)\n", first_message_len);

            // Print hex dump of the packet
            printf("AA Manager: Packet data (hex): ");
            for (ssize_t i = 0; i < first_message_len; ++i) {
                printf("%02X ", first_message[i]);
            }
            printf("\n");

            // Check for minimum EAP header
            if ((size_t)first_message_len < 4) {
                fprintf(stderr, "AA Manager: Packet too short (%zd bytes), waiting for next...\n", first_message_len);
                printf("AA Manager: Received raw EAP packet (%zd bytes)\n", first_message_len);
                continue;  // Wait for next packet
            }

            // Parse EAP header fields
            uint8_t code = first_message[0];
            uint8_t identifier = first_message[1];
            uint16_t length = (first_message[2] << 8) | first_message[3];
            uint8_t type = (first_message_len > 4) ? first_message[4] : 0;

            // Validate declared length matches received size
            if ((size_t)first_message_len < length) {
                fprintf(stderr, "AA Manager: Declared length (%u) exceeds received size (%zd), waiting for next...\n",
                        length, first_message_len);
                continue;
            }

            // Log parsed EAP info
            printf("AA Manager: Parsed EAP Header — Code: %02X, ID: %02X, Length: %u", code, identifier, length);
            if (first_message_len > 4) {
                printf(", Type: %02X", type);
            }
            printf("\n");

            // Valid message received; break the loop
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &timing.end_send_msg);

        timing.total_sfirst_msg_ns = time_diff_ns(timing.start_send_msg, timing.end_send_msg);

        free(eap_request);
        return device_sock;
}

int main(int argc, char *argv[]) {
        
        // Start measuring main
        clock_gettime(CLOCK_MONOTONIC, &timing.start_main);


        if (argc < 6) {
        fprintf(stderr, "Usage: %s <device> <ip> <mud> <port>\n", argv[0]);
        return FAILURE;
        }

        ext_auth = argv[1];
        ext_device = argv[2];
        ext_ip = argv[3];
        ext_mud = argv[4];
        ext_port = argv[5];

        clock_gettime(CLOCK_MONOTONIC, &timing.end_init);

        timing.total_start_ns = time_diff_ns(timing.start_main, timing.end_init);

        int device_sock = connect_and_send_request();

        if (handle_eap_request(device_sock) == FAILURE) {
                printf("AA Manager: ERROR - Invalid EAP response, closing connection...\n");
                close(device_sock);
                return FAILURE;
        }

        // Cerrar conexión persistente con AAA cuando el programa finalice
        if (aaa_sock != FAILURE) {
        close(aaa_sock);
        }

        clock_gettime(CLOCK_MONOTONIC, &timing.end_main);

        timing.total_exec_ns = time_diff_ns(timing.start_main, timing.end_main);

        print_timing_report(&timing);

        return SUCCESS;
}