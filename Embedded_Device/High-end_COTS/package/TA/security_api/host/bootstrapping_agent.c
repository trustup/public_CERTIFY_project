#include "custom_se_pkcs11.h"
#include "rfc4764.h"
#include <bootstrapping_agent.h>
#include <stdint.h>

int32_t bsa_sendJoinRequest(char * psk_id, char * msk_id) {

	char mUDuRLValue[DEFAULT_MUD_URL_BUFFER_SIZE]; 
	int32_t res;

	memset(mUDuRLValue, 0x7f, DEFAULT_MUD_URL_BUFFER_SIZE); // Non ascii to identify end of string as 0 si a reserved value
	mUDuRLValue[DEFAULT_MUD_URL_BUFFER_SIZE - 1] = 0;
	res = csp_getMuDFileURL(mUDuRLValue);
	if (res) {
		printf("Error retrieving Mud URL value\n");
		return res;
	} 
	
	char * mud_url_end = strchr(mUDuRLValue, 0x7f);
	*mud_url_end = 0;
	res = join_request(mUDuRLValue, psk_id, msk_id);
	if (res) {
		printf("Error during join request\n");
	}

	return res;
}

int32_t bsa_get_mudURL(void) {

	char mUDuRLValue[DEFAULT_MUD_URL_BUFFER_SIZE]; 

	csp_initialize();

	memset(mUDuRLValue, 0x7f, DEFAULT_MUD_URL_BUFFER_SIZE);
	mUDuRLValue[DEFAULT_MUD_URL_BUFFER_SIZE - 1] = 0;
	if (csp_getMuDFileURL(mUDuRLValue)) {
		printf("Error retrieving Mud URL value\n");
		return 1;
	} 

	csp_terminate();

	char * mud_url_end = strchr(mUDuRLValue, 0x7f);
	*mud_url_end = 0;

	printf("mudUrl:%s", mUDuRLValue);

	return SUCCESS;
}

int32_t bsa_get_certificate(void) {

	csp_initialize();
	
	unsigned char ret_cert[DEFAULT_CERT_BUFFER_SIZE] = { 0 };
	uint16_t cert_size =  DEFAULT_CERT_BUFFER_SIZE;

	if (csp_getCertificateValue((unsigned char *)ret_cert, &cert_size)) {
		printf("Error retreiving certificate value\n");
		return 1;
	}

	printf("retreived cert size: %d\n", cert_size);
	printf("retreived cert: ");
	printf("0x");
	for (uint32_t i = 0; i < cert_size; i++) printf("%02x ", ret_cert[i]);
	printf("\n");
	printf("certificate: %.*s", cert_size, ret_cert);

	csp_terminate();

	return 0;
}


