#!/usr/bin/env python3
"""Test real socket backends on loopback with kernel socket() denied, as on PS5.

Run on Linux/WSL. --backend asio reproduces the old EACCES failure (nonzero exit).
"""
import argparse
from pathlib import Path
import socket
import subprocess
import tempfile
import threading
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--backend', choices=('ps5', 'asio'), default='ps5')
args = parser.parse_args()
backend = 'SocketPS5' if args.backend == 'ps5' else 'SocketAsio'
root = Path(__file__).resolve().parents[1]
payload = bytes(index % 251 for index in range(131072 + 31))

with tempfile.TemporaryDirectory(prefix='openstory-ps5-socket-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text(r'''
#include "Net/BACKEND.h"
#include <cassert>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>
int main(int argc, char** argv) {
    assert(argc == 2);
    ms::BACKEND client;
    std::vector<int8_t> payload(ms::MAX_PACKET_LENGTH + 31);
    for (size_t index = 0; index < payload.size(); ++index) payload[index] = index % 251;
    auto exchange = [&] {
        assert(client.open("127.0.0.1", argv[1]));
        assert(std::memcmp(client.get_buffer(), "0123456789abcdef", 16) == 0);
        bool connected = true;
        size_t size = 0;
        for (int attempt = 0; attempt < 200 && !size; ++attempt) {
            size = client.receive(&connected);
            if (!size) std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        assert(connected && size == 4);
        assert(std::memcmp(client.get_buffer(), "tail", 4) == 0);
        const auto idle = std::chrono::steady_clock::now();
        assert(client.receive(&connected) == 0 && connected);
        assert(std::chrono::steady_clock::now() - idle < std::chrono::milliseconds(200));
        assert(client.dispatch(payload.data(), payload.size()));
        for (int attempt = 0; attempt < 400 && connected; ++attempt) {
            assert(client.receive(&connected) == 0);
            if (connected) std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        assert(!connected); // A peer's orderly EOF must reach the session.
        assert(client.close());
    };
    exchange(); // Coalesced handshake + packet also reproduces old kernel EACCES.
    const auto start = std::chrono::steady_clock::now();
    assert(!client.open("127.0.0.1", argv[1]));
    const auto elapsed = std::chrono::steady_clock::now() - start;
    assert(elapsed >= std::chrono::seconds(7) && elapsed < std::chrono::seconds(11));
    exchange(); // Reconnect after a silent handshake timeout, with fragmented input.
    assert(!client.open("127.0.0.1", argv[1])); // EOF within a handshake.
    exchange(); // Reconnect after that failure too.
}
'''.replace('BACKEND', backend))
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-O1', '-pthread', '-DUSE_ASIO',
                    '-DASIO_STANDALONE', '-DPLATFORM_PS5', '-DOPENSTORY_LAN_LOG',
                    '-I' + str(root / 'src'), str(source),
                    str(root / 'src/Net' / (backend + '.cpp')),
                    str(root / 'tests/ps5-socket-shim.cpp'), '-Wl,--wrap=socket',
                    '-o', str(binary)], check=True)
    failures = []
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        listener.listen()
        listener.settimeout(15)

        def serve():
            try:
                for mode in ('coalesced', 'silent', 'fragmented', 'truncated', 'coalesced'):
                    with listener.accept()[0] as peer:
                        peer.settimeout(12)
                        if mode == 'silent':
                            assert peer.recv(1) == b''
                            continue
                        if mode == 'truncated':
                            peer.sendall(b'01')
                            continue
                        if mode == 'fragmented':
                            peer.sendall(b'01')
                            time.sleep(0.03)
                            peer.sendall(b'23456789abcdeftail')
                        else:
                            peer.sendall(b'0123456789abcdeftail')
                        received = bytearray()
                        while len(received) < len(payload):
                            chunk = peer.recv(len(payload) - len(received))
                            assert chunk, 'Client closed before sending the entire payload'
                            received.extend(chunk)
                        assert received == payload, 'Dispatch changed the payload'
                        peer.shutdown(socket.SHUT_WR)
                        assert peer.recv(1) == b'', 'Dispatch sent extra bytes'
            except (OSError, AssertionError) as error:
                failures.append(error)

        worker = threading.Thread(target=serve, daemon=True)
        worker.start()
        subprocess.run([binary, str(listener.getsockname()[1])], check=True,
                       cwd=work, timeout=20)
        worker.join(timeout=2)
        assert not worker.is_alive() and not failures, failures
print('PASS: sceNet works with kernel socket denied; bounded/fragmented/truncated/coalesced '
      'handshakes, no overread, reconnect, nonblocking receive, EOF, and exact partial sends')
