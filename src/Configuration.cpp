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
#include "Configuration.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>

namespace ms
{
	Configuration::Configuration()
	{
		settings.emplace<ServerIP>();
		settings.emplace<ServerPort>();
		settings.emplace<Fullscreen>();
		settings.emplace<Width>();
		settings.emplace<Height>();
		settings.emplace<VSync>();
		settings.emplace<FontPathNormal>();
		settings.emplace<FontPathBold>();
		settings.emplace<FontPathEmoji>();
		settings.emplace<FontPathCJK>();
		settings.emplace<BGMVolume>();
		settings.emplace<SFXVolume>();
		settings.emplace<SaveLogin>();
		settings.emplace<DefaultAccount>();
		settings.emplace<DefaultPassword>();
		settings.emplace<DefaultWorld>();
		settings.emplace<DefaultChannel>();
		settings.emplace<DefaultRegion>();
		settings.emplace<DefaultCharacter>();
		settings.emplace<Chatopen>();
		settings.emplace<PosSTATS>();
		settings.emplace<PosEQINV>();
		settings.emplace<PosINV>();
		settings.emplace<PosSKILL>();
		settings.emplace<PosQUEST>();
		settings.emplace<PosQUESTHELPER>();
		settings.emplace<PosMAP>();
		settings.emplace<PosUSERLIST>();
		settings.emplace<PosCHAT>();
		settings.emplace<PosMINIMAP>();
		settings.emplace<PosSHOP>();
		settings.emplace<PosNOTICE>();
		settings.emplace<PosMAPLECHAT>();
		settings.emplace<PosCHANNEL>();
		settings.emplace<PosJOYPAD>();
		settings.emplace<PosEVENT>();
		settings.emplace<PosKEYCONFIG>();
		settings.emplace<PosOPTIONMENU>();
		settings.emplace<PosCHARINFO>();
		settings.emplace<PosBUDDYLIST>();
		settings.emplace<PosADDBUDDY>();
		settings.emplace<PosBUDDYGROUP>();
		settings.emplace<PosPARTYSETTINGS>();
		settings.emplace<PosPARTYINVITE>();
		settings.emplace<PosPARTYHELPER>();
		settings.emplace<PosSTORAGE>();
		settings.emplace<PosTRADE>();
		settings.emplace<PosGUILD>();
		settings.emplace<PosGUILDBBS>();
		settings.emplace<PosGUILDMARK>();
		settings.emplace<PosALLIANCE>();
		settings.emplace<PosMESSENGER>();
		settings.emplace<PosPERSONALSHOP>();
		settings.emplace<PosHIREDMERCHANT>();
		settings.emplace<PosMINIGAME>();
		settings.emplace<PosMONSTERBOOK>();
		settings.emplace<PosPARTYSEARCH>();
		settings.emplace<PosRANKING>();
		settings.emplace<PosSKILLMACRO>();
		settings.emplace<PosFAMILY>();
		settings.emplace<PosFAMILYTREE>();
		settings.emplace<PosSYSTEMOPTION>();
		settings.emplace<PosMAPLETV>();
		settings.emplace<PosMEGAPHONE>();
		settings.emplace<PosITEMMEGAPHONE>();
		settings.emplace<PosAVATARMEGAPHONE>();
		settings.emplace<MiniMapType>();
		settings.emplace<MiniMapSimpleMode>();
		settings.emplace<MiniMapDefaultHelpers>();
		settings.emplace<ScreenShake>();
		settings.emplace<GraphicsQuality>();
		settings.emplace<EffectsQuality>();
		settings.emplace<MouseSpeed>();
		settings.emplace<ScreenshotFolder>();
		settings.emplace<HPWarning>();
		settings.emplace<MPWarning>();
		settings.emplace<AllowWhisper>();
		settings.emplace<AllowChatInvite>();
		settings.emplace<AllowPartyInvite>();
		settings.emplace<AllowGuildChat>();
		settings.emplace<AllowAllianceChat>();
		settings.emplace<AllowFamilyInvite>();
		settings.emplace<AllowFriendChat>();
		settings.emplace<AllowTradeRequest>();
		settings.emplace<AllowExpeditionInvite>();
		settings.emplace<AllowGuildInvite>();
		settings.emplace<AllowAllianceInvite>();
		settings.emplace<AllowFollow>();
		settings.emplace<TrackedQuests>();

		load();
	}

	Configuration::~Configuration()
	{
#ifndef DEBUG
		save();
#endif
	}

	void Configuration::load()
	{
		std::unordered_map<std::string, std::string> rawsettings;
		std::ifstream file(FILENAME);

		if (file.is_open())
		{
			// Go through the file line by line
			std::string line;

			while (getline(file, line))
			{
				// If the setting is not empty, load the value.
				size_t split = line.find('=');

				if (split != std::string::npos && split + 2 < line.size())
				{
					rawsettings.emplace(
						line.substr(0, split - 1),
						line.substr(split + 2)
					);
				}
			}
		}

		// Replace default values with loaded values
		for (auto& setting : settings)
		{
			auto rsiter = rawsettings.find(setting.second->name);

			if (rsiter != rawsettings.end())
				setting.second->value = rsiter->second;
		}
	}

	void Configuration::save() const
	{
		std::ofstream config(FILENAME);

		if (!config.is_open())
			return;

		// Section -> ordered key list. Anything not listed falls into Window
		// Positions (if it starts with "Pos") or Other (alphabetical).
		static const std::vector<std::pair<const char*, std::vector<const char*>>> sections = {
			{ "Connection",  { "ServerIP", "ServerPort", "SaveLogin", "Account", "Password",
			                   "World", "Channel", "Character", "Region" } },
			{ "Display",     { "Width", "Height", "Fullscreen", "VSync" } },
			{ "Audio",       { "BGMVolume", "SFXVolume" } },
			{ "Quality",     { "GraphicsQuality", "EffectsQuality", "ScreenShake" } },
			{ "Input",       { "MouseSpeed", "Chatopen" } },
			{ "Warnings",    { "HPWarning", "MPWarning" } },
			{ "Permissions", { "AllowWhisper", "AllowChatInvite", "AllowPartyInvite",
			                   "AllowGuildChat", "AllowAllianceChat", "AllowFamilyInvite",
			                   "AllowFriendChat", "AllowTradeRequest", "AllowExpeditionInvite",
			                   "AllowGuildInvite", "AllowAllianceInvite", "AllowFollow" } },
			{ "Fonts",       { "FontPathNormal", "FontPathBold", "FontPathCJK", "FontPathEmoji" } },
			{ "MiniMap",     { "MiniMapSimpleMode", "MiniMapType", "MiniMapDefaultHelpers" } },
			{ "Misc",        { "ScreenshotFolder", "TrackedQuests" } },
		};

		// Defensive null-skip: TypeMap::get() can insert a null unique_ptr if a
		// type is looked up before being registered in the constructor.
		std::unordered_map<std::string, const Entry*> by_name;
		for (auto& s : settings)
			if (s.second)
				by_name.emplace(s.second->name, s.second.get());

		std::unordered_set<std::string> emitted;

		for (auto& [title, keys] : sections)
		{
			config << "# " << title << std::endl;
			for (auto* key : keys)
			{
				auto it = by_name.find(key);
				if (it != by_name.end())
				{
					config << it->second->to_string() << std::endl;
					emitted.insert(key);
				}
			}
			config << std::endl;
		}

		std::vector<const Entry*> positions;
		std::vector<const Entry*> other;
		for (auto& s : settings)
		{
			if (!s.second || emitted.count(s.second->name))
				continue;
			if (s.second->name.rfind("Pos", 0) == 0)
				positions.push_back(s.second.get());
			else
				other.push_back(s.second.get());
		}

		auto by_name_lt = [](const Entry* a, const Entry* b) { return a->name < b->name; };
		std::sort(positions.begin(), positions.end(), by_name_lt);
		std::sort(other.begin(), other.end(), by_name_lt);

		if (!other.empty())
		{
			config << "# Other" << std::endl;
			for (auto* e : other)
				config << e->to_string() << std::endl;
			config << std::endl;
		}

		if (!positions.empty())
		{
			config << "# Window Positions" << std::endl;
			for (auto* e : positions)
				config << e->to_string() << std::endl;
		}
	}

	void Configuration::BoolEntry::save(bool b)
	{
		value = b ? "true" : "false";
	}

	bool Configuration::BoolEntry::load() const
	{
		return value == "true";
	}

	void Configuration::StringEntry::save(std::string str)
	{
		value = str;
	}

	std::string Configuration::StringEntry::load() const
	{
		return value;
	}

	void Configuration::PointEntry::save(Point<int16_t> vec)
	{
		value = vec.to_string();
	}

	Point<int16_t> Configuration::PointEntry::load() const
	{
		std::string xstr = value.substr(1, value.find(",") - 1);
		std::string ystr = value.substr(value.find(",") + 1, value.find(")") - value.find(",") - 1);

		auto x = string_conversion::or_zero<int16_t>(xstr);
		auto y = string_conversion::or_zero<int16_t>(ystr);

		return { x, y };
	}

	bool Configuration::get_show_fps() const
	{
		return SHOW_FPS;
	}

	bool Configuration::get_show_packets() const
	{
		return SHOW_PACKETS;
	}

	bool Configuration::get_auto_login() const
	{
		return AUTO_LOGIN;
	}

	uint8_t Configuration::get_auto_world()
	{
		return auto_world;
	}

	uint8_t Configuration::get_auto_channel()
	{
		return auto_channel;
	}

	std::string Configuration::get_auto_acc()
	{
		return auto_acc;
	}

	std::string Configuration::get_auto_pass()
	{
		return auto_pass;
	}

	std::string Configuration::get_auto_pic()
	{
		return auto_pic;
	}

	int32_t Configuration::get_auto_cid()
	{
		return auto_cid;
	}

	std::string Configuration::get_title() const
	{
		return TITLE;
	}

	std::string Configuration::get_version() const
	{
		return VERSION;
	}

	std::string Configuration::get_login_music() const
	{
		return LoginMusic;
	}

	std::string Configuration::get_login_music_sea() const
	{
		return LoginMusicSEA;
	}

	std::string Configuration::get_login_music_newtro() const
	{
		return LoginMusicNewtro;
	}

	std::string Configuration::get_joinlink() const
	{
		return JOINLINK;
	}

	std::string Configuration::get_website() const
	{
		return WEBSITE;
	}

	std::string Configuration::get_findid() const
	{
		return FINDID;
	}

	std::string Configuration::get_findpass() const
	{
		return FINDPASS;
	}

	std::string Configuration::get_resetpic() const
	{
		return RESETPIC;
	}

	std::string Configuration::get_chargenx() const
	{
		return CHARGENX;
	}

	void Configuration::set_macs(char* macs)
	{
		MACS = macs;
	}

	void Configuration::set_hwid(char* hwid, char* volumeSerialNumber)
	{
		VolumeSerialNumber = volumeSerialNumber;

		std::string newHWID;

		newHWID.append(hwid);
		newHWID.append("_");

		std::string part1 = VolumeSerialNumber.substr(0, 2);
		std::string part2 = VolumeSerialNumber.substr(2, 2);
		std::string part3 = VolumeSerialNumber.substr(4, 2);
		std::string part4 = VolumeSerialNumber.substr(6, 2);

		newHWID.append(part4);
		newHWID.append(part3);
		newHWID.append(part2);
		newHWID.append(part1);

		HWID = newHWID;
	}

	void Configuration::set_max_width(int16_t max_width)
	{
		MAXWIDTH = max_width;
	}

	void Configuration::set_max_height(int16_t max_height)
	{
		MAXHEIGHT = max_height;
	}

	std::string Configuration::get_macs()
	{
		return MACS;
	}

	std::string Configuration::get_hwid()
	{
		return HWID;
	}

	std::string Configuration::get_vol_serial_num()
	{
		return VolumeSerialNumber;
	}

	int16_t Configuration::get_max_width()
	{
		return MAXWIDTH;
	}

	int16_t Configuration::get_max_height()
	{
		return MAXHEIGHT;
	}

	bool Configuration::get_rightclicksell()
	{
		return rightclicksell;
	}

	void Configuration::set_rightclicksell(bool value)
	{
		rightclicksell = value;
	}

	bool Configuration::get_show_weekly()
	{
		return show_weekly;
	}

	void Configuration::set_show_weekly(bool value)
	{
		show_weekly = value;
	}

	bool Configuration::get_start_shown()
	{
		return start_shown;
	}

	void Configuration::set_start_shown(bool value)
	{
		start_shown = value;
	}

	uint8_t Configuration::get_worldid()
	{
		return worldid;
	}

	void Configuration::set_worldid(uint8_t id)
	{
		worldid = id;
	}

	uint8_t Configuration::get_channelid()
	{
		return channelid;
	}

	void Configuration::set_channelid(uint8_t id)
	{
		channelid = id;
	}

	bool Configuration::get_admin()
	{
		return admin;
	}

	void Configuration::set_admin(bool value)
	{
		admin = value;
	}
}