#!/usr/bin/python3

# Burst 3 packets with timestamp every second to an echo server.
# Used for monitoring network quality.
# Usage: ./udp_echo_client.py <server ip> <server port> <tcp|udp>
# TCP doesn't work unless you reduce the burst count to 1.

import struct
import sys
import os
import socket
from threading import Thread
import time

SERVER_IP = sys.argv[1]
SERVER_PORT = int(sys.argv[2])
PROTO = sys.argv[3]

EXITING = False

RCVD_CNT = 0
SENT_CNT = 0
TOTAL_LATENCY = 0

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM if "tcp" == PROTO else socket.SOCK_DGRAM)
s.connect((SERVER_IP, SERVER_PORT))
s.settimeout(1)

if PROTO == "tcp":
    s.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))

def recv_print(**kwargs):
    global RCVD_CNT
    global TOTAL_LATENCY
    sk = kwargs["socket"]
    while not EXITING:
        try:
            _d = sk.recv(1024)
        except socket.timeout:
            print("RECV TIMEOUT")
            continue
        if len(_d) == 0:
            time.sleep(1)
            continue
        si = int.from_bytes(_d[0:4], byteorder="little")
        sd = int.from_bytes(_d[4:12], byteorder="little")
        RCVD_CNT = RCVD_CNT + 1
        TOTAL_LATENCY = TOTAL_LATENCY + time.time_ns() - sd
        print(f"[ID {si}] Round trip time: {(time.time_ns() - sd) / 1000000:.2f} ms, sent {SENT_CNT}, rcvd {RCVD_CNT}, loss {(1 - RCVD_CNT / SENT_CNT)*100:.2f}%, avg latency {TOTAL_LATENCY / RCVD_CNT / 1000000:.2f} ms")

def send_i(i):
    global SENT_CNT
    d = i.to_bytes(4, byteorder="little") + \
        time.time_ns().to_bytes(8, byteorder="little")
    SENT_CNT = SENT_CNT + 1
    if os.name == "posix" and PROTO == "tcp":
        s.sendmsg([d], [(0, 0, b"")], socket.MSG_EOR)
    else:
        s.sendto(d, (SERVER_IP, SERVER_PORT))

thr = Thread(target=recv_print, kwargs={"socket": s})
thr.start()

i = 0
while True:
    send_i(i)
    i += 1
    send_i(i)
    i += 1
    send_i(i)
    i += 1

    try:
        time.sleep(1)
    except KeyboardInterrupt:
        EXITING = True
        print("Stopping...")
        break

thr.join()
