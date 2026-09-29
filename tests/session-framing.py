#!/usr/bin/env python3
"""Run real Session/Cryptography against valid small frames and an in-memory socket."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1])
root = parser.parse_args().source_root.resolve()
with tempfile.TemporaryDirectory(prefix='openstory-session-') as directory:
    work = Path(directory)
    src = work / 'src'
    for name in ('Net/Session.h', 'Net/Session.cpp', 'Net/Cryptography.h',
                 'Net/Cryptography.cpp', 'Net/NetConstants.h', 'MapleStory.h',
                 'Error.h', 'Template/Singleton.h'):
        target = src / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / 'src' / name, target)
    platform_config = Path('platform/shared/PlatformConfig.h')
    if (root / platform_config).exists():
        (work / platform_config).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / platform_config, work / platform_config)
    (src / 'Configuration.h').write_text(r'''
#pragma once
#include <string>
namespace ms {
struct ServerIP {};
struct ServerPort {};
template<class T> struct Setting {
    static Setting& get() { static Setting setting; return setting; }
    std::string load() const { return "in-memory"; }
};
}
''')
    (src / 'Net/PacketSwitch.h').write_text(r'''
#pragma once
#include "NetConstants.h"
#include <functional>
#include <stdexcept>
#include <vector>
namespace ms {
inline std::vector<std::vector<int8_t>> delivered;
inline std::function<void()> after_packet;
struct PacketError : std::runtime_error { using std::runtime_error::runtime_error; };
struct PacketSwitch {
    void forward(const int8_t* bytes, size_t length) const {
        delivered.emplace_back(bytes, bytes + length);
        if (after_packet) after_packet();
    }
};
}
''')
    (src / 'Net/SocketPS5.h').write_text(r'''
#pragma once
#include "NetConstants.h"
#include <array>
#include <cstring>
#include <vector>
namespace ms {
// v83 handshake with equal send/receive IVs so real Cryptography can act as the peer.
inline const std::array<int8_t, 16> handshake{14, 0, 83, 0, 1, 0, '1', 1, 2, 3, 4, 1, 2, 3, 4, 8};
inline std::vector<int8_t> input;
class SocketPS5 {
    std::array<int8_t, MAX_PACKET_LENGTH> buffer{};
    bool handshaking = false;
public:
    bool open(const char*, const char*) {
        handshaking = true;
        return true;
    }
    bool close() { return true; }
    size_t receive(bool*) {
        handshaking = false;
        const size_t length = input.size();
        std::memcpy(buffer.data(), input.data(), length);
        input.clear();
        return length;
    }
    const int8_t* get_buffer() const { return handshaking ? handshake.data() : buffer.data(); }
    bool dispatch(const int8_t*, size_t) { return true; }
};
}
''')
    for backend in ('SocketAsio', 'SocketWinsock'):
        (src / 'Net' / (backend + '.h')).write_text(
            '#pragma once\n#include "SocketPS5.h"\n'
            f'namespace ms {{ using {backend} = SocketPS5; }}\n')
    (work / 'check.cpp').write_text(r'''
#include "Net/Session.h"
#include <cassert>
#include <iostream>
using Bytes = std::vector<int8_t>;
const Bytes ping{17, 0};
const Bytes status{3, 0, 0, 0}; // SERVERSTATUS: opcode followed by a zero status.
int failures = 0;

Bytes frame(ms::Cryptography& peer, Bytes body) {
    Bytes wire(ms::HEADER_LENGTH);
    peer.create_header(wire.data(), body.size());
    peer.encrypt(body.data(), body.size());
    wire.insert(wire.end(), body.begin(), body.end());
    return wire;
}
void feed(ms::Session& session, const Bytes& bytes) {
    ms::input = bytes;
    session.read();
}
void check(const char* name, const std::vector<Bytes>& expected) {
    if (ms::delivered != expected) {
        std::cerr << "FAIL: " << name << " (expected " << expected.size()
                  << " exact frames, received " << ms::delivered.size() << ")\n";
        ++failures;
    } else std::cout << "PASS: " << name << '\n';
}
void fragments(const char* name, std::initializer_list<size_t> sizes,
               const std::vector<Bytes>& messages) {
    ms::delivered.clear();
    ms::Session session;
    assert(!session.init());
    ms::Cryptography peer(ms::handshake.data());
    Bytes wire;
    for (const auto& body : messages) {
        const auto packet = frame(peer, body);
        wire.insert(wire.end(), packet.begin(), packet.end());
    }
    size_t position = 0;
    for (size_t size : sizes) {
        assert(position + size <= wire.size());
        feed(session, Bytes(wire.begin() + position, wire.begin() + position + size));
        position += size;
    }
    assert(position == wire.size());
    check(name, messages);
}
void reconnect(const char* name, const Bytes& body, size_t prefix) {
    ms::delivered.clear();
    ms::Session session;
    assert(!session.init());
    ms::Cryptography peer(ms::handshake.data());
    const auto interrupted = frame(peer, body);
    feed(session, Bytes(interrupted.begin(), interrupted.begin() + prefix));
    assert(ms::delivered.empty());
    session.reconnect("in-memory", "1");
    peer = ms::Cryptography(ms::handshake.data());
    feed(session, frame(peer, ping));
    check(name, {ping});
}
void reconnect_from_handler() {
    ms::delivered.clear();
    ms::Session session;
    assert(!session.init());
    ms::Cryptography peer(ms::handshake.data());
    auto wire = frame(peer, ping);
    const auto tail = frame(peer, ping);
    wire.insert(wire.end(), tail.begin(), tail.end());
    bool restarted = false;
    ms::after_packet = [&] {
        if (!restarted) {
            restarted = true;
            session.reconnect("in-memory", "1");
        }
    };
    feed(session, wire);
    ms::after_packet = {};
    check("handler reconnect discards old connection tail", {ping});
    peer = ms::Cryptography(ms::handshake.data());
    feed(session, frame(peer, status));
    check("handler reconnect accepts new connection frame", {ping, status});
}
int main() {
    fragments("complete frame", {6}, {ping});
    fragments("one byte per read", {1, 1, 1, 1, 1, 1}, {ping});
    fragments("header then body", {4, 2}, {ping});
    fragments("complete frame and partial next header", {8, 4}, {ping, ping});
    fragments("complete frame and partial next body", {11, 1}, {ping, ping});
    fragments("fragmented body", {6, 2}, {status});
    reconnect("reconnect after partial header", ping, 2);
    reconnect("reconnect after partial body", status, 6);
    reconnect_from_handler();
    return failures ? 1 : 0;
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-O1', '-DUSE_ASIO', '-DPLATFORM_PS5',
                    '-I' + str(src), str(work / 'check.cpp'), str(src / 'Net/Session.cpp'),
                    str(src / 'Net/Cryptography.cpp'), '-o', str(binary)], check=True)
    subprocess.run([binary], cwd=work, check=True, timeout=10)
