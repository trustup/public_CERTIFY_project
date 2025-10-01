#include "include/network_manager.h"
#include <network_manager.h>
#include <stdio.h>
#include <unistd.h>

uint32_t send_bytes(char * addr, uint16_t port, uint8_t * message, size_t message_sz, int * sockfd_ptr) {

	int sockfd;
	struct sockaddr_in server_addr;

	// Create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket creation failed");
		return -1;
	}

	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	// Convert address from text to binary form
	if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
		perror("Invalid address or address not supported");
		close(sockfd);
	return -1;
	}

	// Connect to the server
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		close(sockfd);
	return -1;
	}

	// Send message
	if (send(sockfd, message, message_sz, 0) < 0) {
		perror("Send failed");
		close(sockfd);
	return -1;
	}

	printf("Message sent successfully\n");

	// Close the socket
	if (!sockfd) 
		close(sockfd);
	else 
		*sockfd_ptr = sockfd;
	return 0;
}

uint32_t send_bytes_socket(int sockfd, uint8_t * message, size_t message_sz) {

	// Send message
	if (send(sockfd, message, message_sz, 0) < 0) {
		perror("Send failed");
		close(sockfd);
	return -1;
	}

	printf("Message sent successfully\n");

	// Close the socket
	if (!sockfd) 
		close(sockfd);
	return 0;
}

uint32_t receive_bytes(uint16_t port, char * buffer, size_t * buffer_size, int * sockfd, int * server_socket) {

	struct sockaddr_in server_addr, client_addr;
        socklen_t addr_size = sizeof(client_addr);
        ssize_t received_size;
	int opt = 1;
        
	printf("Create socket\n");
        // Create socket
        *server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (*server_socket == -1) {
            perror("Socket creation failed");
            return 1;
        }

	if (setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt failed");
		return 1;
	}
	if (setsockopt(*server_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		return 1;
	}
        
	printf("Configure server address structure\n");
        // Configure server address structure
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
	printf("Bind socket to port\n");
        // Bind socket to port
        if (bind(*server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Bind failed");
            close(*server_socket);
            return 2;
        }
        
	printf("Listen for incoming connections\n");
        // Listen for incoming connections
        if (listen(*server_socket, 1) == -1) {
            perror("Listen failed");
            close(*server_socket);
            return 3;
        }
        
	printf("Accept a connection\n");
        // Accept a connection
        *sockfd = accept(*server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (*sockfd == -1) {
            perror("Accept failed");
            close(*server_socket);
            return 4;
        }
        
	printf("Receive data\n");
        // Receive data
        received_size = recv(*sockfd, buffer, *buffer_size, 0);
        if (received_size == -1) {
            perror("Receive failed");
            close(*server_socket);
            close(*sockfd);
            return 5;
        }
        
	printf("Finished receiving\n");
	buffer[received_size] = '\0'; // Null-terminate the string
        // Update buffer size with actual received bytes
        *buffer_size = (size_t)received_size;
        
        return 0;  // Success
}


uint32_t receive_bytes_socket(int socketfd, char *buffer, size_t * buffer_size) {

	if (!buffer || *buffer_size == 0) {
	fprintf(stderr, "Invalid buffer or size.\n");
	return -1;
	}

	ssize_t bytes_received = recv(socketfd, buffer, *buffer_size, 0);
	if (bytes_received < 0) {
		perror("recv");
		return -1;
	}

	buffer[bytes_received] = '\0'; // Null-terminate the string
	*buffer_size = bytes_received;
	return 0;
}

uint32_t send_bytes_encrypted_signed(char * key_id, char * addr, uint16_t port, uint8_t * message, size_t message_sz, int * sockfd_ptr) {

	uint32_t res;
	uint16_t crc = 0x0000;

	size_t encrypted_message_sz = message_sz + 16 - (message_sz % 16);
	size_t final_message_sz = encrypted_message_sz + 2;
	unsigned char * encrypted_message = malloc(final_message_sz);
	unsigned char * final_message = encrypted_message;

	if ( (res = csp_encryptData(key_id, message, message_sz, encrypted_message, AES_256_CBC)) ) {
		printf("Error encrypting message: %s\n", message);
		return res;
	}

	crc = crc_buypass(encrypted_message, encrypted_message_sz);
	
	final_message[encrypted_message_sz] = htons(crc) & 0xFF00;
	final_message[encrypted_message_sz + 1] = htons(crc) & 0x00FF;

	return send_bytes(addr, port, final_message, final_message_sz, sockfd_ptr);
}


uint32_t close_connection(int socketfd) {

	close(socketfd);
	return 0;
}

uint32_t reset_connection(int * socketfd, int * server_socket, int new_port, char * buffer, size_t * buffer_size) {

	close(*socketfd);

	return receive_bytes(new_port, buffer, buffer_size, socketfd, server_socket);
}

uint32_t get_mac(char * mac_address) {

	struct ifreq ifr;
	struct ifconf ifc;
	char buf[1024];
	int success = 0;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) { 
		printf("Error opening socket\n");
		return -1;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
		printf("Error ioctling socket\n");
		return -1;
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) {
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
		    if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
			if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
			    success = 1;
			    break;
			}
		    }
		}
		else {
			printf("Error iterating over ifc\n");
			return -1;
		}

	}

	if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
	else return -1;
	return 0;
}

