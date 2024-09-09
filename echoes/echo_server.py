#!/usr/bin/python3

# A simple echo server. See udp_echo_client.py
# Usage: ./echo_server.py <port> <tcp|udp>

import struct
import sys
import socket
import threading

def handle_client(client_socket):
    with client_socket:
        client_socket.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
        while True:
            data = client_socket.recv(1024)
            if not data or len(data) == 0:
                print("Client exit")
                break
            print("Received from client: len " + str(len(data)))
            client_socket.sendall(data)  # Echo the data back to the client

proto = sys.argv[2]

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM if proto == "udp" else socket.SOCK_STREAM)

server_address = '0.0.0.0'
server_port = int(sys.argv[1])

server = (server_address, server_port)
sock.bind(server)
print("Listening on " + server_address + ":" + str(server_port))

if proto == "udp":
    while True:
        payload, client_address = sock.recvfrom(1024)
        print("Send to %s %s" % (client_address[0], client_address[1]))
        sent = sock.sendto(payload, client_address)
else:
    sock.listen(100)
    while True:
        client_socket, addr = sock.accept()
        print("Connection established with " + str(addr))
        threading.Thread(target=handle_client, args=(client_socket,)).start()
