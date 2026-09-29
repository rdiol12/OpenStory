#!/usr/bin/env python3
"""Emulate the reported BGRA driver behavior; this is not a PS5 hardware test."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
production = (root / 'src/Graphics/GraphicsGL.cpp').read_text()
match = re.search(r'void\s+upload_bgra\s*\([^)]*\)\s*\{', production)
legacy = match is None
if legacy:
    # Reproduce the current behavior using the actual atlas GL statement.
    call = re.search(r'glTexSubImage2D\(GL_TEXTURE_2D, 0, x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels\);', production)
    assert call, 'Expected production atlas upload statement'
    helper = 'void upload_bgra(GLint x, GLint y, GLsizei width, GLsizei height, const void* pixels) {' + call[0] + '}'
else:
    end, depth = match.end(), 1
    while depth:
        depth += (production[end] == '{') - (production[end] == '}')
        end += 1
    helper = production[match.start():end]
    for arguments in ('x, y, width, height, pixels', 'gx, gy, strike_w, strike_h, src_pixels'):
        assert re.search(r'upload_bgra\(\s*' + r'\s*,\s*'.join(arguments.split(', ')) + r'\s*\)', production), arguments

code = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
using GLint = int;
using GLsizei = int;
using GLenum = unsigned;
constexpr GLenum GL_TEXTURE_2D = 1, GL_BGRA = 2, GL_RGBA = 3, GL_UNSIGNED_BYTE = 4;
int calls = 0;
GLenum uploaded_format = 0;
const void* uploaded_pointer = nullptr;
std::vector<uint8_t> sampled;
void glTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei width,
                     GLsizei height, GLenum format, GLenum type, const void* pixels) {
    ++calls;
    assert(target == GL_TEXTURE_2D && level == 0 && type == GL_UNSIGNED_BYTE);
    assert(x == 11 && y == 13 && width == 2 && height == 2 && pixels);
    uploaded_format = format;
    uploaded_pointer = pixels;
    const auto* bytes = static_cast<const uint8_t*>(pixels);
    sampled.assign(bytes, bytes + width * height * 4);
#ifndef PLATFORM_PS5
    // Desktop correctly honors the requested BGRA component order.
    if (format == GL_BGRA)
        for (size_t i = 0; i < sampled.size(); i += 4) std::swap(sampled[i], sampled[i+2]);
#endif
    // PS5 EMULATION: reproduce the reported RGBA interpretation of BGRA bytes.
    // A passing test proves our upload bytes, not the behavior of console hardware.
}
''' + helper + r'''
int main() {
    std::array<uint8_t, 16> bgra{0,0,255,255, 0,255,0,128, 255,0,0,64, 0,255,255,0};
    const auto original = bgra;
    const std::vector<uint8_t> rgba{255,0,0,255, 0,255,0,128, 0,0,255,64, 255,255,0,0};
    upload_bgra(11, 13, 2, 2, bgra.data());
    assert(bgra == original && "upload must not mutate shared source pixels");
    if (sampled != rgba) {
        std::cerr << "FAIL: emulated texture sampling swaps red/blue; expected red RGBA=(255,0,0,255), got=("
                  << int(sampled[0]) << ',' << int(sampled[1]) << ',' << int(sampled[2]) << ',' << int(sampled[3]) << ")\n";
        return 1;
    }
    assert(calls == 1);
#ifdef PLATFORM_PS5
    assert(uploaded_format == GL_RGBA);
#else
    assert(uploaded_format == GL_BGRA && uploaded_pointer == bgra.data());
#endif
#ifndef LEGACY_UPLOAD
    upload_bgra(11, 13, 0, 2, bgra.data());
    upload_bgra(11, 13, 2, 0, bgra.data());
    upload_bgra(11, 13, -1, 2, bgra.data());
    upload_bgra(11, 13, 2, -1, bgra.data());
    upload_bgra(11, 13, 2, 2, nullptr);
    assert(calls == 1 && "empty/null uploads must not reach GL or read source bytes");
#endif
}
'''
print('Driver behavior is emulated; this is not hardware verification.' +
      (' Testing the existing production upload statement (helper absent).' if legacy else ''), flush=True)
with tempfile.TemporaryDirectory(prefix='openstory-texture-colors-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text(code)
    for platform in ('desktop', 'ps5-emulated'):
        binary = work / platform
        defines = (['-DPLATFORM_PS5'] if platform == 'ps5-emulated' else []) + (['-DLEGACY_UPLOAD'] if legacy else [])
        subprocess.run(['c++', '-std=c++17', *defines, str(source), '-o', str(binary)], check=True)
        subprocess.run([binary], check=True)
print('PASS: BGRA artwork/emoji upload preserves RGBA colors/alpha, source bytes, and desktop behavior')
