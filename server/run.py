#!/usr/bin/env python3

import enum
import http.server
import queue
import socket
import struct
import threading
import time
from datetime import datetime
from websockets.sync.server import serve, ServerConnection, Server as WsServer
from websockets.exceptions import ConnectionClosedOK

import os
os.chdir(os.path.dirname(__file__))

SWITCH_PORT = 8171
HTTP_PORT = 8172
WS_PORT = 8173

msg_queue = queue.Queue()


# ========== LOGGING ==========

# TODO: remove
_print = print
print = None

def log(user: str, message: str):
    timestamp = datetime.now().strftime("%H:%M:%S")
    _print(f"[{timestamp} - {user}] {message}")


# ========== PACKETS ==========

PACKET_HEADER_SIZE = 0x10

class PacketType(enum.IntEnum):
    Script = 0x01


class PacketScript:
    TYPENAME = "script"
    OPCODE = PacketType.Script.value

    def __init__(self, name: str, data: bytes):
        self.name = name
        self.data = data
    
    def construct_header(self) -> bytearray:
        out = bytearray(PACKET_HEADER_SIZE)
        out[0] = self.OPCODE
        out[1] = min(len(self.name), 0xff)
        out[4:8] = struct.pack("!I", len(self.data))
        return out

    def construct(self) -> bytearray:
        out = bytearray()
        out.extend(self.construct_header())
        out.extend(struct.pack("255s", bytes(self.name, "utf-8")))
        out.extend(self.data)
        return out


# class PacketScriptInfo:
#     TYPENAME = "script info"
#     OPCODE = PacketType.ScriptInfo.value

#     def __init__(self, name: str):
#         self.name = name
    
#     def construct_header(self) -> bytearray:
#         out = bytearray(PACKET_HEADER_SIZE)
#         out[0] = self.OPCODE
#         out[1] = min(len(self.name), 0xff)
#         return out

#     def construct(self) -> bytearray:
#         out = bytearray()
#         out.extend(self.construct_header())
#         out.extend(bytes(self.name, "utf-8")[:0xff])
#         return out


# ========== SWITCH SERVER ==========

def switch_send_func(client_sock: socket.socket, stop):
    while not stop():
        try:
            packet = msg_queue.get(timeout=0.5)
        except queue.Empty:
            continue

        log("switch", f"sending packet type `{packet.TYPENAME}`")
        client_sock.send(packet.construct())


# ========== HTTP SERVER ==========

class http_handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory="web", **kwargs)
    
    def log_message(self, format, *args):
        log("http", format % args)


class StoppableHTTPServer(http.server.HTTPServer):
    def run(self):
        try:
            log("http", f"listening on port {HTTP_PORT}")
            self.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            self.server_close()
            log("http", "server closed")


# ========== WEBSOCKET SERVER ==========

def ws_recv_packet(websocket: ServerConnection):
    packet_type = websocket.recv()
    if not packet_type.startswith("type: "):
        log("ws", f"malformed packet type: `{packet_type}`")
        log("ws", "continuing...")
    packet_type = packet_type.removeprefix("type: ")

    log("ws", f"received packet type {repr(packet_type)}")

    if packet_type == "script":
        script_len = websocket.recv()
        script_filename = websocket.recv()
        script_data = websocket.recv(decode=False)
        log("ws", f"\tscript name: {script_filename}")
        log("ws", f"\tscript len: {script_len}")
        msg_queue.put(PacketScript(script_filename, script_data))
    else:
        log("ws", f"unsupported packet type {repr(packet_type)}")


def ws_handler(websocket: ServerConnection):
    log("ws", f"received connection from client {websocket.id}")

    while True:
        try:
            ws_recv_packet(websocket)
        except ConnectionClosedOK:
            log("ws", "client closed connection")
            break


def serve_ws(server: WsServer):
    try:
        log("ws", f"listening on port {WS_PORT}")
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
    log("switch", f"listening on port {SWITCH_PORT}")

    while True:
        log("switch", "waiting for connection...")
        client_sock, client_addr = server_sock.accept()
        log("switch", f"connection from {client_addr}")

        # create thread
        stop_switch_send_thread = False
        switch_send_thread = threading.Thread(target=switch_send_func, args=(client_sock, lambda: stop_switch_send_thread))
        switch_send_thread.start()

        try:
            while True:
                data = client_sock.recv(1024).decode("utf-8")
                if not data:
                    log("switch", f"received 0 bytes")
                    break

                log("switch", f"{client_addr} -> {data}")
                # while not msg_queue.empty():
                #     client_sock.send(msg_queue.get())

        except ConnectionResetError:
            log("switch", "connection reset")
            break

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
