#!/usr/bin/env python3
"""Exercise the real Asio connection/handshake against local TCP peers only."""
from pathlib import Path
import socket
import subprocess
import tempfile
import threading
import time

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='openstory-socket-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text(r'''
#include "Net/SocketAsio.h"
#include <cassert>
#include <chrono>
#include <cstring>
#include <thread>
int main(int argc, char** argv) {
    assert(argc == 2);
    ms::SocketAsio client;
    const auto start = std::chrono::steady_clock::now();
    assert(!client.open("127.0.0.1", argv[1])); // A silent peer must time out.
    assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(11));
    for (bool expected : {true, false, true}) {
        assert(client.open("127.0.0.1", argv[1]) == expected);
        if (!expected) continue; // EOF halfway through the handshake.
        assert(std::memcmp(client.get_buffer(), "0123456789abcdef", 16) == 0);
        bool connected = true;
        size_t size = 0;
        for (int attempt = 0; attempt < 100 && !size; ++attempt) {
            size = client.receive(&connected);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        assert(connected && size == 4);
        assert(std::memcmp(client.get_buffer(), "tail", 4) == 0);
        assert(client.close());
    }
}
''')
    for defines in ([], ['-DPLATFORM_PS5']):
        binary = work / 'check'
        subprocess.run(['c++', '-std=c++17', '-O1', '-pthread', '-DUSE_ASIO',
                        '-DASIO_STANDALONE', *defines, '-I' + str(root / 'src'),
                        str(source), str(root / 'src/Net/SocketAsio.cpp'),
                        '-o', str(binary)], check=True)
        failures = []
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen()
            listener.settimeout(15)
            def serve():
                try:
                    for mode in ('silent', 'fragmented', 'truncated', 'fragmented'):
                        with listener.accept()[0] as peer:
                            peer.settimeout(12)
                            if mode == 'truncated':
                                peer.sendall(b'01')
                                continue
                            if mode == 'fragmented':
                                peer.sendall(b'01')
                                time.sleep(0.03)
                                peer.sendall(b'23456789abcdeftail')
                            while peer.recv(1024):
                                pass
                except OSError as error:
                    failures.append(error)
            worker = threading.Thread(target=serve, daemon=True)
            worker.start()
            subprocess.run([binary, str(listener.getsockname()[1])], check=True, timeout=14)
            worker.join(timeout=2)
            assert not worker.is_alive() and not failures, failures
print('PASS: bounded handshake, fragmented input, no overread, and reconnect after failure')
