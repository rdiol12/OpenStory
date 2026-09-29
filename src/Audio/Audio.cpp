//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "Audio.h"

#include "../Configuration.h"
#include "../Util/CrashLog.h"

#ifdef OPENSTORY_SDL
#include <SDL.h>
#include <SDL_mixer.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <new>
namespace
{
	Mix_Music* music = nullptr;
	std::string music_path;
	SDL_RWops* audio_stream(nl::audio audio);
	void play_music(const std::string& path, int loops);
}
#else
#include <bass.h>
#endif

#include <cmath>

#ifdef USE_NX
#include <nlnx/audio.hpp>
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Sound::Sound(Name name)
	{
		id = soundids[name];
	}

	Sound::Sound(int32_t itemid)
	{
		auto fitemid = format_id(itemid);

		if (itemids.find(fitemid) != itemids.end())
		{
			id = itemids.at(fitemid);
		}
		else
		{
			auto pid = (10000 * (itemid / 10000));
			auto fpid = format_id(pid);

			if (itemids.find(fpid) != itemids.end())
				id = itemids.at(fpid);
			else
				id = itemids.at("02000000");
		}
	}

	Sound::Sound(nl::node src)
	{
		id = add_sound(src);
	}

	Sound::Sound()
	{
		id = 0;
	}

	void Sound::play() const
	{
		if (id > 0)
			play(id);
	}

	void Sound::play(Point<int16_t> world_position) const
	{
		if (id == 0)
			return;

		// Distance from the listener (local player). Within FULL_RANGE the
		// sound is at full volume; it then fades linearly to silence at
		// MAX_RANGE and is skipped entirely beyond it.
		constexpr double FULL_RANGE = 400.0;
		constexpr double MAX_RANGE = 1200.0;
		// Horizontal offset that maps to hard-left / hard-right panning.
		constexpr double PAN_RANGE = 800.0;

		double dx = static_cast<double>(world_position.x() - listener_position.x());
		double dy = static_cast<double>(world_position.y() - listener_position.y());
		double distance = std::sqrt(dx * dx + dy * dy);

		if (distance >= MAX_RANGE)
			return; // too far to hear at all

		float volume = 1.0f;

		if (distance > FULL_RANGE)
			volume = static_cast<float>((MAX_RANGE - distance) / (MAX_RANGE - FULL_RANGE));

		float pan = static_cast<float>(dx / PAN_RANGE);

		if (pan < -1.0f)
			pan = -1.0f;
		else if (pan > 1.0f)
			pan = 1.0f;

		play(id, volume, pan);
	}

	Error Sound::init()
	{
#ifdef OPENSTORY_SDL
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0 || !(Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3)
			|| Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 1024) < 0)
		{
			Mix_CloseAudio();
			Mix_Quit();
			return Error::AUDIO;
		}
		Mix_AllocateChannels(32);
#else
		if (!BASS_Init(-1, 44100, 0, nullptr, 0))
			return Error::Code::AUDIO;
#endif

		nl::node uisrc = nl::nx::sound["UI.img"];

		add_sound(Sound::Name::BUTTONCLICK, uisrc["BtMouseClick"]);
		add_sound(Sound::Name::BUTTONOVER, uisrc["BtMouseOver"]);
		add_sound(Sound::Name::CHARSELECT, uisrc["CharSelect"]);
		add_sound(Sound::Name::DLGNOTICE, uisrc["DlgNotice"]);
		add_sound(Sound::Name::MENUDOWN, uisrc["MenuDown"]);
		add_sound(Sound::Name::MENUUP, uisrc["MenuUp"]);
		add_sound(Sound::Name::RACESELECT, uisrc["RaceSelect"]);
		add_sound(Sound::Name::SCROLLUP, uisrc["ScrollUp"]);
		add_sound(Sound::Name::SELECTMAP, uisrc["SelectMap"]);
		add_sound(Sound::Name::TAB, uisrc["Tab"]);
		add_sound(Sound::Name::WORLDSELECT, uisrc["WorldSelect"]);
		add_sound(Sound::Name::DRAGSTART, uisrc["DragStart"]);
		add_sound(Sound::Name::DRAGEND, uisrc["DragEnd"]);
		add_sound(Sound::Name::WORLDMAPOPEN, uisrc["WorldmapOpen"]);
		add_sound(Sound::Name::WORLDMAPCLOSE, uisrc["WorldmapClose"]);

		nl::node gamesrc = nl::nx::sound["Game.img"];

		add_sound(Sound::Name::GAMESTART, gamesrc["GameIn"]);
		add_sound(Sound::Name::JUMP, gamesrc["Jump"]);
		add_sound(Sound::Name::DROP, gamesrc["DropItem"]);
		add_sound(Sound::Name::PICKUP, gamesrc["PickUpItem"]);
		add_sound(Sound::Name::PORTAL, gamesrc["Portal"]);
		add_sound(Sound::Name::LEVELUP, gamesrc["LevelUp"]);
		add_sound(Sound::Name::HURTDAMAGE, gamesrc["Damage"]);
		add_sound(Sound::Name::QUESTCOMPLETE, gamesrc["QuestClear"]);
		add_sound(Sound::Name::QUESTALERT, gamesrc["QuestAlert"]);
		add_sound(Sound::Name::QUESTCOUNT, gamesrc["questCount"]);
		add_sound(Sound::Name::TOMBSTONE, gamesrc["Tombstone"]);

		nl::node itemsrc = nl::nx::sound["Item.img"];

		for (auto node : itemsrc)
			add_sound(node.name(), node["Use"]);

		uint8_t volume = Setting<SFXVolume>::get().load();

		if (!set_sfxvolume(volume))
			return Error::Code::AUDIO;

		return Error::Code::NONE;
	}

	void Sound::close()
	{
#ifdef OPENSTORY_SDL
		openstory_diagnostics_checkpoint("audio-halt-music");
		Mix_HaltMusic();
		openstory_diagnostics_checkpoint("audio-halt-channels");
		Mix_HaltChannel(-1);
		openstory_diagnostics_checkpoint("audio-free-music");
		if (music) Mix_FreeMusic(music);
		music = nullptr;
		music_path.clear();
		openstory_diagnostics_checkpoint("audio-free-samples");
		for (auto sample : samples) Mix_FreeChunk(reinterpret_cast<Mix_Chunk*>(sample.second));
		samples.clear();
		openstory_diagnostics_checkpoint("audio-close-device");
		Mix_CloseAudio();
		openstory_diagnostics_checkpoint("audio-quit-mixer");
		Mix_Quit();
		openstory_diagnostics_checkpoint("audio-closed");
#else
		BASS_Free();
#endif
	}

	bool Sound::set_sfxvolume(uint8_t vol)
	{
#ifdef OPENSTORY_SDL
		Mix_MasterVolume(std::min<int>(vol, 100) * MIX_MAX_VOLUME / 100);
		return true;
#else
		return BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, vol * 100) == TRUE;
#endif
	}

	void Sound::play(size_t id)
	{
#ifdef OPENSTORY_SDL
		play(id, 1, 0);
#else
		if (!samples.count(id))
			return;

		HCHANNEL channel = BASS_SampleGetChannel((HSAMPLE)samples.at(id), false);
		BASS_ChannelPlay(channel, true);
#endif
	}

	void Sound::play(size_t id, float volume, float pan)
	{
		if (!samples.count(id))
			return;

#ifdef OPENSTORY_SDL
		// Set channel properties before starting playback, including reused channels.
		int channel = -1;
		for (int i = 0; i < Mix_AllocateChannels(-1); ++i)
			if (!Mix_Playing(i)) { channel = i; break; }
		if (channel < 0) return;
		pan = std::clamp(pan, -1.f, 1.f);
		Mix_Volume(channel, int(std::clamp(volume, 0.f, 1.f) * MIX_MAX_VOLUME));
		Mix_SetPanning(channel, Uint8(255 * (pan > 0 ? 1 - pan : 1)), Uint8(255 * (pan < 0 ? 1 + pan : 1)));
		Mix_PlayChannel(channel, reinterpret_cast<Mix_Chunk*>(samples.at(id)), 0);
#else
		HCHANNEL channel = BASS_SampleGetChannel((HSAMPLE)samples.at(id), false);
		// Per-channel volume multiplies with the global SFX volume, so the
		// user's volume setting is still respected.
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, volume);
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, pan);
		BASS_ChannelPlay(channel, true);
#endif
	}

	size_t Sound::add_sound(nl::node src)
	{
		nl::audio ad = src;
		if (ad)
		{
			size_t id = ad.id();

			if (samples.find(id) != samples.end())
				return id;

#ifdef OPENSTORY_SDL
			auto* stream = audio_stream(ad);
			auto* chunk = stream ? Mix_LoadWAV_RW(stream, 1) : nullptr;
			if (!chunk) return 0;
			samples[id] = reinterpret_cast<uint64_t>(chunk);
#else
			auto data = ad.data();
			if (!data) return 0;
			samples[id] = BASS_SampleLoad(true, data, 82, (DWORD)ad.length(), 4, BASS_SAMPLE_OVER_POS);
#endif

			return id;
		}
		else
		{
			return 0;
		}
	}

	void Sound::add_sound(Name name, nl::node src)
	{
		size_t id = add_sound(src);

		if (id)
			soundids[name] = id;
	}

	void Sound::add_sound(std::string itemid, nl::node src)
	{
		size_t id = add_sound(src);

		if (id)
			itemids[itemid] = id;
	}

	std::string Sound::format_id(int32_t itemid)
	{
		std::string strid = std::to_string(itemid);
		if (strid.size() < 8)
			strid.insert(0, 8 - strid.size(), '0');

		return strid;
	}

	void Sound::set_listener_position(Point<int16_t> position)
	{
		listener_position = position;
	}

	std::unordered_map<size_t, uint64_t> Sound::samples;
	EnumMap<Sound::Name, size_t> Sound::soundids;
	std::unordered_map<std::string, size_t> Sound::itemids;
	Point<int16_t> Sound::listener_position;

	Music::Music(std::string p)
	{
		path = p;
	}

	void Music::play() const
	{
#ifdef OPENSTORY_SDL
		play_music(path, -1);
#else
		static HSTREAM stream = 0;
		static std::string bgmpath = "";

		if (path == bgmpath)
			return;

		nl::audio ad = nl::nx::sound.resolve(path);
		auto data = reinterpret_cast<const void*>(ad.data());

		if (data)
		{
			if (stream)
			{
				BASS_ChannelStop(stream);
				BASS_StreamFree(stream);
			}

			stream = BASS_StreamCreateFile(true, data, 82, ad.length(), BASS_SAMPLE_FLOAT | BASS_SAMPLE_LOOP);
			BASS_ChannelPlay(stream, true);

			bgmpath = path;
		}
#endif
	}

	void Music::play_once() const
	{
#ifdef OPENSTORY_SDL
		play_music(path, 0);
#else
		static HSTREAM stream = 0;
		static std::string bgmpath = "";

		if (path == bgmpath)
			return;

		nl::audio ad = nl::nx::sound.resolve(path);
		auto data = reinterpret_cast<const void*>(ad.data());

		if (data)
		{
			if (stream)
			{
				BASS_ChannelStop(stream);
				BASS_StreamFree(stream);
			}

			stream = BASS_StreamCreateFile(true, data, 82, ad.length(), BASS_SAMPLE_FLOAT);
			BASS_ChannelPlay(stream, true);

			bgmpath = path;
		}
#endif
	}

	Error Music::init()
	{
		uint8_t volume = Setting<BGMVolume>::get().load();

		if (!set_bgmvolume(volume))
			return Error::Code::AUDIO;

		return Error::Code::NONE;
	}

	bool Music::set_bgmvolume(uint8_t vol)
	{
#ifdef OPENSTORY_SDL
		Mix_VolumeMusic(std::min<int>(vol, 100) * MIX_MAX_VOLUME / 100);
		return true;
#else
		return BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, vol * 100) == TRUE;
#endif
	}
}

#ifdef OPENSTORY_SDL
namespace
{
	SDL_RWops* audio_stream(nl::audio audio)
	{
#ifdef NLNX_STREAMING
		if (!audio || audio.length() <= 82 || audio.length() > INT_MAX) return nullptr;
		struct Cursor { nl::audio source; Sint64 position = 0; };
		auto* stream = SDL_AllocRW();
		if (!stream) return nullptr;
		auto* cursor = new (std::nothrow) Cursor{audio};
		if (!cursor) { SDL_FreeRW(stream); return nullptr; }
		stream->type = SDL_RWOPS_UNKNOWN;
		stream->hidden.unknown.data1 = cursor;
		stream->size = [](SDL_RWops* rw) -> Sint64 {
			return static_cast<Cursor*>(rw->hidden.unknown.data1)->source.length() - 82;
		};
		stream->seek = [](SDL_RWops* rw, Sint64 offset, int whence) -> Sint64 {
			auto* c = static_cast<Cursor*>(rw->hidden.unknown.data1);
			const Sint64 length = c->source.length() - 82;
			Sint64 start;
			switch (whence) {
			case RW_SEEK_SET: start = 0; break;
			case RW_SEEK_CUR: start = c->position; break;
			case RW_SEEK_END: start = length; break;
			default: return -1;
			}
			if (offset < -start || offset > length - start) return -1;
			return c->position = start + offset;
		};
		stream->read = [](SDL_RWops* rw, void* output, size_t size, size_t count) -> size_t {
			auto* c = static_cast<Cursor*>(rw->hidden.unknown.data1);
			if (!size) return 0;
			const size_t remaining = c->source.length() - 82 - c->position;
			count = std::min(count, remaining / size);
			const size_t bytes = size * count;
			if (!c->source.read(output, size_t(c->position) + 82, bytes)) return 0;
			c->position += bytes;
			return count;
		};
		stream->write = [](SDL_RWops*, const void*, size_t, size_t) -> size_t { return 0; };
		stream->close = [](SDL_RWops* rw) -> int {
			delete static_cast<Cursor*>(rw->hidden.unknown.data1);
			SDL_FreeRW(rw);
			return 0;
		};
		return stream;
#else
		if (!audio.data() || audio.length() <= 82 || audio.length() > INT_MAX) return nullptr;
		return SDL_RWFromConstMem(static_cast<const char*>(audio.data()) + 82, int(audio.length() - 82));
#endif
	}
	void play_music(const std::string& path, int loops)
	{
		if (path == music_path && Mix_PlayingMusic()) return;
		auto* stream = audio_stream(nl::nx::sound.resolve(path));
		auto* next = stream ? Mix_LoadMUS_RW(stream, 1) : nullptr;
		if (!next) return;
		Mix_HaltMusic();
		if (music) Mix_FreeMusic(music);
		music = next;
		music_path = path;
		Mix_PlayMusic(music, loops);
	}
}
bool check_sdl_audio()
{
#ifdef NLNX_STREAMING
	nl::audio source = nl::nx::sound.resolve("BgmUI.img/Title");
	auto* input = audio_stream(source);
	if (!input) { SDL_Log("NX audio stream could not be opened"); return false; }
	char actual[128], expected[128];
	const Sint64 length = SDL_RWsize(input);
	bool stream_ok = length == source.length() - 82 &&
		SDL_RWseek(input, 32, RW_SEEK_SET) == 32 &&
		SDL_RWread(input, actual, 1, sizeof(actual)) == sizeof(actual) &&
		source.read(expected, 82 + 32, sizeof(expected)) &&
		std::memcmp(actual, expected, sizeof(actual)) == 0 &&
		SDL_RWseek(input, -1, RW_SEEK_END) == length - 1 &&
		SDL_RWread(input, actual, 1, sizeof(actual)) == 1 &&
		SDL_RWread(input, actual, 1, 1) == 0 &&
		SDL_RWseek(input, -1, RW_SEEK_SET) == -1 &&
		SDL_RWseek(input, 1, RW_SEEK_END) == -1;
	SDL_RWclose(input);
	if (!stream_ok) { SDL_Log("NX audio stream read/seek check failed"); return false; }
#endif
	ms::Sound(ms::Sound::Name::BUTTONCLICK).play();
	bool sample_playing = Mix_Playing(-1) > 0;
	play_music("BgmUI.img/Title", -1);
	if (!sample_playing || !music || !Mix_PlayingMusic())
		SDL_Log("NX playback check: sample=%d music=%d: %s", sample_playing,
			music && Mix_PlayingMusic(), Mix_GetError());
	return sample_playing && music && Mix_PlayingMusic();
}
#endif
