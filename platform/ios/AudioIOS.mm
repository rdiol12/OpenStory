//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Audio Implementation using OpenAL
//	Replaces the BASS audio library for iOS.
//	OpenAL is available on iOS (though deprecated — works fine for now).
//////////////////////////////////////////////////////////////////////////////////
#include "Audio/Audio.h"

#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#import <AVFoundation/AVFoundation.h>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/audio.hpp>
#endif

namespace ms
{
	// Static members
	std::unordered_map<size_t, uint64_t> Sound::samples;
	EnumMap<Sound::Name, size_t> Sound::soundids;
	std::unordered_map<std::string, size_t> Sound::itemids;

	namespace
	{
		ALCdevice* al_device = nullptr;
		ALCcontext* al_context = nullptr;
		uint8_t sfx_volume = 100;
		uint8_t bgm_volume = 100;

		AVAudioPlayer* bgm_player = nil;
		std::string current_bgm;

		// Simple source pool for sound effects
		static const int MAX_SOURCES = 32;
		ALuint sources[MAX_SOURCES];
		int next_source = 0;
	}

	Sound::Sound(Name name) : id(soundids[name]) {}
	Sound::Sound(int32_t itemid) : id(0)
	{
		std::string strid = format_id(itemid);
		auto iter = itemids.find(strid);

		if (iter != itemids.end())
			id = iter->second;
	}
	Sound::Sound(nl::node src) : id(add_sound(src)) {}
	Sound::Sound() : id(0) {}

	std::string Sound::format_id(int32_t itemid)
	{
		std::string strid = std::to_string(itemid);
		strid.insert(0, 8 - strid.length(), '0');
		return strid;
	}

	size_t Sound::add_sound(nl::node src)
	{
		if (!src)
			return 0;

		nl::audio ad = src;
		auto data = reinterpret_cast<const void*>(ad.data());

		if (data)
		{
			size_t id = ad.id();

			if (samples.find(id) != samples.end())
				return id;

			ALuint buffer;
			alGenBuffers(1, &buffer);

			// NX audio has 82-byte header, actual PCM data starts after
			const void* pcm_data = static_cast<const uint8_t*>(data) + 82;
			size_t pcm_length = ad.length() - 82;

			if (pcm_length > 0)
			{
				alBufferData(buffer, AL_FORMAT_MONO16, pcm_data, static_cast<ALsizei>(pcm_length), 44100);
				samples[id] = buffer;
			}

			return id;
		}

		return 0;
	}

	void Sound::add_sound(Name name, nl::node src)
	{
		size_t sid = add_sound(src);
		soundids[name] = sid;
	}

	void Sound::add_sound(std::string itemid, nl::node src)
	{
		size_t sid = add_sound(src);
		itemids[itemid] = sid;
	}

	void Sound::play(size_t id)
	{
		if (id == 0 || sfx_volume == 0)
			return;

		auto iter = samples.find(id);

		if (iter == samples.end())
			return;

		ALuint buffer = static_cast<ALuint>(iter->second);
		ALuint source = sources[next_source];
		next_source = (next_source + 1) % MAX_SOURCES;

		alSourceStop(source);
		alSourcei(source, AL_BUFFER, buffer);
		alSourcef(source, AL_GAIN, sfx_volume / 100.0f);
		alSourcePlay(source);
	}

	void Sound::play() const
	{
		play(id);
	}

	Error Sound::init()
	{
		// Configure iOS audio session
		NSError* error = nil;
		[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:&error];
		[[AVAudioSession sharedInstance] setActive:YES error:&error];

		// Initialize OpenAL
		al_device = alcOpenDevice(NULL);

		if (!al_device)
			return Error::Code::AUDIO;

		al_context = alcCreateContext(al_device, NULL);

		if (!al_context)
			return Error::Code::AUDIO;

		alcMakeContextCurrent(al_context);

		// Create source pool
		alGenSources(MAX_SOURCES, sources);

		// Load UI sounds
		nl::node uisrc = nl::nx::sound["UI.img"];
		nl::node gamesrc = nl::nx::sound["Game.img"];

		add_sound(Name::BUTTONCLICK, uisrc["BtMouseClick"]);
		add_sound(Name::BUTTONOVER, uisrc["BtMouseOver"]);
		add_sound(Name::CHARSELECT, uisrc["CharSelect"]);
		add_sound(Name::DLGNOTICE, uisrc["DlgNotice"]);
		add_sound(Name::MENUDOWN, uisrc["MenuDown"]);
		add_sound(Name::MENUUP, uisrc["MenuUp"]);
		add_sound(Name::RACESELECT, uisrc["RaceSelect"]);
		add_sound(Name::SCROLLUP, uisrc["ScrollUp"]);
		add_sound(Name::SELECTMAP, uisrc["SelectMap"]);
		add_sound(Name::TAB, uisrc["Tab"]);
		add_sound(Name::WORLDSELECT, uisrc["WorldSelect"]);
		add_sound(Name::DRAGSTART, uisrc["DragStart"]);
		add_sound(Name::DRAGEND, uisrc["DragEnd"]);
		add_sound(Name::WORLDMAPOPEN, uisrc["WorldmapOpen"]);
		add_sound(Name::WORLDMAPCLOSE, uisrc["WorldmapClose"]);
		add_sound(Name::GAMESTART, uisrc["GameIn"]);
		add_sound(Name::JUMP, gamesrc["Jump"]);
		add_sound(Name::DROP, gamesrc["DropItem"]);
		add_sound(Name::PICKUP, gamesrc["PickUpItem"]);
		add_sound(Name::PORTAL, gamesrc["Portal"]);
		add_sound(Name::LEVELUP, gamesrc["LevelUp"]);
		add_sound(Name::HURTDAMAGE, gamesrc["Damage"]);
		add_sound(Name::QUESTCOMPLETE, gamesrc["QuestComplete"]);
		add_sound(Name::TOMBSTONE, gamesrc["Tombstone"]);

		return Error::Code::NONE;
	}

	void Sound::close()
	{
		// Clean up OpenAL
		alDeleteSources(MAX_SOURCES, sources);

		for (auto& pair : samples)
			alDeleteBuffers(1, reinterpret_cast<ALuint*>(&pair.second));

		samples.clear();

		if (al_context)
		{
			alcMakeContextCurrent(NULL);
			alcDestroyContext(al_context);
			al_context = nullptr;
		}

		if (al_device)
		{
			alcCloseDevice(al_device);
			al_device = nullptr;
		}

		bgm_player = nil;
	}

	bool Sound::set_sfxvolume(uint8_t volume)
	{
		sfx_volume = volume;
		return true;
	}

	// Music class

	Music::Music(std::string p) : path(p) {}

	void Music::play() const
	{
		if (path.empty() || bgm_volume == 0)
			return;

		if (path == current_bgm)
			return;

		current_bgm = path;

		// Load BGM from NX — music data is typically large OGG/MP3
		// For now, use a silent fallback until we implement NX audio streaming
		// TODO: Implement NX music streaming to AVAudioPlayer
	}

	void Music::play_once() const
	{
		play();
	}

	Error Music::init()
	{
		return Error::Code::NONE;
	}

	bool Music::set_bgmvolume(uint8_t volume)
	{
		bgm_volume = volume;

		if (bgm_player)
			bgm_player.volume = volume / 100.0f;

		return true;
	}
}
