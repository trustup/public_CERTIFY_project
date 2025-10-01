#include <rfc4764.h>

#define DEFAULT_MUD_URL_BUFFER_SIZE	512
#define DEFAULT_CERT_BUFFER_SIZE	4096

int32_t bsa_sendJoinRequest(char * psk_id, char * msk_id);

int32_t bsa_get_mudURL(void);

int32_t bsa_get_certificate(void);
