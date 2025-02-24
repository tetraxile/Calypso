#!/usr/bin/env python3

import socket
import threading
from queue import Queue
from websockets.sync.server import serve, ServerConnection
from websockets.exceptions import ConnectionClosedOK
# from http.server import SimpleHTTPRequestHandler


SWITCH_PORT = 8171
WS_PORT = 8173

# TODO: remove
_print = print
print = None


def log(user: str, message: str):
    _print(f"[{user}] {message}")


def send_func(client_sock: socket.socket, stop):
    while not stop():
        # message = input()
        # client_sock.send(bytes(message, "ascii"))
        # msg_queue.put(bytes(message, "ascii"))
        pass


def ws_func():
    with serve(handler, "localhost", WS_PORT) as server:
        server.serve_forever()


def handler(websocket: ServerConnection):
    log("ws", f"received connection from client {websocket.id}")

    while True:
        try:
            message = websocket.recv(decode=False)
        except ConnectionClosedOK:
            log("ws", "client closed connection")
            break
        log("ws", f"client said: {message}")


def main():
    # create websocket thread
    ws_thread = threading.Thread(target=ws_func, args=())
    ws_thread.start()

    # create socket
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # bind socket to specified port
    server_address = ("0.0.0.0", SWITCH_PORT)
    server_sock.bind(server_address)

    # start listening on socket
    server_sock.listen(2)

    while True:
        log("switch", "waiting for connection...")
        client_sock, client_addr = server_sock.accept()
        log("switch", f"connection from {client_addr}")

        # create thread
        # msg_queue = Queue()
        # send_thread = threading.Thread(target=send_func, args=(client_sock, msg_queue))
        stop_thread = False
        send_thread = threading.Thread(target=send_func, args=(client_sock, lambda: stop_thread))
        send_thread.start()

        try:
            while True:
                data = client_sock.recv(1024).decode("ascii")
                log("switch", f"received {len(data)} bytes")
                if not data:
                    break

                log("switch", f"{client_addr} -> {data}")
                # while not msg_queue.empty():
                #     client_sock.send(msg_queue.get())

        finally:
            log("switch", "client disconnected")
            stop_thread = True
            send_thread.join()
            log("switch", "send thread terminated")
            client_sock.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        log("all", "exiting...")
