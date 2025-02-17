#!/usr/bin/env python3

import argparse
import socket
import threading
from queue import Queue

from http.server import SimpleHTTPRequestHandler


def port_parser(value: str) -> int:
    ivalue = int(value)
    if ivalue not in range(0, 65536):
        raise argparse.ArgumentTypeError("port must be in range [0, 65535]")
    return ivalue


def send_func(client_sock: socket.socket, stop):
    while not stop():
        # message = input()
        # client_sock.send(bytes(message, "ascii"))
        # msg_queue.put(bytes(message, "ascii"))
        pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", help="[default: 8171]", type=port_parser, default="8171")

    args = parser.parse_args()

    # create socket
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # bind socket to specified port
    server_address = ("0.0.0.0", args.port)
    server_sock.bind(server_address)

    # start listening on socket
    server_sock.listen(2)

    while True:
        print("waiting for connection...")
        client_sock, client_addr = server_sock.accept()
        print(f"connection from {client_addr}")

        # create thread
        # msg_queue = Queue()
        # send_thread = threading.Thread(target=send_func, args=(client_sock, msg_queue))
        stop_thread = False
        send_thread = threading.Thread(target=send_func, args=(client_sock, lambda: stop_thread))
        send_thread.start()

        try:
            while True:
                data = client_sock.recv(1024).decode("ascii")
                print(f"received {len(data)} bytes")
                if not data:
                    break

                print(f"{client_addr} -> {data}")
                # while not msg_queue.empty():
                #     client_sock.send(msg_queue.get())

        finally:
            print("client disconnected")
            stop_thread = True
            send_thread.join()
            print("send thread terminated")
            client_sock.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("exiting...")
