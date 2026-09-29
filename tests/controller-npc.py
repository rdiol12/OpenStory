#!/usr/bin/env python3
"""Compile actual controller NPC selection with an isolated interaction recorder."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
production = (root / 'src/Gameplay/MapleMap/MapNpcs.cpp').read_text()
start = production.find('void MapNpcs::interact(')
assert start >= 0, 'MapNpcs::interact is not implemented'
method = production[start:production.index('\n\t}', start) + len('\n\t}')]

with tempfile.TemporaryDirectory(prefix='openstory-controller-npc-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text(r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>
namespace ms {
template<class T> struct Point {
    T px, py;
    T x() const { return px; }
    T y() const { return py; }
};
struct Npc {
    int32_t oid;
    Point<int16_t> position;
    bool active;
    int32_t get_oid() const { return oid; }
    Point<int16_t> get_position() const { return position; }
    bool is_active() const { return active; }
};
std::vector<int32_t> interactions;
void talk_to_npc(Npc& npc) { interactions.push_back(npc.get_oid()); }
struct MapNpcs {
    std::vector<std::pair<int32_t, std::unique_ptr<Npc>>> npcs;
    void interact(Point<int16_t> player);
};
''' + method + r'''
}
int main() {
    auto check = [](std::initializer_list<ms::Npc> candidates, ms::Point<int16_t> player, int32_t expected) {
        ms::MapNpcs map;
        for (const auto& npc : candidates)
            map.npcs.emplace_back(npc.oid, std::make_unique<ms::Npc>(npc));
        ms::interactions.clear();
        map.interact(player);
        assert(ms::interactions == (expected ? std::vector<int32_t>{expected} : std::vector<int32_t>{}));
    };
    check({}, {1000, 1000}, 0);
    check({{1, {1000, 1000}, false}, {2, {1151, 1000}, true},
           {3, {849, 1000}, true}, {4, {1000, 1081}, true},
           {5, {1000, 919}, true}}, {1000, 1000}, 0);
    check({{1, {1000, 1000}, false}, {2, {1000, 1070}, true},
           {3, {1050, 1000}, true}}, {1000, 1000}, 3);
    check({{9, {1150, 1080}, true}}, {1000, 1000}, 9);
    check({{9, {850, 920}, true}}, {1000, 1000}, 9);
    check({{20, {970, 1040}, true}, {10, {1030, 960}, true}}, {1000, 1000}, 10);
    check({{10, {1030, 960}, true}, {20, {970, 1040}, true}}, {1000, 1000}, 10);
    check({{1, {-32760, 0}, true}}, {32760, 0}, 0); // Differences must not wrap at int16 limits.
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', str(source), '-o', str(binary)], check=True)
    subprocess.run([binary], check=True)
print('PASS: nearest active NPC, inclusive reach bounds, deterministic ties, and no-target behavior')
