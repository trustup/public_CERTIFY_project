import socket
import sys
from random import randbytes
from rv_lenode import rv_lenode_bootstrapping, rv_lenode_reconf_enable_auth, rv_lenode_remote_attestation

def main(port : int):
    try:
        # Create a TCP/IP socket
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Allows to immediately reuse the address
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Bind the socket to the server, on the specified port
        server_address = ("0.0.0.0", port)
        print(f"Starting server on port {port}...")
        server_socket.bind(server_address)
        
        # Listen for incoming connections
        server_socket.listen(32)
        
        print("Waiting for connections...")
        # Wait until a connection arrives
        SoC_socket, client_address = server_socket.accept()
        print(f"Connection established from {client_address}")

        # Call the callback function passing the connection socket
        res = rv_lenode_bootstrapping(SoC_socket)
        print("Result of bootstrapping: " + "Success. Continuing" if res else "Failed. Exiting")
        if res != True:
            return
        
        res = rv_lenode_reconf_enable_auth(SoC_socket)
        print("Result of reconfiguration: " + "Success." if res else "Failed.")

        res = rv_lenode_remote_attestation(SoC_socket)
        print("Result of remote attestation: " + "Success." if res else "Failed.")

        # Let's trigger the Device security lock by sending a tampered message.
        # IMPORTANT: Send at least 48 bytes because integrity check is done when
        # at least the CHEADER (48 bytes long) is received
        print("Sending a tampered (random bytes) message to the node")
        SoC_socket.sendall(randbytes(48))
            
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
