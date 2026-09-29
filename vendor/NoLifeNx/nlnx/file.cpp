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

#include "file_impl.hpp"
#include "node_impl.hpp"
#include <algorithm>
#ifdef _WIN32
#  ifdef _MSC_VER
#    include <codecvt>
#  else
#    include <clocale>
#    include <cstdlib>
#  endif
#  ifdef __MINGW32__
#    include <windows.h>
#  else
#    include <Windows.h>
#  endif // __MINGW32__
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/fcntl.h>
#  include <sys/mman.h>
#  include <unistd.h>
#endif
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#ifdef OPENSTORY_LAN_LOG
extern "C" void openstory_diagnostics_phase(const char*, uintptr_t);
static void nx_phase(const char* label, uintptr_t value = 0) { openstory_diagnostics_phase(label, value); }
#else
static void nx_phase(const char*, uintptr_t = 0) {}
#endif

namespace nl {
#ifdef NLNX_STREAMING
    bool _file_data::read(uint64_t offset, void* output, size_t length) const noexcept {
        if (offset > size || length > size - offset || (length && !output)) return false;
        if (!length) return true;
        if (!base) nx_phase("nx-read-lock");
        // Seek/read share one descriptor cursor; protect the entire request from audio/graphics races.
        std::lock_guard<std::mutex> lock(read_mutex);
        if (!base) nx_phase("nx-read-seek");
        const auto target = static_cast<off_t>(offset + (wrapped ? 16 : 0));
        off_t position;
        do { position = ::lseek(file_handle, target, SEEK_SET); }
        while (position < 0 && errno == EINTR);
        if (position < 0 || position != target) {
            nx_phase("nx-file-seek-failed", position < 0 ? errno : 0);
            return false;
        }
        if (!base) nx_phase("nx-read-data");
        size_t received = 0;
        while (received < length) {
            const size_t chunk = std::min(length - received, size_t(1024 * 1024));
            const ssize_t count = ::read(file_handle, static_cast<char*>(output) + received, chunk);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) {
                nx_phase("nx-file-read-failed", count < 0 ? errno : 0);
                return false;
            }
            received += static_cast<size_t>(count);
        }
        if (wrapped) {
            auto* bytes = static_cast<unsigned char*>(output);
            for (size_t i = 0; i < length; ++i) bytes[i] ^= 0xA5;
        }
        return true;
    }
#endif
    file::file(std::string name) {
        open(name);
    }
    file::~file() {
        close();
    }
    void file::open(std::string name) try {
        nx_phase("nx-file-open");
        nx_phase(name.c_str());
        close();
        m_data = new data();
#ifdef _WIN32
#  ifdef _MSC_VER
        std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
        auto str = convert.from_bytes(name);
#  else
        std::setlocale(LC_ALL, "en_US.utf8");
        auto str = std::wstring(name.size(), 0);
        auto len = std::mbstowcs(const_cast<wchar_t *>(str.c_str()), name.c_str(), str.size());
        str.resize(len);
#  endif
#  if WINAPI_FAMILY == WINAPI_FAMILY_APP
        m_data->file_handle = ::CreateFile2(str.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
#  else
        m_data->file_handle = ::CreateFileW(str.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
#  endif
        if (m_data->file_handle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Failed to open file " + name);
#  if WINAPI_FAMILY == WINAPI_FAMILY_APP
        m_data->map = ::CreateFileMappingFromApp(m_data->file_handle, 0, PAGE_READONLY, 0, nullptr);
#  else
        m_data->map = ::CreateFileMappingW(m_data->file_handle, 0, PAGE_READONLY, 0, 0, nullptr);
#  endif
        if (!m_data->map)
            throw std::runtime_error("Failed to create file mapping of file " + name);
#  if WINAPI_FAMILY == WINAPI_FAMILY_APP
        m_data->base = ::MapViewOfFileFromApp(m_data->map, FILE_MAP_READ, 0, 0);
#  else
        m_data->base = ::MapViewOfFile(m_data->map, FILE_MAP_READ, 0, 0, 0);
#  endif
        if (!m_data->base)
            throw std::runtime_error("Failed to map view of file " + name);
#else
        m_data->file_handle = ::open(name.c_str(), O_RDONLY);
        if (m_data->file_handle == -1)
            throw std::runtime_error("Failed to open file " + name);
        nx_phase("nx-file-stat", m_data->file_handle);
        struct stat finfo;
        if (::fstat(m_data->file_handle, &finfo) == -1)
            throw std::runtime_error("Failed to obtain file information of file " + name);
        nx_phase("nx-file-stat-ready", finfo.st_size);
        if (finfo.st_size < static_cast<off_t>(sizeof(header)))
            throw std::runtime_error("Truncated NX header in file " + name);
        m_data->size = finfo.st_size;
#ifdef NLNX_STREAMING
        char raw_header[sizeof(header)];
        if (!m_data->read(0, raw_header, sizeof(raw_header)))
            throw std::runtime_error("Failed to read NX header from file " + name);
        // The package wrapper changes every data block, avoiding damaged blocks reused from older references.
        if (std::memcmp(raw_header, "OSNX\1\0\0\0", 8) == 0) {
            uint64_t length;
            std::memcpy(&length, raw_header + 8, sizeof(length));
            if (length < sizeof(header) || length != m_data->size - 16)
                throw std::runtime_error("Invalid wrapped NX length: " + name);
            m_data->size = length;
            m_data->wrapped = true;
            if (!m_data->read(0, raw_header, sizeof(raw_header)))
                throw std::runtime_error("Failed to read wrapped NX header: " + name);
            nx_phase("nx-wrapper-decoded", length);
        }
        nx_phase("nx-header-read-ready");
        const auto& disk = *reinterpret_cast<header const*>(raw_header);
        if (disk.magic != 0x34474B50)
            throw std::runtime_error(name + " is not a PKG4 NX file");
        size_t metadata_size = sizeof(header);
        const auto table = [&](uint64_t offset, uint32_t count, size_t stride) {
            if (offset > m_data->size || count > (m_data->size - offset) / stride)
                throw std::runtime_error("Invalid NX table in file " + name);
            metadata_size = std::max(metadata_size, size_t(offset) + size_t(count) * stride);
        };
        table(disk.node_offset, disk.node_count, sizeof(node::data));
        table(disk.string_offset, disk.string_count, sizeof(uint64_t));
        table(disk.bitmap_offset, disk.bitmap_count, sizeof(uint64_t));
        table(disk.audio_offset, disk.audio_count, sizeof(uint64_t));
        // ponytail: retain indices up to 256 MiB per file; page nodes if a future dataset exceeds this.
        if (metadata_size > 256 * 1024 * 1024)
            throw std::runtime_error("NX metadata exceeds the memory limit: " + name);
        m_data->metadata_size = metadata_size;
        nx_phase("nx-index-allocate", metadata_size);
        m_data->base = std::malloc(metadata_size);
        if (!m_data->base) {
            nx_phase("nx-index-allocation-failed", metadata_size);
            throw std::runtime_error("Failed to allocate NX metadata for file " + name);
        }
        if (!m_data->read(0, const_cast<void*>(m_data->base), metadata_size))
            throw std::runtime_error("Failed to read NX metadata from file " + name);
        nx_phase("nx-index-read-ready", metadata_size);
#else
        nx_phase("nx-file-map", m_data->size);
        m_data->base = ::mmap(nullptr, m_data->size, PROT_READ, MAP_SHARED, m_data->file_handle, 0);
        if (reinterpret_cast<intptr_t>(m_data->base) == -1)
            throw std::runtime_error("Failed to create memory mapping of file " + name);
#endif
#endif
        nx_phase("nx-file-header");
        m_data->header = reinterpret_cast<header const *>(m_data->base);
        if (m_data->header->magic != 0x34474B50)
            throw std::runtime_error(name + " is not a PKG4 NX file");
        m_data->node_table = reinterpret_cast<node::data const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->node_offset);
        m_data->string_table = reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->string_offset);
        m_data->bitmap_table = reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->bitmap_offset);
        m_data->audio_table = reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->audio_offset);
#ifdef NLNX_STREAMING
        // Strings and nodes must stay within the resident metadata, not the media blobs.
        for (uint32_t i = 0; i < disk.string_count; ++i) {
            const auto offset = m_data->string_table[i];
            uint16_t length;
            if (offset > metadata_size - sizeof(length))
                throw std::runtime_error("NX string outside metadata: " + name);
            std::memcpy(&length, static_cast<char const*>(m_data->base) + offset, sizeof(length));
            if (length > metadata_size - offset - sizeof(length))
                throw std::runtime_error("Truncated NX string: " + name);
        }
        uint32_t missing_bitmaps = 0;
        for (uint32_t i = 0; i < disk.bitmap_count; ++i)
            if (m_data->bitmap_table[i] < metadata_size || m_data->bitmap_table[i] > m_data->size - 4)
                ++missing_bitmaps;
        if (missing_bitmaps) nx_phase("nx-missing-bitmaps", missing_bitmaps);
        for (uint32_t i = 0; i < disk.audio_count; ++i)
            if (m_data->audio_table[i] < metadata_size || m_data->audio_table[i] > m_data->size)
                throw std::runtime_error("Invalid NX audio offset: " + name);
        for (uint32_t i = 0; i < disk.node_count; ++i) {
            const auto& n = m_data->node_table[i];
            if (n.name >= disk.string_count || n.children > disk.node_count || n.num > disk.node_count - n.children ||
                (n.type == node::type::string && n.string >= disk.string_count) ||
                (n.type == node::type::bitmap && n.bitmap.index >= disk.bitmap_count) ||
                (n.type == node::type::audio && n.audio.index >= disk.audio_count))
                throw std::runtime_error("Invalid NX node reference: " + name);
        }
#endif
        nx_phase("nx-file-ready");
    } catch (...) {
        close();
        throw;
    }
    void file::close() {
        if (!m_data) return;
#ifdef _WIN32
        ::UnmapViewOfFile(m_data->base);
        ::CloseHandle(m_data->map);
        ::CloseHandle(m_data->file_handle);
#else
#ifdef NLNX_STREAMING
        std::free(const_cast<void *>(m_data->base));
#else
        if (m_data->base && m_data->base != MAP_FAILED)
            ::munmap(const_cast<void *>(m_data->base), m_data->size);
#endif
        if (m_data->file_handle >= 0) ::close(m_data->file_handle);
#endif
        delete m_data;
        m_data = nullptr;
    }
    node file::root() const {
        return {m_data->node_table, m_data};
    }
    file::operator node() const {
        return root();
    }
    uint32_t file::string_count() const {
        return m_data->header->string_count;
    }
    uint32_t file::bitmap_count() const {
        return m_data->header->bitmap_count;
    }
    uint32_t file::audio_count() const {
        return m_data->header->audio_count;
    }
    uint32_t file::node_count() const {
        return m_data->header->node_count;
    }
    std::string file::get_string(uint32_t i) const {
        auto const s = reinterpret_cast<char const *>(m_data->base) + m_data->string_table[i];
        return {s + 2, *reinterpret_cast<uint16_t const *>(s)};
    }
}
