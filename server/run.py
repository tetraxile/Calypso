#!/usr/bin/env python3

import socket
import threading
from queue import Queue
from websockets.sync.server import serve, ServerConnection
from websockets.exceptions import ConnectionClosedOK
import http.server
import socketserver
# from http.server import SimpleHTTPRequestHandler

import os
os.chdir(os.path.dirname(__file__))


SWITCH_PORT = 8171
HTTP_PORT = 8172
WS_PORT = 8173


# ========== LOGGING ==========

# TODO: remove
_print = print
print = None

def log(user: str, message: str):
    _print(f"[{user}] {message}")


# ========== SWITCH SERVER ==========

def switch_send_func(client_sock: socket.socket, stop):
    while not stop():
        # message = input()
        # client_sock.send(bytes(message, "ascii"))
        # msg_queue.put(bytes(message, "ascii"))
        pass


# ========== HTTP SERVER ==========

class http_handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory="web", **kwargs)
    
    def log_message(self, format, *args):
        log("http", "{} - - [{}] {}".format(self.address_string(), self.log_date_time_string(), format % args))


class StoppableHTTPServer(http.server.HTTPServer):
    def run(self):
        try:
            log("http", f"serving at port {HTTP_PORT}")
            self.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            self.server_close()
            log("http", "server closed")


# ========== WEBSOCKET SERVER ==========

def ws_handler(websocket: ServerConnection):
    log("ws", f"received connection from client {websocket.id}")

    while True:
        try:
            message = websocket.recv(decode=False)
        except ConnectionClosedOK:
            log("ws", "client closed connection")
            break
        log("ws", f"client said: {message}")


def serve_ws(server):
    try:
        log("ws", f"serving at port {WS_PORT}")
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        log("ws", "server closed")


# ========== SWITCH SERVER ==========

def serve_switch():
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
        # send_thread = threading.Thread(target=switch_send_func, args=(client_sock, msg_queue))
        stop_switch_send_thread = False
        switch_send_thread = threading.Thread(target=switch_send_func, args=(client_sock, lambda: stop_switch_send_thread))
        switch_send_thread.start()

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
            stop_switch_send_thread = True
            switch_send_thread.join()
            log("switch", "send thread terminated")
            client_sock.close()


def main():
    # create http thread
    http_server = StoppableHTTPServer(("", HTTP_PORT), http_handler)
    http_thread = threading.Thread(target=http_server.run, args=())
    http_thread.start()

    # create websocket thread
    ws_server = serve(ws_handler, "localhost", WS_PORT)
    ws_thread = threading.Thread(target=serve_ws, args=(ws_server,))
    ws_thread.start()

    try:
        serve_switch()
    except KeyboardInterrupt:
        log("switch", "closing server...")
        http_server.shutdown()
        http_thread.join()
        ws_server.shutdown()
        ws_thread.join()


if __name__ == "__main__":
    main()
