import argparse
import socket
import threading
import sys
import time

class TCPServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self._shutdown = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        self.sock.listen(1)

    def serve_forever(self):
        try:
            while not self._shutdown.is_set():
                try:
                    conn, addr = self.sock.accept()
                except OSError:
                    break
                handler = threading.Thread(target=self._handle_client, args=(conn, addr), daemon=True)
                handler.start()
                handler.join()
        finally:
            self.sock.close()

    def _handle_client(self, conn, addr):
        conn.settimeout(0.5)
        stop = threading.Event()
        def recv_loop():
            try:
                while not stop.is_set():
                    try:
                        data = conn.recv(4096)
                    except socket.timeout:
                        continue
                    if not data:
                        break
                    try:
                        conn.sendall(data)
                    except OSError:
                        break
                    try:
                        sys.stdout.buffer.write(data)
                        sys.stdout.buffer.flush()
                    except Exception:
                        pass
            finally:
                stop.set()
                try:
                    conn.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                conn.close()
        t = threading.Thread(target=recv_loop, daemon=True)
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
                    conn.sendall(line)
                except Exception:
                    break
        finally:
            stop.set()
            t.join(timeout=1)

    def shutdown(self):
        self._shutdown.set()
        try:
            self.sock.close()
        except Exception:
            pass

class TCPClient:
    def __init__(self, host, port, interactive=False):
        self.host = host
        self.port = port
        self.interactive = interactive
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5)

    def run_interactive(self):
        self.sock.connect((self.host, self.port))
        stop = threading.Event()
        def recv_loop():
            try:
                while not stop.is_set():
                    try:
                        data = self.sock.recv(4096)
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
        t = threading.Thread(target=recv_loop, daemon=True)
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
                    self.sock.sendall(line)
                except Exception:
                    break
        finally:
            stop.set()
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            self.sock.close()
            t.join(timeout=1)

    def send_and_recv(self, data, expect_len=None, timeout=10):
        self.sock.connect((self.host, self.port))
        self.sock.settimeout(1)
        total_sent = 0
        while total_sent < len(data):
            sent = self.sock.send(data[total_sent:])
            if sent == 0:
                raise RuntimeError("socket connection broken")
            total_sent += sent
        received = bytearray()
        start = time.time()
        while expect_len is None or len(received) < expect_len:
            if timeout and (time.time() - start) > timeout:
                break
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            received.extend(chunk)
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except Exception:
            pass
        self.sock.close()
        return bytes(received)

class UDPServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self._shutdown = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        self.last_addr = None

    def serve_forever(self):
        try:
            while not self._shutdown.is_set():
                try:
                    data, addr = self.sock.recvfrom(65535)
                except OSError:
                    break
                if not data:
                    continue
                self.last_addr = addr
                try:
                    self.sock.sendto(data, addr)
                except Exception:
                    pass
                try:
                    sys.stdout.buffer.write(data)
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

class UDPClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(2)

    def send_and_recv(self, data, timeout=5):
        self.sock.sendto(data, (self.host, self.port))
        received = bytearray()
        start = time.time()
        while True:
            if timeout and (time.time() - start) > timeout:
                break
            try:
                chunk, addr = self.sock.recvfrom(65535)
            except socket.timeout:
                continue
            if not chunk:
                break
            received.extend(chunk)
            break
        return bytes(received)

def run_tcp_server(host, port):
    srv = TCPServer(host, port)
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        srv.shutdown()

def run_tcp_client(host, port, message, interactive=False):
    c = TCPClient(host, port, interactive=interactive)
    if interactive:
        c.run_interactive()
    else:
        data = message.encode() if isinstance(message, str) else message
        resp = c.send_and_recv(data, expect_len=len(data))
        sys.stdout.buffer.write(resp)
        sys.stdout.buffer.flush()

def run_udp_server(host, port):
    srv = UDPServer(host, port)
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        srv.shutdown()

def run_udp_client(host, port, message):
    c = UDPClient(host, port)
    data = message.encode() if isinstance(message, str) else message
    resp = c.send_and_recv(data)
    if resp:
        sys.stdout.buffer.write(resp)
        sys.stdout.buffer.flush()

def test_huge_tcp(local_host, port=50000, size=50000):
    srv = TCPServer(local_host, port)
    server_thread = threading.Thread(target=srv.serve_forever, daemon=True)
    server_thread.start()
    time.sleep(0.2)
    payload = b'A' * size
    client = TCPClient(local_host, port)
    resp = client.send_and_recv(payload, expect_len=len(payload), timeout=10)
    srv.shutdown()
    server_thread.join(timeout=1)
    if resp == payload:
        print("OK")
        return 0
    else:
        print("FAIL")
        return 2

def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest='mode')
    sp = sub.add_parser('tcp-server')
    sp.add_argument('--host', default='0.0.0.0')
    sp.add_argument('--port', type=int, default=50000)
    spc = sub.add_parser('tcp-client')
    spc.add_argument('--host', default='127.0.0.1')
    spc.add_argument('--port', type=int, default=50000)
    spc.add_argument('--message', default=None)
    spc.add_argument('--interactive', action='store_true')
    sup = sub.add_parser('udp-server')
    sup.add_argument('--host', default='0.0.0.0')
    sup.add_argument('--port', type=int, default=50000)
    supc = sub.add_parser('udp-client')
    supc.add_argument('--host', default='127.0.0.1')
    supc.add_argument('--port', type=int, default=50000)
    supc.add_argument('--message', default=None)
    st = sub.add_parser('test-huge')
    st.add_argument('--host', default='127.0.0.1')
    st.add_argument('--port', type=int, default=50000)
    st.add_argument('--size', type=int, default=50000)
    args = parser.parse_args()
    if args.mode == 'tcp-server':
        run_tcp_server(args.host, args.port)
    elif args.mode == 'tcp-client':
        if args.interactive or args.message is None:
            run_tcp_client(args.host, args.port, '', interactive=True)
        else:
            run_tcp_client(args.host, args.port, args.message.encode() if isinstance(args.message, str) else args.message)
    elif args.mode == 'udp-server':
        run_udp_server(args.host, args.port)
    elif args.mode == 'udp-client':
        if args.message is None:
            buf = sys.stdin.buffer.read()
            run_udp_client(args.host, args.port, buf)
        else:
            run_udp_client(args.host, args.port, args.message)
    elif args.mode == 'test-huge':
        rc = test_huge_tcp(args.host, args.port, size=args.size)
        sys.exit(rc)
    else:
        parser.print_help()

if __name__ == '__main__':
    main()