import socket
import subprocess

DEBUG = True
HOST = '0.0.0.0'
PORT = 5025

reconfigureCommand = ['security_api_interface', 'reconfigure']

def call_ta_reconfigure(opcode, data):

    if DEBUG: 
        print("Calling TA for reconfiguration")
        print("Opcode: " + str(opcode))
        print("Data: " + str(data))

    data_list = []
    for byte in data: data_list.append(byte+0x01) # We add one to the parameters to avoid sending a zero buffer to the executable
    data = bytes(data_list)

    reconfigureStatus = subprocess.Popen(reconfigureCommand + [opcode, data, str(len(data))], stdout=subprocess.PIPE)
    if (reconfigureStatus.wait() != 0):
        print("Error during reconfiguration")
        print(reconfigureStatus.stdout.read().decode('ascii'))
        return False
    else:
        if DEBUG: print(reconfigureStatus.stdout.read().decode('ascii'))
        print("Reconfiguration successfull")

    return True


def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((HOST, PORT))

    # Listen for incoming connections
    server_socket.listen()

    while True:
        # Accept incoming connection
        client_socket, client_address = server_socket.accept()

        if DEBUG: print(f"Connection from {client_address} established.")

        # Handle the connection
        while True:
            # Receive data from the board
            data = client_socket.recv(2048)
            data_len = len(data)
            if not data:
                break

            if DEBUG:
                print("Received: ", data)
                print("Received hex: ", data.hex())
                print("Message length: ", data_len)

            payload_type = int(data[0])

            if payload_type != 0x02: # Reconfiguration message
                    resp = bytes.fromhex('4b4f')  # KO
                    client_socket.sendall(resp)
                    if DEBUG: print("Returned NACK (K0) message. Invalid payload type.")
                    continue

            bootstrap_code = int(data[1])

            if bootstrap_code != 0x02: # High-end device
                    resp = bytes.fromhex('4b4f')  # KO
                    client_socket.sendall(resp)
                    if DEBUG: print("Returned NACK (K0) message. Invalid bootstrap code.")
                    continue

            binLength = int.from_bytes(data[2:4], "big")
            bin = data[4:(binLength+4)]
            opcode = bin[0].to_bytes(1, "big")
            data = bin[1:]

            if DEBUG:
                print("bin length: ", binLength)
                print("bin: ", bin)
                print("Type: ", payload_type)

            if call_ta_reconfigure(opcode, data):
                resp = bytes.fromhex('4f4b')  # OK
            else:
                resp = bytes.fromhex('4b4f')  # KO
            client_socket.sendall(resp)



if __name__ == "__main__":
    main()

