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

#include "audio.hpp"
#include <cstring>
#ifdef NLNX_STREAMING
#include "file_impl.hpp"
#endif

namespace nl {
#ifdef NLNX_STREAMING
    audio::audio(void const * d, uint32_t l, _file_data const* f) :
        m_file(f), m_data(d), m_length(l) {}
#else
    audio::audio(void const * d, uint32_t l) :
        m_data(d), m_length(l) {}
#endif
    bool audio::operator<(audio const & o) const {
        return m_data < o.m_data;
    }
    bool audio::operator==(audio const & o) const {
        return m_data == o.m_data;
    }
    audio::operator bool() const {
        return m_data ? true : false;
    }
    void const * audio::data() const {
#ifdef NLNX_STREAMING
        if (!m_data || !m_length) return nullptr;
        const auto key = std::make_pair(*static_cast<uint64_t const*>(m_data), m_length);
        auto found = m_file->audio_cache.find(key);
        if (found != m_file->audio_cache.end()) return found->second.data();
        if (key.first > m_file->size || m_length > m_file->size - key.first) return nullptr;
        // Preserve the legacy data() lifetime; SDL playback uses read() and retains no clip cache.
        std::vector<char> bytes(m_length);
        if (!read(bytes.data(), 0, bytes.size())) return nullptr;
        return m_file->audio_cache.emplace(key, std::move(bytes)).first->second.data();
#else
        return m_data;
#endif
    }
    bool audio::read(void* output, size_t offset, size_t count) const {
        if (!m_data || offset > m_length || count > m_length - offset || (count && !output)) return false;
#ifdef NLNX_STREAMING
        const uint64_t start = *static_cast<uint64_t const*>(m_data);
        if (start > m_file->size || offset > m_file->size - start) return false;
        return m_file->read(start + offset, output, count);
#else
        if (count) std::memcpy(output, static_cast<char const*>(m_data) + offset, count);
        return true;
#endif
    }
    uint32_t audio::length() const {
        return m_length;
    }
    size_t audio::id() const {
        return reinterpret_cast<size_t>(m_data);
    }
}
