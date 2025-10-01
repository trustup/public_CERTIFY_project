import socket
import threading
import time

HOST = '0.0.0.0'
PORT = 33000

AAA_SERVER_HOST = '155.54.95.211'
AAA_SERVER_PORT = 44000

ATT_SERVER_HOST = '155.54.95.211'
ATT_SERVER_PORT = 44000

def connect_to_aaa_server(message, reconf, client_socket):
    """
        Connect to the AAA server and forward respective messages.
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as aaa_socket:
            aaa_socket.connect((AAA_SERVER_HOST, AAA_SERVER_PORT))
            print(f"Connected to AAA server at {AAA_SERVER_HOST}:{AAA_SERVER_PORT}.")

            aaa_socket.sendall(message)
            print(f"Sent to AAA server: {message.hex()}")

            if reconf:
                aaa_socket.close()
                # ATTESTATION ------------------------
                initial_atts_message = bytes.fromhex('000100054154545F53')  # ATT_S
                attd_message = bytes.fromhex('000100054154545F44')  # ATT_D
                client_socket.sendall(initial_atts_message)

                print(f"Sent to board: {initial_atts_message}")

                attestation_handler(client_socket, attd_message)

            else:
                response = aaa_socket.recv(2048)
                print(f"Received from AAA server: {response.hex()}")
                return response

            return 0
    except Exception as e:
        print(f"Error connecting to AAA server: {e}")
        return None


def handle_reconfiguration_messages(board_socket, aaa_socket):
    """
        Handles messages for the reconfiguration.
    """

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((HOST, 22000))

    server_socket.listen()

    print(f"[Reconfiguration] Proxy is listening on port 22000...")

    while True:
        client_socket, client_address = server_socket.accept()

        print(f"Connection from {client_address} established.")

        while True:
            try:
                data = client_socket.recv(2048)
                if not data:
                    break

                print(f"Reconfiguration message received from AAA server: {data.hex()}")

                first_byte = data[0]

                match first_byte:
                    case 0x02:
                        board_socket.sendall(data)
                        print("Forwarded 0x02 message from AAA server to the board.")

                        #board_response = board_socket.recv(2048)
                        #if board_response.hex().startswith("02"):
                            #aaa_socket.sendall(board_response)
                        #    print("Forwarded reconfiguration ACK message to AAA server....")
                        #    connect_to_aaa_server(board_response, False, None)
                        #else:
                        #    print("Failed to forward reconfiguration ACK message to AAA server.")

                    case 0x01:
                        board_socket.sendall(data)
                        print("Forwarded 0x01 message from AAA server to the board.")

                    case _:
                        print(f"Unknown message type from AAA server: {first_byte}")

            except Exception as e:
                print(f"Error receiving from AAA server: {e}")
                break

        print("Closing reconfiguration message handling thread.")


def connect_to_att_server(message):
    """
        Connect to the ATT server and forward respective messages.
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as att_socket:
            att_socket.connect((ATT_SERVER_HOST, ATT_SERVER_PORT))
            print(f"Connected to ATT server at {ATT_SERVER_HOST}:{ATT_SERVER_PORT}.")

            att_socket.sendall(message)
            print(f"Sent to ATT server: {message.hex()}")

            response = att_socket.recv(2048)
            print(f"Received from ATT server: {response.hex()}")


            return response
    except Exception as e:
        print(f"Error connecting to ATT server: {e}")
        return None


def attestation_handler(board_socket, attd_message):
    while True:
        try:
            response = board_socket.recv(2048)
            print(f"Received from board: {response.hex()}")

            first_byte = response[0]

            match first_byte:
                case 0x07:  # signature+random
                    print("Received signature+random message from the board.")
                    print("Opening connection to ATT server...")
                    att_response = connect_to_att_server(response)
                    if att_response.hex().startswith("08"):
                        # Forward the response (ACK / NACK) back to the board
                        board_socket.sendall(att_response)
                        print("Forwarded ATT server ACK/NACK to the board.")

                        board_socket.sendall(attd_message)
                        print("Forwarded ATTD message to the board.")

                        bootstrapping_complete = True
                        last_sent_time = time.time()
                    else:
                        print("ATT server response is not an ACK / NACK.")
                case 0x04:  # normal attestion log message
                    print("Received attestation log message from the board.")
                    print("Opening connection to ATT server...")
                    att_response = connect_to_att_server(response)
                    if att_response.hex().startswith("08"):
                        # Forward the response (ACK / NACK) back to the board
                        board_socket.sendall(att_response)
                        print("Forwarded ATT server ACK/NACK to the board.")
                    else:
                        print("ATT server response is not an ACK / NACK.")

                case _:
                    print(f"Unknown message type: {first_byte}")

            if bootstrapping_complete and time.time() - last_sent_time >= 10:
                board_socket.sendall(attd_message)
                print("Sent ATT request to board...")
                last_sent_time = time.time()

        except Exception as e:
            print(f"Error while listening to board: {e}")
            break

def main():
    """
        Main proxy server function that listens for board connections and forwards
        messages to AAA server and vice versa.
    """

    proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    proxy_socket.bind((HOST, PORT))

    proxy_socket.listen()

    print(f"Proxy is listening on port {PORT}...")

    while True:
        client_socket, client_address = proxy_socket.accept()

        print(f"Connection from {client_address} established.")

        while True:
            data = client_socket.recv(2048)
            data_len = len(data)
            if not data:
                break

            print("Received: ", data)
            print("Received hex: ", data.hex())
            print("Message length: ", data_len)

            try:
                if "Hello" in data.decode('utf-8'):
                    print("Client Hello message received from the board.")
                    #resp = "Hello"
                    #bytes_resp = resp.encode('utf-8')
                    client_socket.sendall(data)
                    print("Server Hello message sent back to the board.")

            except UnicodeDecodeError:
                pass

            first_byte = data[0]

            match first_byte:

                # ----------------------- Bootstrapping Message Handling -------------------------------------------
                case 0x01:
                    print("This is a MUD URL message.")
                    print("Opening connection to AAA server...")
                    aaa_response = connect_to_aaa_server(data, False, None)
                    if aaa_response.hex().startswith("05"):
                        client_socket.sendall(aaa_response)
                        print("Forwarded AAA server challenge to the board.")
                    else:
                        print("AAA server response is not a challenge.")

                case 0x07:
                    print("Received challenge+signature+random message from the board.")
                    print("Opening connection to AAA server...")
                    aaa_response = connect_to_aaa_server(data, False, None)
                    if aaa_response.hex().startswith("08"):
                        # Forward the response (ACK / NACK) back to the board
                        client_socket.sendall(aaa_response)
                        print("Forwarded AAA server ACK/NACK to the board.")

                        # ----------------------- Reconfiguration Message Handling ------------------------------
                        print("Starting reconfiguration message handling thread...")
                        try:
                            aaa_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                            aaa_socket.connect((AAA_SERVER_HOST, AAA_SERVER_PORT))
                            aaa_socket.close()
                            threading.Thread(target=handle_reconfiguration_messages, args=(client_socket, aaa_socket),
                                             daemon=True).start()
                        except Exception as e:
                            print(f"Failed to start reconfiguration message handling: {e}")

                    else:
                        print("AAA server response is not an ACK / NACK.")

                case 0x02:
                    print("Forwarded reconfiguration ACK message to AAA server.")
                    aaa_response = connect_to_aaa_server(data, True, client_socket)

                case _:
                    print(f"Unknown message: {data}")
                    print(f"Unknown message type first byte: {first_byte}")


        print(f"Connection from {client_address} closed.")
        client_socket.close()


if __name__ == '__main__':
    main()
