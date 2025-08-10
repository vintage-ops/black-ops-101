import socket
import threading
import pickle

class ControlServer:
    def __init__(self, host='0.0.0.0', port=9999):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((host, port))
        self.clients = []
        
    def handle_client(self, conn):
        while True:
            cmd = input("Enter command: ")
            conn.send(cmd.encode())
            response = conn.recv(4096)
            print(response.decode())
            
    def start(self):
        self.sock.listen(5)
        print("[+] Listening for connections...")
        
        while True:
            conn, addr = self.sock.accept()
            print(f"[+] Connection from {addr}")
            client_thread = threading.Thread(target=self.handle_client, args=(conn,))
            client_thread.start()

if __name__ == '__main__':
    server = ControlServer()
    server.start()