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

#include <bass.h>

#include <cctype>
#include <cmath>
#include <string>

#ifdef USE_NX
#include <nlnx/audio.hpp>
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		bool name_iequals(const std::string& a, const std::string& b)
		{
			if (a.size() != b.size())
				return false;

			for (size_t i = 0; i < a.size(); i++)
			{
				auto left = std::tolower(static_cast<unsigned char>(a[i]));
				auto right = std::tolower(static_cast<unsigned char>(b[i]));

				if (left != right)
					return false;
			}

			return true;
		}

		// Resolve a '/'-separated node path, falling back to a case-insensitive
		// scan of a level's children whenever the exact name is not there.
		//
		// NoLifeNx looks children up with a bytewise binary search, so node
		// names are case-sensitive, and a few vanilla Map.wz "info/bgm" strings
		// do not match the real node names in Sound.wz. The known one in GMS
		// v83 is "BGM06.img/FinalFight" (22 maps), which Nexon mis-cased: the
		// track really lives at "Bgm06.img/FinalFight". Without this fallback
		// the lookup silently returns a null node and those maps play nothing.
		//
		// The exact lookup is tried first, so normal names cost nothing extra;
		// the linear rescan only runs on the miss path.
		nl::node resolve_relaxed(nl::node root, const std::string& path)
		{
			nl::node exact = root.resolve(path);

			if (exact)
				return exact;

			nl::node current = root;

			for (size_t pos = 0; pos < path.size(); )
			{
				size_t slash = path.find('/', pos);
				size_t len = (slash == std::string::npos) ? path.size() - pos : slash - pos;
				std::string segment = path.substr(pos, len);

				pos = (slash == std::string::npos) ? path.size() : slash + 1;

				if (segment.empty())
					continue;

				nl::node child = current[segment];

				if (!child)
				{
					for (nl::node candidate : current)
					{
						if (name_iequals(candidate.name(), segment))
						{
							child = candidate;
							break;
						}
					}
				}

				// Still nothing: the path genuinely is not in this NX file.
				// GMS v83 Sound.nx has no "Bgm20.img/Subway" and no "BgmTH.img"
				// at all, and both are referenced by Map.nx. Returning the null
				// node is deliberate -- node/audio conversions on a null node
				// yield audio{nullptr, 0}, so the callers below see a null data
				// pointer and simply keep the current track. No crash, no log.
				if (!child)
					return nl::node();

				current = child;
			}

			return current;
		}
	}

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
		if (!BASS_Init(-1, 44100, 0, nullptr, 0))
			return Error::Code::AUDIO;

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
		BASS_Free();
	}

	bool Sound::set_sfxvolume(uint8_t vol)
	{
		return BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, vol * 100) == TRUE;
	}

	void Sound::play(size_t id)
	{
		if (!samples.count(id))
			return;

		HCHANNEL channel = BASS_SampleGetChannel((HSAMPLE)samples.at(id), false);
		BASS_ChannelPlay(channel, true);
	}

	void Sound::play(size_t id, float volume, float pan)
	{
		if (!samples.count(id))
			return;

		HCHANNEL channel = BASS_SampleGetChannel((HSAMPLE)samples.at(id), false);
		// Per-channel volume multiplies with the global SFX volume, so the
		// user's volume setting is still respected.
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, volume);
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, pan);
		BASS_ChannelPlay(channel, true);
	}

	size_t Sound::add_sound(nl::node src)
	{
		nl::audio ad = src;

		auto data = reinterpret_cast<const void*>(ad.data());

		if (data)
		{
			size_t id = ad.id();

			if (samples.find(id) != samples.end())
				return id;

			samples[id] = BASS_SampleLoad(true, data, 82, (DWORD)ad.length(), 4, BASS_SAMPLE_OVER_POS);

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
		static HSTREAM stream = 0;
		static std::string bgmpath = "";

		if (path == bgmpath)
			return;

		nl::audio ad = resolve_relaxed(nl::nx::sound, path);
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
	}

	void Music::play_once() const
	{
		static HSTREAM stream = 0;
		static std::string bgmpath = "";

		if (path == bgmpath)
			return;

		nl::audio ad = resolve_relaxed(nl::nx::sound, path);
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
		return BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, vol * 100) == TRUE;
	}
}