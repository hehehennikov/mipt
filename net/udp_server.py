import socket
import sys
import threading

class UDPServer:
    def __init__(self, host="0.0.0.0", port=50000):
        self.host = host
        self.port = int(port)
        self._shutdown = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))

    def serve_forever(self):
        try:
            while not self._shutdown.is_set():
                try:
                    data, addr = self.sock.recvfrom(65535)
                except OSError:
                    break
                if not data:
                    continue
                # echo
                try:
                    self.sock.sendto(data, addr)
                except Exception:
                    pass
                try:
                    prefix = f"[{addr[0]}:{addr[1]}] ".encode('utf-8')
                    sys.stdout.buffer.write(prefix + data)
                    sys.stdout.buffer.flush()
                except Exception:
                    pass
        finally:
            try:
                self.sock.close()
            except Exception:
                pass

    def shutdown(self):
        self._shutdown.set()
        try:
            self.sock.close()
        except Exception:
            pass