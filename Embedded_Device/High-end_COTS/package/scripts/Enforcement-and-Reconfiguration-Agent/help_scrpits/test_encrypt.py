from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.backends import default_backend
import os
import base64

def encrypt_aes256_cbc(key: bytes, plaintext: str) -> dict:
    if len(key) != 32:
        raise ValueError("Key must be 32 bytes (256 bits) long.")

    iv = b'\x00' * 16  # 16 zero bytes
    backend = default_backend()

    # Pad plaintext to AES block size
    padder = padding.PKCS7(128).padder()
    padded_data = padder.update(plaintext.encode()) + padder.finalize()

    # Create AES-CBC cipher and encrypt
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=backend)
    encryptor = cipher.encryptor()
    ciphertext = encryptor.update(padded_data) + encryptor.finalize()

    return ciphertext

# Example usage
key = os.urandom(32)  # AES-256 = 32 bytes key
plaintext = "Secret message!"

result = encrypt_aes256_cbc(key, plaintext)
print("Ciphertext:", result)
