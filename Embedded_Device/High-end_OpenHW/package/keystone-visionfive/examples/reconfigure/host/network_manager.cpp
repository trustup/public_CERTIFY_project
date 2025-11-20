#include <network_manager.h>

#define AES_256_CBC 1

uint16_t crc_buypass(const unsigned char * input_str, size_t num_bytes) {
	printf("[DEBUG] crc_buypass: INICIO num_bytes=%zu\n", num_bytes);
	uint16_t crc = 0x0000;
  
	for (size_t i = 0; i < num_bytes; i++) {
	  crc ^= ((uint16_t)input_str[i] << 8); // XOR with the current byte, shifted left by 8 bits
  
	  for (int j = 0; j < 8; j++) {
		if (crc & 0x8000) {
		  crc = (crc << 1) ^ 0x8005; // Shift left and XOR with polynomial
		} else {
		  crc = (crc << 1); // Shift left
		}
	  }
	}
  
	printf("[DEBUG] crc_buypass: FIN crc=0x%04x\n", crc);
	return crc;
  }

uint32_t send_bytes(char * addr, uint16_t port, uint8_t * message, size_t message_sz, int * sockfd_ptr) {
	// printf("[DEBUG] send_bytes: addr=%s, port=%u, message_sz=%zu\n", addr, port, message_sz);
	printf("[HOST] Connecting and sending message to server for join_request...\n");
	int sockfd;
	struct sockaddr_in server_addr;

	// Create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket creation failed");
		return -1;
	}
	// printf("[DEBUG] send_bytes: socket creado sockfd=%d\n", sockfd);

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
	// printf("[DEBUG] send_bytes: dirección convertida\n");

	// Connect to the server
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		close(sockfd);
	return -1;
	}
	// printf("[DEBUG] send_bytes: conectado al servidor\n");

	// Send message
	if (send(sockfd, message, message_sz, 0) < 0) {
		perror("Send failed");
		close(sockfd);
	return -1;
	}

	// printf("[DEBUG] send_bytes: Message sent successfully\n");

	// Close the socket
	if (!sockfd) 
		close(sockfd);
	else 
		*sockfd_ptr = sockfd;
	// printf("[DEBUG] send_bytes: FIN\n");
	return 0;
}

uint32_t send_bytes_socket(int sockfd, uint8_t * message, size_t message_sz) {
	// printf("[DEBUG] send_bytes_socket: sockfd=%d, message_sz=%zu\n", sockfd, message_sz);
	printf("[HOST] Sending bytes socket\n");
	// Send message
	if (send(sockfd, message, message_sz, 0) < 0) {
		perror("Send failed");
		close(sockfd);
	return -1;
	}

	// printf("[DEBUG] send_bytes_socket: Message sent successfully\n");

	// Close the socket
	if (!sockfd) 
		close(sockfd);
	// printf("[DEBUG] send_bytes_socket: FIN\n");
	return 0;
}

uint32_t receive_bytes(uint16_t port, char * buffer, size_t * buffer_size, int * sockfd, int * server_socket) {
	printf("[HOST] Setting up server to receive data for join_request...\n");
	printf("[DEBUG] receive_bytes: port=%u, buffer_size=%zu\n", port, *buffer_size);
	struct sockaddr_in server_addr, client_addr;
        socklen_t addr_size = sizeof(client_addr);
        ssize_t received_size;
	int opt = 1;
        
	// printf("Create socket\n");
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
        
	// printf("Configure server address structure\n");
        // Configure server address structure
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
	// printf("Bind socket to port\n");
        // Bind socket to port
        if (bind(*server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Bind failed");
            close(*server_socket);
            return 2;
        }
        
	// printf("Listen for incoming connections\n");
        // Listen for incoming connections
        if (listen(*server_socket, 1) == -1) {
            perror("Listen failed");
            close(*server_socket);
            return 3;
        }
        
	// printf("Accept a connection\n");
        // Accept a connection
        *sockfd = accept(*server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (*sockfd == -1) {
            perror("Accept failed");
            close(*server_socket);
            return 4;
        }
        
	// printf("Receive data\n");
        // Receive data
        received_size = recv(*sockfd, buffer, *buffer_size, 0);
        if (received_size == -1) {
            perror("Receive failed");
            close(*server_socket);
            close(*sockfd);
            return 5;
        }
        
	// printf("Finished receiving\n");
	buffer[received_size] = '\0'; // Null-terminate the string
        // Update buffer size with actual received bytes
        *buffer_size = (size_t)received_size;
        
	// printf("[DEBUG] receive_bytes: FIN received_size=%zd\n", received_size);
        return 0;  // Success
}


uint32_t receive_bytes_socket(int socketfd, char *buffer, size_t * buffer_size) {
	// printf("[DEBUG] receive_bytes_socket: socketfd=%d, buffer_size=%zu\n", socketfd, *buffer_size);
	printf("[HOST] Receiving data from server for join_request...\n");
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
	// printf("[DEBUG] receive_bytes_socket: FIN bytes_received=%zd\n", bytes_received);
	return 0;
}

uint32_t send_bytes_encrypted_signed(char * key_id, char * addr, uint16_t port, uint8_t * message, size_t message_sz, int * sockfd_ptr) {
	printf("[DEBUG] send_bytes_encrypted_signed: key_id=%s, addr=%s, port=%u, message_sz=%zu\n", key_id, addr, port, message_sz);
	uint32_t res;
	uint16_t crc = 0x0000;

	size_t encrypted_message_sz = message_sz + 16 - (message_sz % 16);
	size_t final_message_sz = encrypted_message_sz + 2;
	unsigned char * encrypted_message = (unsigned char*)malloc(final_message_sz);
	unsigned char * final_message = encrypted_message;

	if ( (res = csp_encryptData(key_id, message, message_sz, encrypted_message, AES_256_CBC)) ) {
		printf("Error encrypting message: %s\n", message);
		return res;
	}

	crc = crc_buypass(encrypted_message, encrypted_message_sz);
	
	final_message[encrypted_message_sz] = htons(crc) & 0xFF00;
	final_message[encrypted_message_sz + 1] = htons(crc) & 0x00FF;
	printf("[DEBUG] send_bytes_encrypted_signed: FIN\n");
	return send_bytes(addr, port, final_message, final_message_sz, sockfd_ptr);
}


uint32_t close_connection(int socketfd) {
	printf("[DEBUG] close_connection: socketfd=%d\n", socketfd);
	close(socketfd);
	printf("[DEBUG] close_connection: FIN\n");
	return 0;
}

uint32_t reset_connection(int * socketfd, int * server_socket, int new_port, char * buffer, size_t * buffer_size) {
	// printf("[DEBUG] reset_connection: socketfd=%d, server_socket=%d, new_port=%d\n", *socketfd, *server_socket, new_port);
	close(*socketfd);

	return receive_bytes(new_port, buffer, buffer_size, socketfd, server_socket);
}

uint32_t get_mac(char * mac_address) {
	// printf("[DEBUG] get_mac: INICIO\n");
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
	// printf("[DEBUG] get_mac: FIN\n");
	return 0;
}

