import socket

# Configuración
HOST = '0.0.0.0'   # Escuchar en todas las interfaces
PORT = 12345       # Cambia este puerto si lo necesitas

# Crear socket TCP
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(1)

print(f"Servidor escuchando en puerto {PORT}...")

try:
    while True:
        conn, addr = server_socket.accept()
        print(f"Conexión entrante de {addr}")

        with conn:
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                # Imprimir mensaje en hexadecimal
                print("Mensaje recibido (hex):", data.hex())

except KeyboardInterrupt:
    print("\nServidor detenido manualmente.")
finally:
    server_socket.close()

