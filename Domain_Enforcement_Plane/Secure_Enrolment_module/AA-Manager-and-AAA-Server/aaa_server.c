#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/cmac.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/core_names.h>
#include <sys/poll.h>

// Definitions of cryptographic parameters and sizes used throughout the protocol
#define KEY_SIZE 16
#define RAND_SIZE 16
#define MAC_SIZE 16
#define PSK_SIZE 32
#define MSK_SIZE 64
#define EMSK_SIZE 64
#define ID_P_MAX_SIZE 64
#define INPUT_SIZE 48
#define MAX_MESSAGE_SIZE 512
#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16
#define IV_SIZE AES_BLOCK_SIZE
#define MAX_ID_S_LEN 966
#define COUNTER_SIZE 16
#define CMAC 1
#define BLOCK_SIZE 16
#define TOTAL_MSK_BLOCKS 4
#define TOTAL_EMSK_BLOCKS 4
#define TOTAL_SIZE (BLOCK_SIZE * TOTAL_BLOCKS)
#define DERIVED_KEY_LEN 16

// Server and Pre-Shared Key (PSK) identifiers and their configurations
#define ID_S "aaa-server"
#define ID_S_LEN strlen(ID_S)

#define MAX_IDENTITY_LEN 256

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

#define NONCE_LEN 12
#define IV_LEN 12
#define TAG_LEN 16    // GCM authentication tag length
#define BUFFER_SIZE 128

// Definitions of cryptographic variables and sizes
unsigned char rand_s[RAND_SIZE], rand_p[RAND_SIZE];
uint8_t ak[KEY_SIZE], kdk[KEY_SIZE], tek[KEY_SIZE], msk[MSK_SIZE], emsk[EMSK_SIZE];

char* result = NULL;
size_t result_len = 0;

// Global SSL context
SSL_CTX *ssl_ctx = NULL;

int encrypted_len;
int final_encrypted_len;
uint8_t decrypted_result_len = 0;
char decrypted_result[MAX_MESSAGE_SIZE];
uint8_t psk[KEY_SIZE] = {0};  // PSK of all zeros

uint8_t *global_key = NULL;
size_t global_key_len;
static int set_msk = 0;

char received_identity[MAX_IDENTITY_LEN];
static int message_count = -1;  // Global or static variable to keep track of the message number

char serialized[BUFFER_SIZE], ciphertext[BUFFER_SIZE], decrypted[BUFFER_SIZE];
char nonce[NONCE_LEN], tag[TAG_LEN];
int serialized_len;

// PCHANNEL Data Structure
typedef struct {
    uint32_t nonce;
    uint8_t result_flag;     // 2 bits
    uint8_t extension_flag;  // 1 bit, set to 0 if no extensions
    uint8_t reserved;        // 5 bits, set to 0
} PChannel;

struct TimingMetrics {
    // Main timing
    struct timespec start_main, end_main, end_init;
    long total_start_ns;
    long total_exec_ns;

    struct timespec start_c_challenge, end_c_challenge;
    long c_challenge_ns;

    struct timespec start_send_c_challenge, end_send_c_challenge;
    long send_c_challenge_ns;

    struct timespec start_key_setup, end_key_setup;
    long key_setup;

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
    printf("Build challenge time          : %ld ns (%.3f ms)\n", t->c_challenge_ns, t->c_challenge_ns / 1e6);
    printf("Send challenge to AA          : %ld ns (%.3f ms)\n", t->send_c_challenge_ns, t->send_c_challenge_ns / 1e6);
    printf("Key setup time                : %ld ns (%.3f ms)\n", t->key_setup, t->key_setup / 1e6);
    printf("Total execution time          : %ld ns (%.3f ms)\n", t->total_exec_ns, t->total_exec_ns / 1e6);

    printf("===========================\n");
}

// Initialize OpenSSL and create context
int init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        ERR_print_errors_fp(stderr);
        return FAILURE;
    }

    // Load cert and key (self-signed for now)
    if (SSL_CTX_use_certificate_file(ssl_ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ssl_ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return FAILURE;
    }

    return SUCCESS;
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

// Function to print the extracted values from a message
void print_extracted_values(uint8_t eap_code, uint8_t eap_id, uint16_t eap_length, uint8_t eap_type, uint8_t eap_flags, const uint8_t *eap_rand_s, const uint8_t *rand_p, const uint8_t *received_mac_p) {
    printf("[EAP] Access-Request - EAP PSK 2:\n");
    printf("        - Code: %02x\n", eap_code);
    printf("        - ID: %02x\n", eap_id);
    printf("        - Length: %d\n", eap_length);
    printf("        - Type: %02x\n", eap_type);
    printf("        - Flags: %02x\n", eap_flags);
    printf("        - Rand_s: ");
    for (int i = 0; i < RAND_SIZE; i++) printf("%02x", eap_rand_s[i]);
    printf("\n");
    printf("        - Rand_p: ");
    for (int i = 0; i < RAND_SIZE; i++) printf("%02x", rand_p[i]);
    printf("\n");
    printf("        - MAC_C: ");
    for (int i = 0; i < MAC_SIZE; i++) printf("%02x", received_mac_p[i]);
}

// Function to validate data extracted from the EAP header
int validate_eap_header(uint16_t eap_length, size_t recv_len, uint8_t eap_type, uint8_t eap_code) {
    if (eap_type != 0x2f || eap_code != 0x02) {
        printf("AAA Server: Invalid EAP message type or code\n");
        return FAILURE;
    }
    return SUCCESS;
}

// Function to generate random value for RAND_S
void generate_random_value(unsigned char *rand_value, size_t size) {
        if (!RAND_bytes(rand_value, size)) {
        fprintf(stderr, "Error generating random bytes\n");
        exit(1);
        }
}

// Function to extract rand_s data from a message
void extract_rand_s(const uint8_t *message, uint8_t *eap_rand_s) {
    memcpy(eap_rand_s, &message[6], RAND_SIZE);
}

// Function to calculate the size of ID_P
size_t calculate_id_p_len(size_t recv_len) {
    return recv_len - (6 + RAND_SIZE + RAND_SIZE + MAC_SIZE);
}

// Function to extract ID_P data from a message
void extract_id_p(const uint8_t *message, char *id_p, size_t id_p_len) {
    memcpy(id_p, &message[6 + RAND_SIZE + RAND_SIZE + MAC_SIZE], id_p_len);
    id_p[id_p_len] = '\0';
}

// Function to extract rand_p and mac_c data from a message
void extract_rand_p_and_mac_p(const uint8_t *message, uint8_t *rand_p, uint8_t *received_mac_c) {
    memcpy(rand_p, &message[6 + RAND_SIZE], RAND_SIZE);
    memcpy(received_mac_c, &message[6 + RAND_SIZE + RAND_SIZE], MAC_SIZE);
}

// Function to derive TEK, MSK and EMSK
int derive_session_keys(const uint8_t *kdk, const uint8_t *rand_p, uint8_t *tek, uint8_t *msk, uint8_t *emsk) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    uint8_t inter_block[BLOCK_SIZE];
    uint8_t inter_block_b[BLOCK_SIZE];
    int len = 0;

    if (!ctx) {
        fprintf(stderr, "Error: Failed to create EVP_CIPHER_CTX\n");
        return FAILURE;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, kdk, NULL) != 1 ||
        EVP_CIPHER_CTX_set_padding(ctx, 0) != 1) {
        fprintf(stderr, "Error: Failed to initialize AES-ECB\n");
        EVP_CIPHER_CTX_free(ctx);
        return FAILURE;
    }

    if (EVP_EncryptUpdate(ctx, inter_block, &len, rand_p, BLOCK_SIZE) != 1 || len != BLOCK_SIZE) {
        fprintf(stderr, "Error: Failed to derive inter_block\n");
        EVP_CIPHER_CTX_free(ctx);
        return FAILURE;
    }

    memcpy(inter_block_b, inter_block, BLOCK_SIZE);
    inter_block_b[BLOCK_SIZE - 1] ^= 0x01;

    if (EVP_EncryptUpdate(ctx, tek, &len, inter_block_b, BLOCK_SIZE) != 1 || len != BLOCK_SIZE) {
        fprintf(stderr, "Error: Failed to derive TEK\n");
        EVP_CIPHER_CTX_free(ctx);
        return FAILURE;
    }

    for (uint8_t i = 0; i < TOTAL_MSK_BLOCKS; i++) {
        memcpy(inter_block_b, inter_block, BLOCK_SIZE);
        inter_block_b[BLOCK_SIZE - 1] ^= (0x02 + i);

        if (EVP_EncryptUpdate(ctx, msk + (i * BLOCK_SIZE), &len, inter_block_b, BLOCK_SIZE) != 1 || len != BLOCK_SIZE) {
            fprintf(stderr, "Error: Failed to derive MSK block %d\n", i);
            EVP_CIPHER_CTX_free(ctx);
            return FAILURE;
        }
    }

    for (uint8_t i = 0; i < TOTAL_EMSK_BLOCKS; i++) {
        memcpy(inter_block_b, inter_block, BLOCK_SIZE);
        inter_block_b[BLOCK_SIZE - 1] ^= (0x06 + i);

        if (EVP_EncryptUpdate(ctx, emsk + (i * BLOCK_SIZE), &len, inter_block_b, BLOCK_SIZE) != 1 || len != BLOCK_SIZE) {
            fprintf(stderr, "Error: Failed to derive EMSK block %d\n", i);
            EVP_CIPHER_CTX_free(ctx);
            return FAILURE;
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

// Function to derive AK o KDK
int derive_key(const uint8_t *global_key, uint8_t *key, uint8_t type) {
    EVP_CIPHER_CTX *ctx = NULL;
    int len = 0;
    uint8_t zero_block[KEY_SIZE] = {0};
    uint8_t intermediate[KEY_SIZE] = {0};
    uint8_t c1[COUNTER_SIZE] = {0};
    uint8_t temp[KEY_SIZE] = {0};

    c1[COUNTER_SIZE - 1] = type;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create EVP_CIPHER_CTX\n");
        return FAILURE;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, global_key, NULL) != 1 ||
        EVP_CIPHER_CTX_set_padding(ctx, 0) != 1 ||
        EVP_EncryptUpdate(ctx, intermediate, &len, zero_block, sizeof(zero_block)) != 1 ||
        len != KEY_SIZE) {
        fprintf(stderr, "Error: Failed to derive inter_block\n");
        EVP_CIPHER_CTX_free(ctx);
        return FAILURE;
    }

    for (int i = 0; i < KEY_SIZE; i++) {
        temp[i] = intermediate[i] ^ c1[i];
    }

    if (EVP_EncryptInit_ex(ctx, NULL, NULL, NULL, NULL) != 1 ||
        EVP_EncryptUpdate(ctx, key, &len, temp, sizeof(temp)) != 1 || len != KEY_SIZE) {
        fprintf(stderr, "Error: Failed to derive key\n");
        EVP_CIPHER_CTX_free(ctx);
        return FAILURE;
    }

    EVP_CIPHER_CTX_free(ctx);
    return SUCCESS;
}

int32_t csp_sign(const unsigned char* ak, unsigned char* dataToSign, uint16_t dataToSignLen, unsigned char* signature) {
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "CMAC", NULL);
    EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string("cipher", "AES-128-CBC", 0),
        OSSL_PARAM_construct_end()
    };
    size_t sig_len;

    if (!ctx || !mac) {
        fprintf(stderr, "Error al inicializar EVP_MAC.\n");
        return FAILURE;
    }

    if (!EVP_MAC_init(ctx, ak, AES_BLOCK_SIZE, params)) {
        fprintf(stderr, "Error en EVP_MAC_init.\n");
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return FAILURE;
    }

    if (!EVP_MAC_update(ctx, dataToSign, dataToSignLen)) {
        fprintf(stderr, "Error en EVP_MAC_update.\n");
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return FAILURE;
    }

    if (!EVP_MAC_final(ctx, signature, &sig_len, AES_BLOCK_SIZE)) {
        fprintf(stderr, "Error en EVP_MAC_final.\n");
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return FAILURE;
    }

    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    return SUCCESS;
}

unsigned char* encrypt_gcm(const unsigned char *key, const unsigned char *data, int data_len, int *out_len) {
    EVP_CIPHER_CTX *ctx;
    unsigned char iv[IV_LEN];
    unsigned char tag[TAG_LEN];
    unsigned char *output;
    int len, ciphertext_len;

    if (!RAND_bytes(iv, IV_LEN)) {
        fprintf(stderr, "IV generation failed\n");
        return NULL;
    }

    output = malloc(TAG_LEN + IV_LEN + data_len);
    if (!output) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);

    EVP_EncryptUpdate(ctx, output + TAG_LEN + IV_LEN, &len, data, data_len);
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, output + TAG_LEN + IV_LEN + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);

    memcpy(output, tag, TAG_LEN);
    memcpy(output + TAG_LEN, iv, IV_LEN);

    *out_len = TAG_LEN + IV_LEN + ciphertext_len;
    EVP_CIPHER_CTX_free(ctx);
    return output;
}

int compute_mac_p(const unsigned char* ak, const char* id_p, const char* id_s,
                  const char* rand_s, const char* rand_p, unsigned char* mac_p) {

    size_t id_p_len = strlen(id_p);
    size_t id_s_len = strlen(id_s);
    size_t conc_sz = id_p_len + id_s_len + AES_BLOCK_SIZE * 2;

    unsigned char* conc = malloc(conc_sz);
    if (!conc) {
        fprintf(stderr, "Error: No se pudo asignar memoria.\n");
        return FAILURE;
    }

    unsigned char* conc_it = conc;
    memcpy(conc_it, id_p, id_p_len); conc_it += id_p_len;
    memcpy(conc_it, id_s, id_s_len); conc_it += id_s_len;
    memcpy(conc_it, rand_s, AES_BLOCK_SIZE); conc_it += AES_BLOCK_SIZE;
    memcpy(conc_it, rand_p, AES_BLOCK_SIZE);

    int result = csp_sign(ak, conc, conc_sz, mac_p);
    free(conc);

    return result;
}

// Function to generate MAC_S using EVP_MAC (CMAC-AES-128)
void compute_mac_s(const uint8_t *ak, const char *id_s, size_t id_s_len, const uint8_t *rand_p, uint8_t *mac_s) {
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "CMAC", NULL);
    EVP_MAC_CTX *mac_ctx = EVP_MAC_CTX_new(mac);
    OSSL_PARAM params[2];
    uint8_t input_data[256];
    size_t input_len = 0;

    // Concatenate ID_S and RAND_P
    memcpy(input_data + input_len, id_s, id_s_len);
    input_len += id_s_len;
    memcpy(input_data + input_len, rand_p, RAND_SIZE);
    input_len += RAND_SIZE;

    // Set up CMAC-AES-128
    params[0] = OSSL_PARAM_construct_utf8_string("cipher", "AES-128-CBC", 0);
    params[1] = OSSL_PARAM_construct_end();
    EVP_MAC_init(mac_ctx, ak, KEY_SIZE, params);

    // Compute the MAC (MAC_S)
    EVP_MAC_update(mac_ctx, input_data, input_len);
    size_t mac_len;
    EVP_MAC_final(mac_ctx, mac_s, &mac_len, MAC_SIZE);

    // Clean up
    EVP_MAC_CTX_free(mac_ctx);
    EVP_MAC_free(mac);
}


// Function to validate received MAC_C
int validate_mac_p(const uint8_t *received_mac_p, const uint8_t *computed_mac_p) {
    if (memcmp(received_mac_p, computed_mac_p, MAC_SIZE) != 0) {
        printf("AAA server: [EAP] MAC_P validation failed\n Authentication failed!\n");
        return -1;
    }
    return 0;
}

// Function to derive KDK
int derive_kdk() {
    if (derive_key(global_key, kdk, 0x02) != 0) {
        fprintf(stderr, "Error: Key derivation failed\n");
        return -1;
    }
    print_hex("        - Derived KDK", kdk, KEY_SIZE);

    return 0;
}

// Function to derive AK
int derive_ak() {
    if (derive_key(global_key, ak, 0x01) != 0) {
        fprintf(stderr, "Error: Key derivation failed\n");
        return -1;
    }
    print_hex("        - Derived AK", ak, KEY_SIZE);

    return 0;
}

int serialize_pchannel(PChannel *pch, unsigned char *buffer) {
    size_t offset = 0;
    memcpy(buffer + offset, &pch->nonce, sizeof(pch->nonce));
    offset += sizeof(pch->nonce);

    uint8_t flags = (pch->result_flag & 0x03) | ((pch->extension_flag & 0x01) << 2);
    buffer[offset++] = flags;
    buffer[offset++] = pch->reserved;

    return offset;
}

int encrypt_pchannel(const unsigned char *plaintext, int plaintext_len,
                     const unsigned char *tek, unsigned char *nonce,
                     unsigned char *ciphertext, unsigned char *tag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, ciphertext_len;

    RAND_bytes(nonce, NONCE_LEN);  // Generate random nonce

    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, tek, nonce);

    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

int decrypt_pchannel(const unsigned char *ciphertext, int ciphertext_len,
                     const unsigned char *tek, const unsigned char *nonce,
                     const unsigned char *tag, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, plaintext_len, ret;

    EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, tek, nonce);

    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, (void *)tag);
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    EVP_CIPHER_CTX_free(ctx);
    return ret > 0 ? plaintext_len + len : -1;
}

int compare_pchannel(const uint8_t *original, const uint8_t *decrypted, size_t len) {
    if (memcmp(original, decrypted, len) == 0) {
        printf("AAA Server: Encrypted and Decrypted Pchannel data are identical.\n");
        printf("AAA Server: Decryption successful.\n");
        return 1;
    } else {
        printf("PCHANNEL mismatch detected!\n");

        printf("Original:  ");
        for (size_t i = 0; i < len; i++) printf("%02x", original[i]);
        printf("\n");

        printf("Decrypted: ");
        for (size_t i = 0; i < len; i++) printf("%02x", decrypted[i]);
        printf("\n");

        return 0;
    }
}

// Function to create an EAP access challenge, corresponding to PKE-Request 1
void construct_access_challenge(uint8_t *response_message, uint8_t eap_id, ssize_t *response_len) {
    const char *id_s = ID_S;
    size_t id_s_len = ID_S_LEN;
    // Generate random challenge value
    generate_random_value(rand_s, RAND_SIZE);
    printf("        - RAND_S generated for PKE-Request\n");

    printf("AAA Server: Constructing EAP Challenge:\n");

    uint16_t length = 5 + 1 + RAND_SIZE + id_s_len;  // Calculate length: header + flags + rand_s + id_s

    // EAP-PSK-1 construction
    response_message[0] = EAP_CODE_ACCESS_CHALLENGE;  // EAP Code: Access-Challenge
    response_message[1] = eap_id;                   // EAP ID
    response_message[2] = (length >> 8) & 0xFF;     // EAP length high byte
    response_message[3] = length & 0xFF;            // EAP length low byte
    response_message[4] = EAP_TYPE_PSK;             // EAP Type: PSK

    // Set the flags field to indicate EAP-PSK-1
    response_message[5] = EAP_PSK_1_FLAG;

    // Add rand_s (challenge random value)
    memcpy(&response_message[6], rand_s, RAND_SIZE);

    // Add id_s (server identity)
    memcpy(&response_message[6 + RAND_SIZE], id_s, id_s_len);

    // Set the total response length
    *response_len = 6 + RAND_SIZE + id_s_len;

    // Log the entire EAP Access-Challenge message in a block
    printf("[EAP] Access-Challenge - EAP PSK 1:\n");
    printf("        - Code: %02x\n", response_message[0]);  // EAP Code
    printf("        - ID: %02x\n", response_message[1]);    // EAP ID
    printf("        - Length: %d\n", (response_message[2] << 8) | response_message[3]);  // Length
    printf("        - Type: %02x\n", response_message[4]);  // EAP Type (PSK)
    printf("        - Flags: %02x\n", response_message[5]); // Flags

    // Print rand_s
    printf("        - Rand_s: ");
    for (int i = 0; i < RAND_SIZE; i++) {
        printf("%02x", response_message[6 + i]);
    }
    printf("\n");

    // Print id_s (server identity)
    printf("        - ID_s: %s\n", id_s);
}

// Function to create a response to identity request
void construct_response(uint8_t *response_message, const char *response_text, uint8_t response_code, ssize_t *response_len) {
    response_message[0] = response_code;  // Set the response code (e.g., success or failure)

    size_t response_text_len = strlen(response_text);

    // Copy the response text (e.g., "Access-Accept" or "Access-Reject") into the message
    memcpy(&response_message[1], response_text, response_text_len);

    // Set the total response length (response code + response text)
    *response_len = 1 + response_text_len;
}

// Function to process and validate the message about EAP identity
int validate_eap_identity(const uint8_t *request_message, size_t recv_len, SSL *ssl) {
    // Step 1: Validate the EAP-Response/Identity
    if (recv_len < 5) {  // Minimum length for EAP-Response/Identity is 5 bytes
    printf("AAA Server: [EAP] Message too short to process\n");
    return FAILURE;  // Failure due to short message
    }

    uint8_t eap_code = request_message[0];  // First byte is the EAP code
    uint8_t eap_id = request_message[1];    // Second byte is the EAP ID
    uint16_t eap_length = (request_message[2] << 8) | request_message[3];  // Convert length to host byte order
    uint8_t eap_type = request_message[4];  // Fifth byte is the EAP type

    // Log with specified format
    printf("[EAP] Access-Request:\n");
    printf("        - Code: %02x\n", eap_code);
    printf("        - ID: %02x\n", eap_id);
    printf("        - Length: %d\n", eap_length);
    printf("        - Type: %02x\n", eap_type);

    // Extract the identity (starting from byte 5 onward)
    size_t identity_len = eap_length - 5;  // The remaining bytes after the 4-byte EAP header
    
    // Ensure we do not exceed the size of our global buffer (reserve one byte for the null terminator)
    if (identity_len > MAX_IDENTITY_LEN - 1) {
        identity_len = MAX_IDENTITY_LEN - 1;  // Truncate if necessary
    }

    // Copy the identity into the global variable
    memcpy(received_identity, &request_message[5], identity_len);
    received_identity[identity_len] = '\0';  // Null-terminate

    printf("AAA Server: [EAP] Identity received: %s\n", received_identity);

    // Validate the received identity against the expected value
    //const char* expected_identity = "52:54:00:12:34:56";
    const char* expected_identity = "Raspberrypi-1";
    uint8_t response_message[1024];
    ssize_t response_len = 0;

    // Send an Access-Challenge (EAP-PSK-1)
    if (strcmp(received_identity, expected_identity) == 0) {
        printf("AAA Server: [EAP] Identity validation successful\n");

        clock_gettime(CLOCK_MONOTONIC, &timing.start_c_challenge);

        construct_access_challenge(response_message, eap_id, &response_len);

        clock_gettime(CLOCK_MONOTONIC, &timing.end_c_challenge);
        timing.c_challenge_ns = time_diff_ns(timing.start_c_challenge, timing.end_c_challenge);

    } else {
        printf("AAA Server: [EAP] Invalid identity received. User '%s' is not in the list\n", received_identity);
        construct_response(response_message, "Access-Reject", 0x04, &response_len);  // Failure response
    }

    clock_gettime(CLOCK_MONOTONIC, &timing.start_send_c_challenge);

    // Send the response back
    int sent_len = SSL_write(ssl, response_message, response_len);
    if (sent_len != response_len) {
        fprintf(stderr, "AAA Server: Error: Failed to send EAP message via TLS\n");
        ERR_print_errors_fp(stderr);  // Optional: for debug
        return FAILURE;
    }

    clock_gettime(CLOCK_MONOTONIC, &timing.end_send_c_challenge);
    timing.send_c_challenge_ns = time_diff_ns(timing.start_send_c_challenge, timing.end_send_c_challenge);

    printf("AAA Server: [EAP] Access Challenge (EAP PSK 1) sent successfully (length: %d)\n", sent_len);
    return SUCCESS;  // Success
}

// Function to create and send a response for EAP third message
int send_eap_psk3(SSL *ssl, uint8_t eap_id, const uint8_t *rand_s, const uint8_t *mac_s, const uint8_t *pchannel_data, size_t pchannel_data_len) {
    uint8_t response_message[MAX_MESSAGE_SIZE];
    uint16_t message_len;

    // Calculate total message length
    message_len = 5 + 1 + RAND_SIZE + MAC_SIZE + pchannel_data_len;

    // EAP Header
    response_message[0] = 0x01; // EAP Code (Response/Success)
    response_message[1] = eap_id; // EAP ID (same as request ID)
    response_message[2] = (message_len >> 8) & 0xFF; // Length (high byte)
    response_message[3] = message_len & 0xFF; // Length (low byte)
    response_message[4] = 0x2f; // EAP Type (EAP-PSK)

    // EAP Flags (set as needed, here we set to 0 for simplicity)
    response_message[5] = EAP_PSK_3_FLAG;

    // Copy RAND_S (16 bytes), MAC_S (16 bytes) and Encrypted PChannel Data
    memcpy(&response_message[6], rand_s, RAND_SIZE);
    memcpy(&response_message[6 + RAND_SIZE], mac_s, MAC_SIZE);
    memcpy(&response_message[6 + RAND_SIZE + MAC_SIZE], pchannel_data, pchannel_data_len);

    // Print the full EAP-Response message (LOG)
    //print_hex("AAA Server: Full EAP-Response Message", response_message, message_len);
    printf("AAA Server: [EAP] Sending message of Length: %d\n", message_len);

    // Send the message to the client
    int sent_len = SSL_write(ssl, response_message, message_len);
    if (sent_len != message_len) {
        fprintf(stderr, "AAA Server: Error: Failed to send EAP-Response/Success message via TLS\n");
        ERR_print_errors_fp(stderr);  // Optional: for debug
        return FAILURE;
    }

    printf("AAA Server: [EAP] Response/Success message sent to client\n");
    return SUCCESS;
}

// Function to create and send a response for EAP success
int send_eap_success(SSL *ssl, uint8_t eap_id) {

   size_t identity_len = strlen(received_identity);
   uint16_t response_length = 5 + identity_len + sizeof(msk);

    printf("AAA Server: Sending plain identity: %s\n", received_identity);
    printf("AAA Server: Sending plain MSK:\n");
    for (size_t i = 0; i < sizeof(msk); i++) {
        printf("%02x", msk[i]);
    }
    printf("\n");

   // Allocate memory for the response
   uint8_t *response_message = (uint8_t *)malloc(response_length);
   if (!response_message) {
    printf("AAA Server: Failed to allocate memory for EAP response\n");
    return -1;
   }

   printf("AAA Server: Constructing EAP Message:\n");
   // Construct the EAP-Success packet
   response_message[0] = 0x03;  // EAP-Success code
   response_message[1] = eap_id;  // Use the same ID as received
   response_message[2] = (response_length >> 8) & 0xFF;  // Length high byte
   response_message[3] = response_length & 0xFF;  // Length low byte
   response_message[4] = 0x2f; // EAP Type (EAP-PSK)

   memcpy(&response_message[5], received_identity, identity_len);
   memcpy(&response_message[5 + identity_len], msk, sizeof(msk));

   printf("[EAP] Access-Success:\n");
   printf("        - Code: %02x\n", response_message[0]);
   printf("        - ID: %02x\n", response_message[1]);
   printf("        - Length: %d\n", response_length);
   printf("        - Type: %02x\n", response_message[4]);

    // Send the message to the client
    int sent_len = SSL_write(ssl, response_message, response_length);
    if (sent_len != response_length) {
        fprintf(stderr, "AAA Server: Error: Failed to send EAP Success message via TLS\n");
        ERR_print_errors_fp(stderr);  // Optional: for debug
        return FAILURE;
    }

   printf("AAA Server: [EAP] Access Accept sent successfully (length: %d)\n", response_length);
   free(response_message);
   return SUCCESS;
}

// Function to process and validate EAP PSK second message
int validate_eap_psk2(const uint8_t *request_message, size_t recv_len, SSL *ssl) {

    if (validate_message_length(recv_len) != 0) return FAILURE;

    printf("AAA Server: [EAP] Received message (length: %zd)\n", recv_len);

    // Step 2: Extract EAP header information
    uint8_t eap_code, eap_id, eap_type, eap_flags;
    uint16_t eap_length;
    extract_eap_header(request_message, &eap_code, &eap_id, &eap_length, &eap_type, &eap_flags);

    // Step 3: Validate EAP length and type
    if (validate_eap_header(eap_length, recv_len, eap_type, eap_code) != 0) return -1;

    // Step 4: Extract RAND_S
    uint8_t eap_rand_s[RAND_SIZE];
    extract_rand_s(request_message, eap_rand_s);

    // Step 5: Extract RAND_P and MAC_P
    uint8_t received_mac_p[MAC_SIZE];
    extract_rand_p_and_mac_p(request_message, rand_p, received_mac_p);

    // Step 6: Print extracted values
    print_extracted_values(eap_code, eap_id, eap_length, eap_type, eap_flags, eap_rand_s, rand_p, received_mac_p);
    printf("\n");

    // Extract ID_P
    size_t id_p_len = calculate_id_p_len(recv_len);
    char id_p[id_p_len];

    extract_id_p(request_message, id_p, id_p_len);
    printf("        - ID_p: %.*s\n", (int)id_p_len, id_p);

    printf("AAA Server: [EAP] Checking integrity of MAC_P...\n");

    clock_gettime(CLOCK_MONOTONIC, &timing.start_key_setup);

    // Derive AK and compute MAC_P
    if (derive_ak() != 0) return FAILURE;

    unsigned char mac_p[MAC_SIZE];

    if (compute_mac_p(ak, id_p, ID_S, (char*)rand_s, (char*)rand_p, mac_p) == SUCCESS) {
        printf("AAA Server: [EAP] Computed MAC_P: ");
        for (int i = 0; i < AES_BLOCK_SIZE; i++) {
            printf("%02x", mac_p[i]);
        }
        printf("\n");
    } else {
        fprintf(stderr, "AAA Server: Error to computed MAC_P.\n");
        return FAILURE;
    }

    // Validate MAC_P against received_mac_p
    if (validate_mac_p(received_mac_p, mac_p) != 0) return FAILURE;
    printf("        - MAC_P validation successful\n");

    // Step 10: Derive keys
    printf("AAA Server: [EAP] Starting first Key derivation\n");

    if (derive_kdk() != 0) return FAILURE;

    if (derive_session_keys(kdk, rand_p, tek, msk, emsk) != 0) {
        fprintf(stderr, "AAA Server: Error: Failed to derive session keys\n");
        return FAILURE;
    }

    print_hex("        - Derived TEK", tek, KEY_SIZE);

    const char *id_s = ID_S;
    uint8_t id_s_len = strlen((char *)id_s); // Assuming `id_s` is null-terminated
    uint8_t computed_mac_s[MAC_SIZE];

    //print_hex("AAA Server: ID_S Generated", id_s, id_s_len);
    compute_mac_s(ak, id_s, id_s_len, rand_p, computed_mac_s);

    print_hex("AAA Server: Computed MAC_S", computed_mac_s, MAC_SIZE);

    clock_gettime(CLOCK_MONOTONIC, &timing.end_key_setup);
    timing.key_setup = time_diff_ns(timing.start_key_setup, timing.end_key_setup);

    // Standard PCHANNEL without extensions
    PChannel pch = {
        .nonce = 12345,
        .result_flag = 0x01,     // Success
        .extension_flag = 0,     // No extension
        .reserved = 0
    };

    // Serialize the PCHANNEL data
    serialized_len = serialize_pchannel(&pch, serialized);

    // Print plaintext PCHANNEL (for comparison after decryption)
    printf("AAA Server: [EAP] Serialized (plaintext) PCHANNEL: ");
    for (int i = 0; i < serialized_len; i++) printf("%02x", serialized[i]);
    printf("\n");

    // Encrypt the serialized data
    encrypted_len = encrypt_pchannel(serialized, serialized_len, tek, nonce, ciphertext, tag);

    // Check encryption success
    if (encrypted_len < 0) {
        printf("AAA Server: Encryption failed.\n");
        return -1;
    }

    // Prepare full encrypted PCHANNEL (nonce + ciphertext + tag)
    unsigned char encrypted_pchannel_data[NONCE_LEN + encrypted_len + TAG_LEN];
    memcpy(encrypted_pchannel_data, nonce, NONCE_LEN);
    memcpy(encrypted_pchannel_data + NONCE_LEN, ciphertext, encrypted_len);
    memcpy(encrypted_pchannel_data + NONCE_LEN + encrypted_len, tag, TAG_LEN);

    // Final encrypted length
    final_encrypted_len = NONCE_LEN + encrypted_len + TAG_LEN;

    printf("AAA Server: [EAP] Encrypted PCHANNEL: ");
    for (int i = 0; i < final_encrypted_len; i++) printf("%02x", encrypted_pchannel_data[i]);
    printf("\n");

    // Step 6: Send the encrypted PChannel data using send_eap_psk3
    int result_k_encr = send_eap_psk3(ssl, eap_id, rand_s, computed_mac_s, encrypted_pchannel_data, final_encrypted_len);
    if (result_k_encr != 0) {
        fprintf(stderr, "AAA Server: Error: Failed to send EAP-Response (PKE-3)\n");
        return FAILURE;
    }

    return SUCCESS;
}

// Function to process and validate EAP PSK third message
int validate_eap_psk3(const uint8_t *recv_message, size_t recv_len, SSL *ssl) {

    if (recv_len < 31) {  // Minimum length for EAP-Response/Challenge is 54 bytes (header + rand_s + rand_p + mac_p)
        printf("AAA Server: [EAP] Message too short to process\n");
        return -1;  // Failure due to short message
    }

    // Extract the EAP-Response fields
    uint8_t eap_code = recv_message[0];  // First byte is the EAP code
    uint8_t eap_id = recv_message[1];       // Second byte is the EAP ID
    //uint16_t eap_length = (recv_message[3] << 8) | recv_message[2];
    uint16_t eap_length = (recv_message[3] << 8) | recv_message[2];
    uint8_t eap_type = recv_message[4];  // Fifth byte is the EAP type

    if (eap_type != 0x2f || eap_code != 0x02) {
        printf("AAA Server: Invalid EAP message type or code\n");
        return -1;
    }
    printf("Raw length bytes: %02x %02x\n", recv_message[2], recv_message[3]);

    // Extract rand_s, rand_p, and mac_p from the message
    uint8_t eap_flags = recv_message[5];  // Flags at byte 5
    uint8_t eap_rand_s[16];

    memcpy(eap_rand_s, &recv_message[6], 16);  // Extract rand_s

    printf("[EAP] Access-Request - EAP PSK 4:\n");
    printf("        - Code: %02x\n", recv_message[0]);  // EAP-Response code
    printf("        - ID: %02x\n", recv_message[1]);        // EAP ID
    printf("        - Length: %d\n", eap_length);           // Response length
    printf("        - Type: %02x\n", recv_message[4]);  // EAP-Type Challenge
    printf("        - Flags: %02x\n", recv_message[5]);  // EAP Flags

    // Print rand_s as a hex string
    printf("        - Rand_s: ");
    for (int i = 0; i < 16; i++) {
         printf("%02x", recv_message[6 + i]);
    }
    printf("\n");

    uint8_t extracted_encrypted_pchannel_data[final_encrypted_len];
    memcpy(extracted_encrypted_pchannel_data, &recv_message[6 + 16], final_encrypted_len);

    printf("AAA Server: [EAP] Received Encrypted PCHANNEL: ");
    for (int i = 0; i < final_encrypted_len; i++) printf("%02x", extracted_encrypted_pchannel_data[i]);
    printf("\n");

    uint8_t recv_nonce[NONCE_LEN];
    uint8_t recv_tag[TAG_LEN];
    uint8_t recv_ciphertext[final_encrypted_len - NONCE_LEN - TAG_LEN];

    memcpy(recv_nonce, extracted_encrypted_pchannel_data, NONCE_LEN);

    memcpy(recv_ciphertext, extracted_encrypted_pchannel_data + NONCE_LEN,
        final_encrypted_len - NONCE_LEN - TAG_LEN);

    memcpy(recv_tag, extracted_encrypted_pchannel_data + final_encrypted_len - TAG_LEN, TAG_LEN);

    // Decrypt the PCHANNEL
    uint8_t decrypted_pchannel_data[BUFFER_SIZE];
    int decrypted_len = decrypt_pchannel(ciphertext, final_encrypted_len - NONCE_LEN - TAG_LEN,
                                        tek, nonce, tag, decrypted_pchannel_data);
    if (decrypted_len < 0) {
        printf("Decryption failed or tag mismatch\n");
        return -1;
    }

    compare_pchannel(serialized, decrypted_pchannel_data, serialized_len);

    printf("AAA Server: [EAP] Key setup...\n");
    print_hex("        - Derived MSK", msk, MSK_SIZE);
    print_hex("        - Derived EMSK", emsk, EMSK_SIZE);

    int result_psk3 = send_eap_success(ssl, eap_id);
    if (result_psk3 != 0) {
        fprintf(stderr, "Error: Failed to send EAP-Response (PKE-3)\n");
        return FAILURE;
    }

    printf("AAA Server: [EAP] Authentication finished!\n");
    return SUCCESS;  // Success
}

int handle_key_selection(const char *recv_message, SSL *ssl) {
    // Defensive check: Ensure we have a valid string
    if (recv_message == NULL) {
        printf("AAA Server: handle_key_selection: received NULL pointer!\n");
        return FAILURE;
    }

    // Print the received message safely, assuming it's null-terminated
    printf("AAA Server: handle_key_selection: received message: '%s'\n", recv_message);

    // Free old key if already set
    if (global_key != NULL) {
        free(global_key);
        global_key = NULL;
        global_key_len = 0;
    }

    // Compare the received string and set the key
    if (strcmp(recv_message, "eap_msk") == 0 && set_msk == 0) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(msk, MSK_SIZE, hash);

        global_key_len = DERIVED_KEY_LEN;
        global_key = malloc(global_key_len);
        if (global_key == NULL) {
            printf("malloc failed for MSK\n");
            return FAILURE; // or custom error code
        }
        memcpy(global_key, hash, DERIVED_KEY_LEN);

        printf("AAA Server: Key changed to MSK\n");
        print_hex("        - MSK", global_key, global_key_len);
        set_msk = 1;

    }

    if (strcmp(recv_message, "eap_psk") == 0) {
        global_key_len = KEY_SIZE;
        global_key = malloc(global_key_len);
        if (global_key == NULL) {
            printf("malloc failed for PSK\n");
            return FAILURE; // or custom error code
        }
        memcpy(global_key, psk, global_key_len);
        printf("AAA Server: Key set to default\n");
        set_msk = 0;
    }

    if (SSL_write(ssl, "OK", 2) <= 0) {
        printf("AAA Server: Failed to send 'OK' response via TLS\n");
        ERR_print_errors_fp(stderr);  // Optional: print OpenSSL error stack
        return FAILURE;
    }

    return SUCCESS;
}


int handle_request_tls(SSL *ssl) {
    uint8_t recv_message[1024];  // Buffer para el mensaje recibido
    ssize_t recv_len;

    // Set socket timeout on underlying socket
    int sockfd = SSL_get_fd(ssl);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // === Paso 1: Recibir y Validar EAP Identity ===
    recv_len = SSL_read(ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        int ssl_err = SSL_get_error(ssl, recv_len);
        if (ssl_err == SSL_ERROR_ZERO_RETURN) {
            printf("AAA Server: TLS connection closed by peer (recv_len: %ld)\n", recv_len);
        } else if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
            printf("AAA Server: TLS read timed out or incomplete, try again\n");
        } else {
            printf("AAA Server: TLS read failed (recv_len: %ld, SSL error: %d)\n", recv_len, ssl_err);
            ERR_print_errors_fp(stderr);  // Optional: print OpenSSL error stack
        }
        return FAILURE;
    }

    // Call the key selection handler
    if (handle_key_selection(recv_message, ssl) != SUCCESS) {
        printf("AAA Server: Error: failed to handle key selection\n");
        return FAILURE;
    }

    recv_len = SSL_read(ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: [EAP] No response received (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }

    uint8_t eap_code = recv_message[0];
    uint8_t eap_type = recv_message[4];
    uint8_t eap_flags = recv_message[5];

    if (eap_code == 0x02 && eap_type == 0x01) {
        printf("AAA Server: [EAP] Received Access Request (EAP-Response/ID)\n");

        if (validate_eap_identity(recv_message, recv_len, ssl) != SUCCESS) {
            return FAILURE;
        }

    } else {
        printf("AAA Server: [EAP] Invalid EAP Identity message\n");
        return FAILURE;
    }

    // === Paso 2: Recibir y Validar EAP PSK2 ===
    recv_len = SSL_read(ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: [EAP] No response received for PSK2 (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }

    eap_code = recv_message[0];
    eap_type = recv_message[4];
    eap_flags = recv_message[5];

    if (eap_code == 0x02 && eap_type == 0x2f && eap_flags == 0x40) {
        printf("AAA Server: [EAP] Received Access Challenge Response (EAP PSK 2)\n");

        if (validate_eap_psk2(recv_message, recv_len, ssl) != SUCCESS) {
            return FAILURE;
        }

    } else {
        printf("AAA Server: [EAP] Invalid response for PSK2 (Type: %02x, Flags: %02x)\n", eap_type, eap_flags);
        return FAILURE;
    }

    // === Paso 3: Recibir y Validar EAP PSK3 ===
    recv_len = SSL_read(ssl, recv_message, sizeof(recv_message));
    if (recv_len <= 0) {
        printf("AAA Server: [EAP] No response received for PSK3 (recv_len: %zd)\n", recv_len);
        return FAILURE;
    }

    eap_code = recv_message[0];
    eap_type = recv_message[4];
    eap_flags = recv_message[5];

    if (eap_code == 0x02 && eap_type == 0x2f && eap_flags == 0xc0) {
        printf("AAA Server: [EAP] Received Access Challenge Response (EAP PSK 3)\n");

        if (validate_eap_psk3(recv_message, recv_len, ssl) != SUCCESS) {
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
    
    printf("Manufacturer's AAA Server:\n");

    // Initialize OpenSSL
    if (init_openssl() != SUCCESS) {
        printf("AAA Server: ERROR - OpenSSL init failed\n");
        return FAILURE;
    }

    aaa_server = socket(AF_INET, SOCK_STREAM, 0);
    if (aaa_server == -1) {
        perror("AAA Server: Could not create socket");
        return FAILURE;
    }
    printf("AAA Server: Socket created\n");
    
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(1815);

    int opt = 1;
    setsockopt(aaa_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(aaa_server, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("AAA Server: Bind failed");
        close(aaa_server);
        return FAILURE;
    }
    
    printf("AAA Server: Bind done\n");

    if (listen(aaa_server, 3) < 0) {
        perror("AAA Server: Listen failed");
        close(aaa_server);
        return FAILURE;
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

        aa_manager = accept(aaa_server, (struct sockaddr *)&aa, &c);
        if (aa_manager < 0) {
            perror("AAA Server: Accept failed");
            continue;
        }

        printf("AAA Server: Connection accepted from Domain AAA Server\n");

        // Create SSL for the new socket
        SSL *aaa_ssl = SSL_new(ssl_ctx);
        SSL_set_fd(aaa_ssl, aa_manager);

        if (SSL_accept(aaa_ssl) <= 0) {
            printf("AAA Server: Error: TLS handshake failed\n");
            ERR_print_errors_fp(stderr);
            SSL_free(aaa_ssl);
            close(aa_manager);
            continue;
        }

        printf("AAA Server: TLS connection established\n");

        while (1) {
            int req_status = handle_request_tls(aaa_ssl);

            if (req_status == FAILURE) {
                printf("AAA Server: Connection ended due to failure\n");
                break;
            } else if (req_status == SUCCESS) {
                printf("AAA Server: Connection ended successfully\n");
                break;
            }
        }

        SSL_shutdown(aaa_ssl);
        SSL_free(aaa_ssl);
        close(aa_manager);

        clock_gettime(CLOCK_MONOTONIC, &timing.end_main);
        timing.total_exec_ns = time_diff_ns(timing.start_main, timing.end_main);

        print_timing_report(&timing);

        printf("AAA Server: Waiting for the next connection...\n");
    }

    close(aaa_server);
    SSL_CTX_free(ssl_ctx);
    return 0;
}