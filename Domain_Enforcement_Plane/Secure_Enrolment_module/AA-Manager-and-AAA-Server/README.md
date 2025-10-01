# Demo RFC-4764 - EAP-PSK

This repository contains a demonstration implementation for RFC 4764. The project includes three C program files that need to be compiled and executed in a specific sequence for the demonstration to work correctly.

## Dependencies

If you encounter any issues during compilation, you may need to install the required libraries and dependencies. Ensure the following are installed:

- `gcc`
- `libcoap-3`
- `libcrypto`
- `libssl`
- `libcurl`
- `libcjson`
- `libuuid`
- `libmicrohttpd`

On a Debian-based system, you can install the dependencies using:
```bash
sudo apt-get install gcc libcoap3-dev libssl-dev libcurl4-openssl-dev libcjson-dev libmicrohttpd-dev uuid-dev python3 python3-flask
```

## Files

1. `aa_manager.c`        - The authenticator implementation.
2. `domain_server.c`     - The domain's AAA server implementation.
3. `aaa_server.c`        - The manufacturer's AAA server implementation.

## Compilation

To compile the programs, execute the following commands in your terminal:

1. Compile the AA Manager:
   ```bash
   gcc aa_manager.c -o aa_manager -lcrypto -lssl -lcurl -lcjson -lmicrohttpd
   ```
   
2. Compile the manufacturer's AAA server:
   ```bash
   gcc aaa_server.c -o aaa_server -lcrypto -lssl -lcjson
   ```

3. Compile the domain's AAA server:
   ```bash
   gcc  domain_aaa_server.c -o domain_aaa_server -lcrypto -lssl
   ```
   
4. Generate a self-signed certificate (for testing):
   ```bash
   openssl req -new -x509 -days 365 -nodes -out server.crt -keyout server.key
   ```

## Execution Sequence

The programs must be run in the following order:

1. Start the domain's AAA server:
   ```bash
   ./domain_aaa_server
   ```

2. Start the manufacturer's AAA server:
   ```bash
   ./aaa_server
   ```
   
4. Finally, run the aa_manager with the correct parameters:
   ```bash
   python3 bootstrapping_request_manager.py
   ```
   
5. For a quick test, you can use post_server.
   ```bash
   gcc post_server.c -o post_server -lcurl -lcjson -lmicrohttpd
   ./post_server
   ```

Ensure each program is running before starting the next to maintain the correct sequence and communication.

## Notes

- This project demonstrates the principles outlined in RFC 4764 with some modifications.
- Modify the source code as needed to suit specific testing or demonstration requirements.
