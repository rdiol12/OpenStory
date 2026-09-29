//////////////////////////////////////////////////////////////////////////////
// NoLifeNx - Part of the NoLifeStory project                               //
// Copyright © 2013 Peter Atashian                                          //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////

#include "bitmap.hpp"
#include <lz4.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <climits>
#ifdef NLNX_STREAMING
#include "file_impl.hpp"
#endif

namespace nl {
#ifdef NLNX_STREAMING
    bitmap::bitmap(void const * d, uint16_t w, uint16_t h, _file_data const* f) :
        m_file(f), m_data(d), m_width(w), m_height(h) {}
#else
    bitmap::bitmap(void const * d, uint16_t w, uint16_t h) :
        m_data(d), m_width(w), m_height(h) {}
#endif
    bool bitmap::operator<(bitmap const & o) const {
        return m_data < o.m_data;
    }
    bool bitmap::operator==(bitmap const & o) const {
        return m_data == o.m_data;
    }
    bitmap::operator bool() const {
        return m_data ? true : false;
    }
    std::vector<char> bitmap_buf = std::vector<char>(8 * 1024 * 1024);
    void const * bitmap::data() const {
        if (!m_data)
            return nullptr;
        if (uint64_t(m_width) * m_height * 4 > INT_MAX) return nullptr;
        auto const l = length();
        if (l + 0x20 > bitmap_buf.size())
            bitmap_buf.resize(l + 0x20);
        // The bitmap blob is [uint32 compressed_length][LZ4 block]. Use the SAFE
        // decompressor bounded by BOTH the compressed size and the output size,
        // so corrupt or dimension-mismatched data can't overrun the input and
        // crash (the _fast variant trusts the output size blindly and reads past
        // the compressed blob when it's wrong). On error we zero the buffer so a
        // bad bitmap draws blank instead of taking down the client.
        uint32_t clen;
#ifdef NLNX_STREAMING
        const uint64_t offset = *static_cast<uint64_t const*>(m_data);
        if (!m_file->read(offset, &clen, sizeof(clen)) || clen > INT_MAX ||
            clen > static_cast<uint32_t>(LZ4_compressBound(static_cast<int>(l))) ||
            offset > m_file->size - 4 || clen > m_file->size - offset - 4) return nullptr;
        // One reusable compressed buffer, matching the existing decompressed-buffer lifetime.
        static std::vector<char> compressed;
        compressed.resize(clen);
        if (!m_file->read(offset + 4, compressed.data(), clen)) return nullptr;
        const char* input = compressed.data();
#else
        std::memcpy(&clen, m_data, sizeof(clen));
        const char* input = 4 + reinterpret_cast<char const *>(m_data);
#endif
        int const got = ::LZ4_decompress_safe(
            input,
            bitmap_buf.data(),
            static_cast<int>(clen),
            static_cast<int>(l));
        if (got < 0)
            std::memset(bitmap_buf.data(), 0, l);
        return bitmap_buf.data();
    }
    uint16_t bitmap::width() const {
        return m_width;
    }
    uint16_t bitmap::height() const {
        return m_height;
    }
    uint32_t bitmap::length() const {
        return 4u * m_width * m_height;
    }
    size_t bitmap::id() const {
        return reinterpret_cast<size_t>(m_data);
    }
}
