#ifndef NETWORK_MANAGER
#define NETWORK_MANAGER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <custom_se_pkcs11.h>
#include <checksum.h>

uint32_t send_bytes(char * addr, uint16_t port, uint8_t * message, size_t message_sz, int * sockfd);

uint32_t send_bytes_socket(int sockfd, uint8_t * message, size_t message_sz);

uint32_t receive_bytes(uint16_t port, char * buffer, size_t * buffer_size, int * sockfd, int * server_socket);

uint32_t receive_bytes_socket(int socketfd, char *buffer, size_t * buffer_size);

uint32_t send_bytes_encrypted_signed(char * key_id, char * addr, uint16_t port, uint8_t * message, size_t message_sz, int * sockfd);

uint32_t close_connection(int socketfd);

uint32_t reset_connection(int * socketfd, int * server_socket, int new_port, char * buffer, size_t * buffer_size);

uint32_t get_mac(char * mac_address);

#endif // NETWORK_MANAGER
