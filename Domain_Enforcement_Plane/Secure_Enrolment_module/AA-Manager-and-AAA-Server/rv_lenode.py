import socket
import hashlib
import hmac
from random import randbytes

RV_LENODE_MSK = "24390E18CB94309EDCE27D99514EC8F4338F2E0AA071D8A3703AC869A9888266"

def _print_hex_array(data: bytes, bytes_per_line = 8):
    for i in range(0, len(data), bytes_per_line):
        line = data[i:i+bytes_per_line]
        hex_line = ' '.join(f'{b:02x}' for b in line)
        print(hex_line)

def _recvall(sock, n):
    data = b""
    while len(data) < n:
        to_read = n - len(data)
        packet = sock.recv(to_read)
        if not packet:
            raise ConnectionError("Connection closed")
        data += packet
    return data

# Create the certify CMESSAGE
def _certify_cmessage(payload, authenticate = False, encrypt = False) -> bytes:
    aa = (1).to_bytes(1) if authenticate == True else (0).to_bytes(1)
    ea = (1).to_bytes(1) if encrypt == True else (0).to_bytes(1)
    ak = (2).to_bytes(2, byteorder='big') # MSK as key always
    ek = (2).to_bytes(2, byteorder='big') # MSK as key always
    zeros1 = bytes(2)
    payload_len = len(payload).to_bytes(4, byteorder='big')
    zeros2 = bytes(4)
    authentication_tag = bytes(32)
    
    if encrypt == True:
        pass # TODO: Implement encryption

    if authenticate == True:
        authentication_tag = hmac.new(
            bytes.fromhex(RV_LENODE_MSK),
            aa + ea + ak + ek + zeros1 + payload_len + zeros2 + payload,
            hashlib.sha256).digest()
    
    return aa + ea + ak + ek + zeros1 + payload_len + zeros2 + authentication_tag + payload

def _clg_message(clg : bytes):
    return (13).to_bytes(1) + bytes(15) + clg[:64]

def _policy_change_signature_message():
    payload_type = (10).to_bytes(1)
    policies_len = (48).to_bytes(4, byteorder='big')
    zeros = bytes(11)
    p1 = bytearray(48)
    p1[0] = 1
    p1[1] = 1
    p1[2] = 1
    return payload_type + policies_len + zeros + p1

def rv_lenode_bootstrapping(SoC_socket : socket.socket) -> bool:
    """
    Do the domain bootstrapping for the RISCV le-node from [SoC_socket] connection.
    The caller MUST pass a [SoC_socket] from which this function can receive and send the messages,
    considering the CERTIFY optional security header and the MUD URL message itself
    Reference: https://umu-projects.inf.um.es/products/files/doceditor.aspx?fileid=13579
    at slide #1 and #27
    This function:
    1) Receives the CERTIFY optional security header + MUD URL message from the node,
    2) Execute the remote attestation by sending a challenge and waiting for the response,
    3) Returns true if process is successfull otherwise false
    """

    certify_header = _recvall(SoC_socket, 48)
    payload_len = int.from_bytes(certify_header[8:12], byteorder='big')
    mud_msg = _recvall(SoC_socket, payload_len)
    
    # MUD URL is payload_type = 9
    if mud_msg[0] != 9: return False

    bootstrp_code = int(mud_msg[1])
    mud_len = int.from_bytes(mud_msg[2:6], byteorder='big')
    url = mud_msg[16: 16 + mud_len].decode("ascii")

    print(f"Received MUD URL message\nbootstrp_code: {bootstrp_code}; mud_len: {mud_len}; url: {url}\n")

    # Sending the challenge message
    clg = randbytes(64)
    clg_msg = _certify_cmessage(_clg_message(clg), True, False)
    if SoC_socket.sendall(clg_msg) != None:
        return False
    
    print("Sent CLG message")
    _print_hex_array(clg_msg)
    print()

    # Wait for response
    certify_header = _recvall(SoC_socket, 48)
    payload_len = int.from_bytes(certify_header[8:12], byteorder='big')
    rsp_msg = _recvall(SoC_socket, payload_len)

    # RSP is payload_type = 14
    if rsp_msg[0] != 14: return False
    rsp = rsp_msg[16: 80]

    print("Received RSP message")
    _print_hex_array(certify_header + rsp_msg)
    print()

    # Check rsp is equal to signed clg
    clg_signed = hmac.new(bytes.fromhex(RV_LENODE_MSK), clg, hashlib.sha256).digest() + bytes(32)
    if rsp != clg_signed: return False

    return True

def rv_lenode_reconf_enable_auth(SoC_socket : socket.socket) -> bool:
    """
    Perform a reconfiguration of the RISCV le-node from [SoC_socket] connection.
    Specifically:
    1) Send a reconfiguration message to the ERA Agent of the node containing one policy
        that forces it to authenticate the messages it produces from now on,
    2) Wait for the ack message from the node (this should be authenticated)
        informing the policy was correctly applied,
    3) Returns true if process is successfull otherwise false
    """

    policies_msg = _certify_cmessage(_policy_change_signature_message(), True)

    if SoC_socket.sendall(policies_msg) != None:
        return False
    
    print("Sent POLICIES message")
    _print_hex_array(policies_msg)
    print()

    # Wait for response
    certify_header = _recvall(SoC_socket, 48)
    payload_len = int.from_bytes(certify_header[8:12], byteorder='big')
    ack_msg = _recvall(SoC_socket, payload_len)

    # ACK is payload_type = 15
    if ack_msg[0] != 15: return False

    print("Received ACK message")
    _print_hex_array(certify_header + ack_msg)
    print()

    status = int.from_bytes(ack_msg[1:5], byteorder='big')
    if status != 1: return False

    return True

def rv_lenode_remote_attestation(SoC_socket : socket.socket) -> bool:
    """
    Perform a remote attestation (clg->rsp) of the RISCV le-node from [SoC_socket] connection.
    """
    
    # Sending the challenge message
    clg = randbytes(64)
    clg_msg = _certify_cmessage(_clg_message(clg), True, False)
    if SoC_socket.sendall(clg_msg) != None:
        return False
    
    print("Sent CLG message")
    _print_hex_array(clg_msg)
    print()

    # Wait for response
    certify_header = _recvall(SoC_socket, 48)
    payload_len = int.from_bytes(certify_header[8:12], byteorder='big')
    rsp_msg = _recvall(SoC_socket, payload_len)

    # RSP is payload_type = 14
    if rsp_msg[0] != 14: return False
    rsp = rsp_msg[16: 80]

    print("Received RSP message")
    _print_hex_array(certify_header + rsp_msg)
    print()

    # Check rsp is equal to signed clg
    clg_signed = hmac.new(bytes.fromhex(RV_LENODE_MSK), clg, hashlib.sha256).digest() + bytes(32)
    if rsp != clg_signed: return False

    return True
