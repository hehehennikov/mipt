import socket
import threading
import sys
import traceback

class TCPServer:
    def __init__(self, host="0.0.0.0", port=50000, backlog=5):
        self.host = host
        self.port = int(port)
        self.backlog = backlog
        self._shutdown = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        self.sock.listen(self.backlog)

        self._clients = {}
        self._clients_lock = threading.Lock()
        self._stdin_thread = None

    def serve_forever(self, broadcast_stdin=False):
        if broadcast_stdin:
            self._stdin_thread = threading.Thread(target=self._stdin_broadcast_loop, daemon=True)
            self._stdin_thread.start()

        try:
            while not self._shutdown.is_set():
                try:
                    conn, addr = self.sock.accept()
                except OSError:
                    break
                t = threading.Thread(target=self._handle_client, args=(conn, addr), daemon=True)
                with self._clients_lock:
                    self._clients[addr] = {"conn": conn, "thread": t, "stop": threading.Event()}
                t.start()
        finally:
            self.shutdown()

    def _handle_client(self, conn, addr):
        stop = self._clients.get(addr, {}).get("stop")
        if stop is None:
            stop = threading.Event()
        try:
            conn.settimeout(0.5)
            while not self._shutdown.is_set() and not stop.is_set():
                try:
                    data = conn.recv(4096)
                except socket.timeout:
                    continue
                except OSError:
                    break
                if not data:
                    break
                try:
                    conn.sendall(data)
                except Exception:
                    break
                try:
                    prefix = f"[{addr[0]}:{addr[1]}] ".encode('utf-8')
                    sys.stdout.buffer.write(prefix + data)
                    sys.stdout.buffer.flush()
                except Exception:
                    pass
        except Exception:
            traceback.print_exc()
        finally:
            try:
                conn.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            try:
                conn.close()
            except Exception:
                pass
            with self._clients_lock:
                self._clients.pop(addr, None)

    def broadcast(self, data: bytes):
        if isinstance(data, str):
            data = data.encode()
        with self._clients_lock:
            for info in list(self._clients.values()):
                conn = info.get("conn")
                try:
                    conn.sendall(data)
                except Exception:
                    pass

    def _stdin_broadcast_loop(self):
        try:
            while not self._shutdown.is_set():
                line = sys.stdin.buffer.readline()
                if not line:
                    break
                self.broadcast(line)
        except Exception:
            pass

    def shutdown(self):
        self._shutdown.set()
        try:
            self.sock.close()
        except Exception:
            pass
        with self._clients_lock:
            for addr, info in list(self._clients.items()):
                info.get("stop", threading.Event()).set()
                conn = info.get("conn")
                try:
                    conn.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                try:
                    conn.close()
                except Exception:
                    pass
            self._clients.clear()
