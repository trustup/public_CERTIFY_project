import socket
import sys
import signal

sys.path.insert(1, 'certify-uc3-demo-main/')
from bootstrapping import low_end_bootstrap
from rv_lenode import rv_lenode_bootstrapping
from high_end_bootstrapping import high_end_bootstrap

DEBUG = True
HOST = '0.0.0.0'
PORT = 33333

PSK_KEY = "eap_psk"
MSK_KEY = "eap_msk"

server_socket = None  # So it's accessible globally for cleanup

def signal_handler(sig, frame):
    global server_socket
    if server_socket:
        print("\nShutting down server and freeing port...")
        server_socket.close()
    sys.exit(0)

# Register signal handler for graceful shutdown on Ctrl+C
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

def main():
    global server_socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Allow immediate reuse of port after shutdown
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        if DEBUG: print(f"Server listening on {HOST}:{PORT}")

        while True:
            client_socket, client_address = server_socket.accept()

            if DEBUG: print(f"Connection from {client_address} established.")

            try:
                while True:
                    data = client_socket.recv(2048)
                    if not data:
                        break

                    if DEBUG:
                        print("Received: ", data)
                        print("Received hex: ", data.hex())
                        print("Message length: ", len(data))

                    payload_type = int(data[0])

                    match payload_type:
                        case 0x00:
                            try:
                                security_header_size = 40
                                mudUrlStart = security_header_size + 16
                                code = 0x04
                                mudUrlLength = int.from_bytes(data[security_header_size+2:security_header_size+6], "big")
                                mudUrl = data[mudUrlStart:mudUrlStart+mudUrlLength].decode('utf-8')
                            except UnicodeDecodeError:
                                client_socket.sendall(bytes.fromhex('4b4f'))
                                if DEBUG: print("Returned NACK (K0) message. Invalid format.")
                                continue

                        case 0x01:
                            try:
                                code = int(data[1])
                                mudUrlLength = int.from_bytes(data[2:4], "big")
                                mudUrl = data[4:4+mudUrlLength].decode('utf-8')
                            except UnicodeDecodeError:
                                client_socket.sendall(bytes.fromhex('4b4f'))
                                if DEBUG: print("Returned NACK (K0) message. Invalid format.")
                                continue

                        case _:
                            client_socket.sendall(bytes.fromhex('4b4f'))
                            if DEBUG: print("Returned NACK (K0) message. Invalid payload type.")
                            continue

                    if DEBUG:
                        print("Mud Url length: ", mudUrlLength)
                        print("Mud Url: ", mudUrl)
                        print("Type: ", payload_type)
                        print("Code: ", code)

                    match code:
                        case 0x01:
                            if DEBUG: print("Calling low end A bootstrapping process...")
                            low_end_bootstrap(server_socket, client_socket, client_address)
                            break
                        case 0x02:
                            if DEBUG: print("Calling high end bootstrapping process for primary key derivation...")
                            high_end_bootstrap(mudUrl, client_socket, PSK_KEY)
                            break
                        case 0x03:
                            if DEBUG: print("Calling high end bootstrapping process for secondary key derivation...")
                            high_end_bootstrap(mudUrl, client_socket, MSK_KEY)
                            break
                        case 0x04:
                            if DEBUG: print("Calling low end B bootstrapping process...")
                            rv_lenode_bootstrapping(server_socket, client_socket)
                            break
                        case _:
                            client_socket.sendall(bytes.fromhex('4b4f'))
                            if DEBUG: print("Returned NACK (K0) message. Invalid bootstrap code.")
            finally:
                client_socket.close()
                if DEBUG: print(f"Connection with {client_address} closed.")
    finally:
        if server_socket:
            server_socket.close()
            if DEBUG: print("Server socket closed. Port freed.")

if __name__ == "__main__":
    main()

