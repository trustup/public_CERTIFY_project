import socket
import sys
from rv_lenode import rv_lenode_reconf_enable_auth


def main(port : int):
    try:
        # Create a TCP/IP socket
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Allows to immediately reuse the address
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        #10.1.0.9
        server_socket.connect(("10.1.0.9", 33333))

        rv_lenode_reconf_enable_auth(server_socket)   # Listen for incoming connections

    except KeyboardInterrupt:
        print("\nServer interrupted.")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Close the server socket
        print("Closing the server...")
        server_socket.close()

if __name__ == "__main__":
    # Get port from command line or use a default port
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 4444

    # Start listening on the specified port
    main(port)

