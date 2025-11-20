// Tamaño AES
#define AES128_BLOCK_SIZE		16
#define AES128_KEY_BYTE_SIZE		16
#define AES128_KEY_BIT_SIZE		128

#define AES256_BLOCK_SIZE		16
#define AES256_KEY_BYTE_SIZE		32
#define AES256_KEY_BIT_SIZE		256

#define PSK_SIZE 16

// OCALLs DEFINIDAS
#define OCALL_PRINT_STRING 1
#define OCALL_RECIVE_KEY_MATERIALS 2
#define OCALL_WAIT_FOR_MESSAGE 3
#define OCALL_RECIVE_MUD_URL 4
#define OCALL_SEND_CERTIFICATE 5
#define OCALL_RECIVE_CERTIFICATE 6
#define OCALL_SEND_RANDOM      7
#define OCALL_RECIVE_MSK 8
#define OCALL_RECIVE_EDK 9
#define OCALL_SEND_RANDOM_BUFFER 10
#define OCALL_RECIVE_KEY_MATERIALS_MSK 11
#define OCALL_RECIVE_KEY_MATERIALS_PSK 12
#define OCALL_WRITE_TO_HOST_PTR 13
#define OCALL_WRITE_FILE 14


#define AES128_BLOCK_SIZE 16
#define MAX_INPUT_LEN 112  // por ejemplo: hasta 112 bytes de texto (7 bloques)
#define MAX_PADDED_LEN (MAX_INPUT_LEN + AES128_BLOCK_SIZE)  // max 128 con padding


// Estructura que guarda las claves PSK
typedef struct {
    int type; // 0 = PSK, 1 = MSK
    char ak[AES128_BLOCK_SIZE];
    char kdk[AES128_BLOCK_SIZE];
} key_material_t;

// Algoritmos de firma
#define HMAC_MD5 "HMAC_MD5"
#define CMAC_AES "CMAC_AES"

// Estructura para enviar claves (MSK o EDK)
typedef enum {
    KEY_TYPE_MSK = 0,
    KEY_TYPE_EDK = 1
} key_type_t;

typedef struct {
    key_type_t type;
    char key[AES128_BLOCK_SIZE * 4];
} key_send_t;


// Sesiones para derivación de claves
#define SESSION_PSK_KDK 0  // Usar PSK (KDK) como clave base
#define SESSION_MSK      1  // Usar MSK como clave base

// Macro para elegir si se usa PSK (1) o MSK (0) como key_id en la firma
#define USE_PSK_FOR_SIGN 1

#define KEY_ID_MSK "msk_identifier"
#define KEY_ID_EDK "edk_identifier"
#define SIGN_ALGO_PATH "/etc/myapp/sign_algo.bin"