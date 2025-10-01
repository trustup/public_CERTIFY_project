import subprocess
import datetime

def high_end_bootstrap(mudUrl, client_socket, high_end_key):

    ip, port = client_socket.getpeername()
    device = "high_end_device-" + str(ip)

    # Responses in hex ("OK" / "KO")
    resp = bytes.fromhex('4f4b')
    ko_resp = bytes.fromhex('4b4f')
    client_socket.sendall(resp)

    # Build the command and arguments
    aa_manager = ["./aa_manager", high_end_key, device, str(ip), mudUrl, "4444"]

    try:
        # Run the C program and capture output
        run_result = subprocess.run(aa_manager, capture_output=True, text=True)

        # Write the output to a log file
        with open("aa_manager_log.txt", "a") as log_file:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            log_file.write(f"\n[{timestamp}] Executed command: {' '.join(aa_manager)}\n")
            log_file.write(f"[{timestamp}] Return code: {run_result.returncode}\n")
            log_file.write(f"[{timestamp}] STDOUT:\n{run_result.stdout}\n")
            log_file.write(f"[{timestamp}] STDERR:\n{run_result.stderr}\n")

    except Exception as e:
        print("Error calling aa_manager:", e)
        # Also write the exception to the log file
        with open("aa_manager_log.txt", "a") as log_file:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            log_file.write(f"\n[{timestamp}] Error calling aa_manager: {str(e)}\n")

        client_socket.sendall(ko_resp)
        return False

    if run_result.returncode == 0:
        output = run_result.stdout.strip()
        print("C program output captured:\n", output)

        # Check if authentication was successful
        if "Authentication finished!" in output:
            print("Authentication was successful.")
            client_socket.sendall(ko_resp)
            return True
        else:
            print("Authentication failed or not completed.")
            client_socket.sendall(ko_resp)
            return False
    else:
        print("C program execution failed:")
        print(run_result.stderr)
        client_socket.sendall(ko_resp)
        return False
