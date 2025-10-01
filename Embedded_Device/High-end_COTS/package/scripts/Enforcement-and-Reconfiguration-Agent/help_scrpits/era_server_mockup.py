import socket
from random import randbytes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.backends import default_backend

key = bytearray.fromhex("dae41759aa9903a9f280ded67227f30bbe9c0bdb03c47d28c45d5be6bb00fed8") #Set with correct EDK

def iso9797_m2_pad(data: bytes, block_size: int = 16) -> bytes:
    pad_len = block_size - (len(data) % block_size)
    padding = b'\x80' + b'\x00' * (pad_len - 1)
    return data + padding

def encrypt_aes256_cbc(key: bytes, plaintext: str):
    if len(key) != 32:
        raise ValueError("Key must be 32 bytes (256 bits) long.")

    iv = b'\x00' * 16  # 16 zero bytes
    backend = default_backend()

    # Pad plaintext to AES block size
    padded_data = iso9797_m2_pad(plaintext)

    # Create AES-CBC cipher and encrypt
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=backend)
    encryptor = cipher.encryptor()
    ciphertext = encryptor.update(padded_data) + encryptor.finalize()

    return ciphertext

def compose_message_wipe():
    bin = 0x04.to_bytes(1)
    len16 = len(bin).to_bytes(2, byteorder='big')
    payload_type = 0x02.to_bytes(1)
    bootstrap_code = 0x02.to_bytes(1)
    return payload_type + bootstrap_code + len16 + bin

def compose_message_change_signature():
    bin = 0x01.to_bytes(1)
    new_alg = 0x00.to_bytes(1) #HMAC
    len16 = 0x02.to_bytes(2, byteorder='big')
    payload_type = 0x02.to_bytes(1)
    bootstrap_code = 0x02.to_bytes(1)
    return payload_type + bootstrap_code + len16 + bin + new_alg

def compose_incorrect_message():
    return randbytes(32)

def compose_message_zeros():
    return b'\x00' * 16

def compose_message_ff():
    return b'\xff' * 16


def send_message():
    host = "10.0.0.8"  # Localhost
    port = 5025         # Port to connect to

    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
            # Connect to the server
            client_socket.connect((host, port))
            print(f"Connected to {host}:{port}")

            # Send incorrect message
            # bad_message = compose_incorrect_message()
            # client_socket.sendall(bad_message)
            # print(f"Sent: {bad_message}")

            # response = client_socket.recv(1024)
            # print(f"Received: {response}")

            message = compose_message_change_signature()
            ecnrypted_message = encrypt_aes256_cbc(key, message)
            #ecnrypted_message = compose_message_ff()

            # Send the message
            client_socket.sendall(ecnrypted_message)
            #client_socket.sendall(compose_message_wipe())
            #client_socket.sendall(compose_incorrect_message())
            print(len(ecnrypted_message))
            print(f"Sent: {message}")

            # Optionally, receive a response from the server
            response = client_socket.recv(1024)
            print(f"Received: {response}")

    except ConnectionRefusedError:
        print(f"Could not connect to {host}:{port}. Is the server running?")


if __name__ == "__main__":
    send_message()
