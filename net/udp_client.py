import socket
import time
import sys
import threading

class UDPClient:
    def __init__(self, host="127.0.0.1", port=50000, timeout=2):
        self.host = host
        self.port = int(port)
        self.timeout = timeout

    def send_and_recv(self, data, timeout=5):
        if isinstance(data, str):
            data = data.encode()
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(self.timeout)
        try:
            sock.sendto(data, (self.host, self.port))
            start = time.time()
            while True:
                if timeout and (time.time() - start) > timeout:
                    return b""
                try:
                    chunk, addr = sock.recvfrom(65535)
                except socket.timeout:
                    continue
                if not chunk:
                    return b""
                return chunk
        finally:
            sock.close()

    def run_interactive(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("", 0))
        sock.settimeout(1.0)
        stop = threading.Event()

        def receiver():
            try:
                while not stop.is_set():
                    try:
                        chunk, addr = sock.recvfrom(65535)
                    except socket.timeout:
                        continue
                    except OSError:
                        break
                    if not chunk:
                        break
                    try:
                        # Prefix with sender address to help debugging
                        prefix = f"[{addr[0]}:{addr[1]}] ".encode("utf-8")
                        sys.stdout.buffer.write(prefix + chunk)
                        sys.stdout.buffer.flush()
                    except Exception:
                        pass
            finally:
                stop.set()

        t = threading.Thread(target=receiver, daemon=True)
        t.start()

        try:
            while not stop.is_set():
                try:
                    line = sys.stdin.buffer.readline()
                except Exception:
                    break
                if not line:
                    break
                try:
                    sock.sendto(line, (self.host, self.port))
                except Exception:
                    break
        finally:
            stop.set()
            try:
                sock.close()
            except Exception:
                pass
            t.join(timeout=1)