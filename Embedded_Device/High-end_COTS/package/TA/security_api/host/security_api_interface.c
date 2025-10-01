#include <bootstrapping_agent.h>
#include <ERA_agent.h>

static int32_t bootstrap(unsigned char * certificate) {

	unsigned char psk[16] = { 0 };		
	char * mudUrl = "http://localhost:8091/MUD_Collins_Bootstrapping";

	csp_initialize();

	printf("Certificate to store: %s\n", (char*)certificate);
	printf("Aleged size: %d\n", (int)strlen((char*)certificate));
	printf("Certificate contents: ");
	printf("0x");
	for (uint32_t i = 0; i < (int)strlen((char*)certificate); i++) printf("%02x ", certificate[i]);
	printf("\n");

	if (csp_installPSK(psk)) {
		printf("Error installing PSK\n");
		return 1;
	}
	if (csp_installMudURL(mudUrl)){
		printf("Error installing mudUrl\n");
		return 1;
	}

	if (csp_installCertificate(certificate, strlen((char*)certificate))){
		printf("Error installing certificate\n");
		return 1;
	}

	if (bsa_sendJoinRequest("PSK", "MSK")){
		printf("Error during join request\n");
		return 1;
	}

	if (bsa_sendJoinRequest("MSK", "EDK_01")) {
		printf("Error during secondary key derivation\n");
		return 1;
	}

	csp_terminate();

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Invalid argument\n");
		return EXIT_FAILURE;
	}

	if (!strcmp(argv[1], "bootstrap")) {
		return bootstrap((unsigned char *)argv[2]);
	}

	if (!strcmp(argv[1], "getcert")) {
		return bsa_get_certificate();
	}

	if (!strcmp(argv[1], "getMudUrl")) {
		return bsa_get_mudURL();
	}

	if (!strcmp(argv[1], "reconfigure")) {
		return reconfigure((uint8_t)(*argv[2]), argv[3], argv[4]);
	}

	if (!strcmp(argv[1], "decrypt")) {
		return decrypt_reconfiguration(argv[2], argv[3]);
	}

	return EXIT_FAILURE;
}
