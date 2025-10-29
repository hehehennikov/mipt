import sys
import click
from .tcp_server import TCPServer
from .tcp_client import TCPClient
from .udp_server import UDPServer
from .udp_client import UDPClient

@click.group()
def cli():
    pass

@cli.command(name="tcp-server")
@click.option('--host', default='0.0.0.0', show_default=True, help='Host to bind')
@click.option('--port', default=50000, show_default=True, type=int, help='Port to bind')
@click.option('--broadcast-stdin', is_flag=True, help='Read server stdin and broadcast to all clients')
def tcp_server(host, port, broadcast_stdin):
    srv = TCPServer(host, port)
    try:
        srv.serve_forever(broadcast_stdin=broadcast_stdin)
    except KeyboardInterrupt:
        pass
    finally:
        srv.shutdown()

@cli.command(name="tcp-client")
@click.option('--host', default='127.0.0.1', show_default=True)
@click.option('--port', default=50000, show_default=True, type=int)
@click.option('--message', default=None, help='Message to send; if omitted, runs interactive mode')
@click.option('--interactive', is_flag=True, help='Force interactive mode')
def tcp_client(host, port, message, interactive):
    client = TCPClient(host, port)
    if interactive or message is None:
        client.run_interactive()
    else:
        data = message if isinstance(message, (bytes, bytearray)) else message.encode()
        resp = client.send_and_recv(data, expect_len=len(data))
        sys.stdout.buffer.write(resp)
        sys.stdout.buffer.flush()

@cli.command(name="udp-server")
@click.option('--host', default='0.0.0.0', show_default=True)
@click.option('--port', default=50000, show_default=True, type=int)
def udp_server(host, port):
    srv = UDPServer(host, port)
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        srv.shutdown()

@cli.command(name="udp-client")
@click.option('--host', default='127.0.0.1', show_default=True)
@click.option('--port', default=50000, show_default=True, type=int)
@click.option('--message', default=None, help='Message to send; if omitted, runs interactive mode (reads stdin)')
@click.option('--interactive', is_flag=True, help='Force interactive mode')
def udp_client(host, port, message, interactive):
    client = UDPClient(host, port)
    if interactive or message is None:
        client.run_interactive()
    else:
        data = message if isinstance(message, (bytes, bytearray)) else message.encode()
        resp = client.send_and_recv(data)
        if resp:
            sys.stdout.buffer.write(resp)
            sys.stdout.buffer.flush()

@cli.command(name="test-huge")
@click.option('--host', default='127.0.0.1', show_default=True)
@click.option('--port', default=50000, show_default=True, type=int)
@click.option('--size', default=50000, show_default=True, type=int)
def test_huge(host, port, size):
    from .tcp_server import TCPServer as _TCPServer
    from .tcp_client import TCPClient as _TCPClient
    import threading
    import time

    srv = _TCPServer(host, port)
    server_thread = threading.Thread(target=srv.serve_forever, daemon=True)
    server_thread.start()
    time.sleep(0.2)
    payload = b'A' * size
    client = _TCPClient(host, port)
    resp = client.send_and_recv(payload, expect_len=len(payload), timeout=10)
    srv.shutdown()
    server_thread.join(timeout=1)
    if resp == payload:
        click.echo('OK')
        raise SystemExit(0)
    else:
        click.echo('FAIL')
        raise SystemExit(2)

if __name__ == '__main__':
    cli()