#!/usr/bin/env python3
"""Check NX reads, interrupted/short I/O, cleanup, and the full PS5 asset set."""
from pathlib import Path
import shutil
import resource
import struct
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
assets = Path(sys.argv[1]).resolve()
reference_assets = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else assets
sys.path.insert(0, str(root / 'platform/ps5'))
from pack_assets import pack
nx = root / 'vendor/NoLifeNx/nlnx'
with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    shutil.copyfile(reference_assets / 'Base.nx', work / 'Base.nx')
    pack(work / 'Base.nx', work / 'wrapped.nx')
    invalid = bytearray((work / 'wrapped.nx').read_bytes())
    struct.pack_into('<Q', invalid, 8, 2**64 - 1)
    (work / 'bad-wrapper.nx').write_bytes(invalid)
    (work / 'bad.nx').write_bytes(b'BAD!' + (work / 'Base.nx').read_bytes()[4:])
    (work / 'short.nx').write_bytes(b'PKG4')
    invalid = bytearray((work / 'Base.nx').read_bytes())
    struct.pack_into('<Q', invalid, 8, 2**64 - 1)
    (work / 'bad-table.nx').write_bytes(invalid)
    invalid = bytearray((work / 'Base.nx').read_bytes())
    string_table = struct.unpack_from('<Q', invalid, 20)[0]
    struct.pack_into('<Q', invalid, string_table, len(invalid) + 1)
    (work / 'bad-string.nx').write_bytes(invalid)
    invalid = bytearray(108)
    struct.pack_into('<IIQIQIQIQ', invalid, 0, 0x34474B50, 1, 64, 1, 84, 1, 96, 0, 96)
    struct.pack_into('<IIHHIHH', invalid, 64, 0, 0, 0, 5, 0, 1, 1)
    struct.pack_into('<Q', invalid, 84, 92)
    struct.pack_into('<Q', invalid, 96, 104)
    struct.pack_into('<I', invalid, 104, 2**32 - 1)
    (work / 'bad-bitmap.nx').write_bytes(invalid)
    # One usable 1x1 image alongside omitted blobs; no malformed memory accesses.
    partial = bytearray(249)
    struct.pack_into('<IIQIQIQIQ', partial, 0, 0x34474B50, 6, 64, 1, 184, 5, 200, 0, 240)
    struct.pack_into('<IIHHQ', partial, 64, 0, 1, 5, 0, 0)
    for index in range(5):
        struct.pack_into('<IIHHIHH', partial, 84 + 20 * index, 0, 0, 0, 5, index, 1, 1)
    struct.pack_into('<Q', partial, 184, 192)  # Empty node name within metadata.
    struct.pack_into('<5Q', partial, 200, 240, 0, 64, len(partial), len(partial) + 16)
    partial[240:] = struct.pack('<I', 5) + b'\x40\x11\x22\x33\xff'  # LZ4 literal block.
    (work / 'partial-bitmaps.nx').write_bytes(partial)
    pack(work / 'partial-bitmaps.nx', work / 'partial-bitmaps-wrapped.nx')
    source = work / 'check.cpp'
    source.write_text(r'''
#include <nlnx/file.hpp>
#include <nlnx/node.hpp>
#include <nlnx/nx.hpp>
#include <nlnx/bitmap.hpp>
#include <nlnx/audio.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>
static int mode;
static std::atomic<int> reads, seeks;
extern "C" void* __real_mmap(void*, size_t, int, int, int, off_t);
extern "C" ssize_t __real_pread(int, void*, size_t, off_t);
extern "C" ssize_t __real_read(int, void*, size_t);
extern "C" off_t __real_lseek(int, off_t, int);
extern "C" void* __wrap_mmap(void* p, size_t n, int prot, int flags, int fd, off_t offset) {
#ifdef PLATFORM_PS5
    assert(fd < 0 && "PS5 NX files must load without file-backed mmap");
#endif
    return __real_mmap(p, n, prot, flags, fd, offset);
}
extern "C" ssize_t __wrap_pread(int fd, void* p, size_t n, off_t offset) {
#ifdef PLATFORM_PS5
    assert(false && "Native NX streaming must not call the suspect pread import");
#endif
    return __real_pread(fd, p, n, offset);
}
extern "C" off_t __wrap_lseek(int fd, off_t offset, int whence) {
    ++seeks;
    if (mode == 4) { errno = EIO; return -1; }
    if (mode == 5 && seeks == 1) { errno = EINTR; return -1; }
    off_t result = __real_lseek(fd, offset, whence);
    std::this_thread::yield(); // Make an unprotected seek/read pair race in the audio check.
    return result;
}
extern "C" ssize_t __wrap_read(int fd, void* p, size_t n) {
    ++reads;
    if (mode == 1) {
        if (reads == 1) { errno = EINTR; return -1; }
        n = std::min(n, size_t(97));
    }
    if (mode == 2 && reads > 1) return 0;
    if (mode == 3 && reads > 1) { errno = EIO; return -1; }
    if (mode >= 2) n = std::min(n, size_t(97));
    return __real_read(fd, p, n);
}
static int descriptors() {
    int n = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) ++n;
    return n;
}
int main(int argc, char** argv) {
    const int before = descriptors();
    mode = 1;
    {
        nl::file file("Base.nx");
        assert(file.node_count() && file.string_count());
        uint64_t hash = 14695981039346656037ULL;
        for (uint32_t i = 0; i < file.string_count(); ++i)
            for (unsigned char c : file.get_string(i)) hash = (hash ^ c) * 1099511628211ULL;
        std::printf("%u %u %llu\n", file.node_count(), file.string_count(), (unsigned long long)hash);
#ifdef PLATFORM_PS5
        assert(reads > 2);
#endif
        file.close();
        file.close();
        file.open("Base.nx");
    }
    assert(descriptors() == before);
    for (const char* path : {"bad.nx", "short.nx", "missing.nx"}) {
        for (int i = 0; i < 20; ++i) {
            bool rejected = false;
            try { nl::file file(path); } catch (const std::runtime_error&) { rejected = true; }
            assert(rejected);
        }
        assert(descriptors() == before);
    }
#ifdef PLATFORM_PS5
    {
        nl::file wrapped("wrapped.nx"), plain("Base.nx");
        assert(wrapped.node_count() == plain.node_count() && wrapped.string_count() == plain.string_count());
        for (uint32_t i = 0; i < plain.string_count(); ++i) assert(wrapped.get_string(i) == plain.get_string(i));
    }
    assert(descriptors() == before);
    for (const char* path : {"bad-table.nx", "bad-string.nx", "bad-wrapper.nx"}) {
        bool rejected = false;
        try { nl::file file(path); } catch (const std::runtime_error&) { rejected = true; }
        assert(rejected && descriptors() == before);
    }
    {
        nl::file file("bad-bitmap.nx");
        assert(!file.root().get_bitmap().data());
    }
    for (const char* path : {"partial-bitmaps.nx", "partial-bitmaps-wrapped.nx"}) {
        nl::file file(path);
        auto image = file.root().begin();
        auto usable = (*image++).get_bitmap();
        const unsigned char expected[] = {0x11, 0x22, 0x33, 0xff};
        assert(usable.length() == sizeof(expected) && usable.data());
        assert(std::memcmp(usable.data(), expected, sizeof(expected)) == 0);
        for (; image != file.root().end(); ++image) {
            reads = 0; seeks = 0;
            auto absent = (*image).get_bitmap();
            assert(!absent && absent.id() == 0 && "missing images must not become drawable textures");
            assert(!absent.data());
            assert(reads == 0 && seeks == 0 && "missing images must not read metadata or outside the file");
        }
        assert(usable.data() && std::memcmp(usable.data(), expected, sizeof(expected)) == 0);
    }
    assert(descriptors() == before);
    for (mode = 2; mode <= 3; ++mode) {
        reads = 0;
        bool rejected = false;
        try { nl::file file("Base.nx"); } catch (const std::runtime_error&) { rejected = true; }
        assert(rejected && reads > 1);
        assert(descriptors() == before);
    }
    mode = 4;
    bool seek_rejected = false;
    try { nl::file file("Base.nx"); } catch (const std::runtime_error&) { seek_rejected = true; }
    assert(seek_rejected && descriptors() == before);
    mode = 5; seeks = 0;
    { nl::file file("Base.nx"); assert(file.node_count() && seeks > 1); }
    assert(descriptors() == before);
#endif
    mode = 0;
    if (argc > 1) {
        nl::nx::load_all(argv[1]);
        assert(nl::nx::base && nl::nx::character && nl::nx::map && nl::nx::mob && nl::nx::ui);
#ifdef PLATFORM_PS5
        // Original UI_83 has an omitted bitmap here. Never dereference it on the mmap path.
        if (!nl::nx::ui["UIWindow2.img"]) {
            auto omitted = nl::nx::ui["UIWindow.img"]["SkillUp"]["btOK"]["normal"]["0"].get_bitmap();
            assert(!omitted && omitted.id() == 0);
            reads = 0; seeks = 0;
            assert(!omitted.data() && reads == 0 && seeks == 0);
        }
#endif
        uint64_t hash = 14695981039346656037ULL;
        auto bytes = [&](const void* data, size_t size) {
            for (size_t i = 0; i < size; ++i) hash = (hash ^ static_cast<const unsigned char*>(data)[i]) * 1099511628211ULL;
        };
        int image_count = 0, audio_count = 0;
        for (auto root : {nl::nx::base, nl::nx::character, nl::nx::effect, nl::nx::etc, nl::nx::item,
             nl::nx::map, nl::nx::mob, nl::nx::morph, nl::nx::npc, nl::nx::quest, nl::nx::reactor,
             nl::nx::skill, nl::nx::sound, nl::nx::string, nl::nx::tamingmob,
             nl::nx::ui["Login.img"], nl::nx::ui["Basic.img"]}) {
            std::vector<nl::node> pending{root};
            int visited = 0, images = 0, sounds = 0;
            while (!pending.empty() && visited++ < 4096) {
                auto node = pending.back(); pending.pop_back();
                auto name = node.name(); bytes(name.data(), name.size());
                if (node.data_type() == nl::node::type::bitmap && images++ < 3) {
                    auto image = node.get_bitmap();
                    assert(image && image.id() == node.get_bitmap().id());
                    auto pixels = image.data(); assert(pixels); bytes(pixels, image.length());
#ifdef PLATFORM_PS5
                    reads = 0; mode = 3; assert(!image.data()); mode = 0;
                    assert(image.data());
#endif
                    ++image_count;
                }
                if (node.data_type() == nl::node::type::audio && sounds++ < 2) {
                    auto audio = node.get_audio();
                    std::vector<char> clip(audio.length());
                    assert(audio && audio.id() == node.get_audio().id());
                    assert(audio.read(clip.data(), 0, clip.size()));
                    assert(audio.data() && std::memcmp(audio.data(), clip.data(), clip.size()) == 0);
                    assert(!audio.read(clip.data(), audio.length(), 1));
#ifdef PLATFORM_PS5
                    assert(clip.size() > 128);
                    auto compare = [&](size_t shift) {
                        char part[128];
                        for (size_t i = 0; i < 1024; ++i) {
                            const size_t at = (i * 7919 + shift) % (clip.size() - sizeof(part));
                            assert(audio.read(part, at, sizeof(part)));
                            assert(std::memcmp(part, clip.data() + at, sizeof(part)) == 0);
                        }
                    };
                    std::thread first(compare, 17), second(compare, 1901);
                    first.join(); second.join();
#endif
                    bytes(clip.data(), clip.size());
                    ++audio_count;
                }
                for (auto child : node) pending.push_back(child);
            }
        }
        assert(image_count > 10 && audio_count > 0);
        std::printf("media %d %d %llu\n", image_count, audio_count, (unsigned long long)hash);
    }
}
''')
    outputs = []
    lz4 = nx / 'includes/lz4_v1_8_2_win64'
    subprocess.run(['cc', '-O1', '-I' + str(lz4 / 'include'), '-c', str(lz4 / 'lz4.c'),
                    '-o', str(work / 'lz4.o')], check=True)
    for defines in (['-DPLATFORM_PS5'], []):
        binary = work / ('native' if defines else 'desktop')
        subprocess.run(['c++', '-std=c++17', '-O1', *defines,
                        '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                        '-I' + str(nx.parent), '-I' + str(lz4 / 'include'), str(source),
                        *[str(nx / name) for name in ('nx.cpp', 'node.cpp', 'file.cpp', 'bitmap.cpp', 'audio.cpp')],
                        str(work / 'lz4.o'),
                        '-pthread', '-Wl,--wrap=mmap,--wrap=pread,--wrap=read,--wrap=lseek', '-o', str(binary)], check=True)
        outputs.append(subprocess.check_output([binary], cwd=work, text=True, timeout=20))
    assert outputs[0] == outputs[1], 'Buffered and mapped NX contents differ'
    def memory_limit():
        resource.setrlimit(resource.RLIMIT_AS, (512 * 1024**2, 512 * 1024**2))
    streamed = subprocess.check_output([work / 'native', assets], cwd=work, text=True, timeout=180, preexec_fn=memory_limit)
    mapped = subprocess.check_output([work / 'desktop', reference_assets], cwd=work, text=True, timeout=180)
    assert streamed == mapped, 'Streamed and mapped bitmap/audio contents differ'
    print(streamed.strip())
print('PASS: streamed NX/media match mapped data; seek/read errors and concurrent audio handled; full asset set within 512 MiB')
