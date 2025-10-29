import socket
import sys
import threading
import time

class TCPClient:
    def __init__(self, host="127.0.0.1", port=50000, timeout=5):
        self.host = host
        self.port = int(port)
        self.timeout = timeout

    def send_and_recv(self, data: bytes, expect_len=None, timeout=10):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((self.host, self.port))
            sock.settimeout(1.0)
            total_sent = 0
            while total_sent < len(data):
                sent = sock.send(data[total_sent:])
                if sent == 0:
                    raise RuntimeError("socket connection broken")
                total_sent += sent
            received = bytearray()
            start = time.time()
            while expect_len is None or len(received) < expect_len:
                if timeout and (time.time() - start) > timeout:
                    break
                try:
                    chunk = sock.recv(4096)
                except socket.timeout:
                    continue
                if not chunk:
                    break
                received.extend(chunk)
            return bytes(received)
        finally:
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            sock.close()

    def run_interactive(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.host, self.port))
        sock.settimeout(1.0)
        stop = threading.Event()

        def receiver():
            try:
                while not stop.is_set():
                    try:
                        data = sock.recv(4096)
                    except socket.timeout:
                        continue
                    if not data:
                        break
                    try:
                        sys.stdout.buffer.write(data)
                        sys.stdout.buffer.flush()
                    except Exception:
                        pass
            finally:
                stop.set()

        t = threading.Thread(target=receiver, daemon=True)
        t.start()
        try:
            while not stop.is_set():
                line = sys.stdin.buffer.readline()
                if not line:
                    break
                try:
                    sock.sendall(line)
                except Exception:
                    break
        finally:
            stop.set()
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            sock.close()
            t.join(timeout=1)
