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
#include "UIKeyConfig.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../UITypes/UILoginNotice.h"
#include "../UITypes/UINotice.h"

#include "../../Data/ItemData.h"
#include "../../Data/SkillData.h"

#include "../../Constants.h"
#include "../../Net/Packets/PlayerPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIKeyConfig::UIKeyConfig(const Inventory& in_inventory, const SkillBook& in_skillbook) : UIDragElement<PosKEYCONFIG>(Point<int16_t>(480, 20)), inventory(in_inventory), skillbook(in_skillbook), dirty(false)
	{
		keyboard = &UI::get().get_keyboard();
		staged_mappings = keyboard->get_maplekeys();

		nl::node KeyConfig = nl::nx::ui["UIWindow2.img"]["KeyConfig"];

		icon = KeyConfig["icon"];

		sprites.emplace_back(KeyConfig["backgrnd"]);
		sprites.emplace_back(KeyConfig["backgrnd2"]);
		sprites.emplace_back(KeyConfig["backgrnd3"]);

		buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(KeyConfig["BtCancel"]);
		buttons[Buttons::DEFAULT] = std::make_unique<MapleButton>(KeyConfig["BtDefault"]);
		buttons[Buttons::DELETE] = std::make_unique<MapleButton>(KeyConfig["BtDelete"]);
		buttons[Buttons::KEYSETTING] = std::make_unique<MapleButton>(KeyConfig["BtQuickSlot"]);
		buttons[Buttons::OK] = std::make_unique<MapleButton>(KeyConfig["BtOK"]);

		dimension = Point<int16_t>(622, 374);
		dragarea = Point<int16_t>(622, 20);

		// Center on screen
		int16_t screen_w = Constants::Constants::get().get_viewwidth();
		int16_t screen_h = Constants::Constants::get().get_viewheight();
		position = Point<int16_t>((screen_w - 622) / 2, (screen_h - 374) / 2);

		load_keys_pos();
		load_unbound_actions_pos();
		load_action_mappings();
		load_action_icons();
		load_item_icons();
		load_skill_icons();
		load_macro_icons();

		bind_staged_action_keys();
	}

	/// Load
	void UIKeyConfig::load_keys_pos()
	{
		// Positions from LibreMaple — hardcoded per-key to match the background image exactly

		// F-key row (y=28)
		keys_pos[KeyConfig::Key::ESCAPE] = Point<int16_t>(11, 28);
		keys_pos[KeyConfig::Key::F1] = Point<int16_t>(79, 28);
		keys_pos[KeyConfig::Key::F2] = Point<int16_t>(113, 28);
		keys_pos[KeyConfig::Key::F3] = Point<int16_t>(147, 28);
		keys_pos[KeyConfig::Key::F4] = Point<int16_t>(181, 28);
		keys_pos[KeyConfig::Key::F5] = Point<int16_t>(223, 28);
		keys_pos[KeyConfig::Key::F6] = Point<int16_t>(257, 28);
		keys_pos[KeyConfig::Key::F7] = Point<int16_t>(291, 28);
		keys_pos[KeyConfig::Key::F8] = Point<int16_t>(325, 28);
		keys_pos[KeyConfig::Key::F9] = Point<int16_t>(367, 28);
		keys_pos[KeyConfig::Key::F10] = Point<int16_t>(401, 28);
		keys_pos[KeyConfig::Key::F11] = Point<int16_t>(435, 28);
		keys_pos[KeyConfig::Key::F12] = Point<int16_t>(469, 28);

		// Number row (y=65)
		keys_pos[KeyConfig::Key::GRAVE_ACCENT] = Point<int16_t>(11, 65);
		keys_pos[KeyConfig::Key::NUM1] = Point<int16_t>(45, 65);
		keys_pos[KeyConfig::Key::NUM2] = Point<int16_t>(79, 65);
		keys_pos[KeyConfig::Key::NUM3] = Point<int16_t>(113, 65);
		keys_pos[KeyConfig::Key::NUM4] = Point<int16_t>(147, 65);
		keys_pos[KeyConfig::Key::NUM5] = Point<int16_t>(181, 65);
		keys_pos[KeyConfig::Key::NUM6] = Point<int16_t>(215, 65);
		keys_pos[KeyConfig::Key::NUM7] = Point<int16_t>(249, 65);
		keys_pos[KeyConfig::Key::NUM8] = Point<int16_t>(283, 65);
		keys_pos[KeyConfig::Key::NUM9] = Point<int16_t>(317, 65);
		keys_pos[KeyConfig::Key::NUM0] = Point<int16_t>(351, 65);
		keys_pos[KeyConfig::Key::MINUS] = Point<int16_t>(385, 65);
		keys_pos[KeyConfig::Key::EQUAL] = Point<int16_t>(419, 65);

		// QWERTY row (y=99)
		keys_pos[KeyConfig::Key::Q] = Point<int16_t>(61, 99);
		keys_pos[KeyConfig::Key::W] = Point<int16_t>(95, 99);
		keys_pos[KeyConfig::Key::E] = Point<int16_t>(129, 99);
		keys_pos[KeyConfig::Key::R] = Point<int16_t>(163, 99);
		keys_pos[KeyConfig::Key::T] = Point<int16_t>(197, 99);
		keys_pos[KeyConfig::Key::Y] = Point<int16_t>(231, 99);
		keys_pos[KeyConfig::Key::U] = Point<int16_t>(265, 99);
		keys_pos[KeyConfig::Key::I] = Point<int16_t>(299, 99);
		keys_pos[KeyConfig::Key::O] = Point<int16_t>(333, 99);
		keys_pos[KeyConfig::Key::P] = Point<int16_t>(367, 99);
		keys_pos[KeyConfig::Key::LEFT_BRACKET] = Point<int16_t>(401, 99);
		keys_pos[KeyConfig::Key::RIGHT_BRACKET] = Point<int16_t>(435, 99);
		keys_pos[KeyConfig::Key::BACKSLASH] = Point<int16_t>(469, 99);

		// ASDF row (y=132)
		keys_pos[KeyConfig::Key::A] = Point<int16_t>(78, 132);
		keys_pos[KeyConfig::Key::S] = Point<int16_t>(112, 132);
		keys_pos[KeyConfig::Key::D] = Point<int16_t>(146, 132);
		keys_pos[KeyConfig::Key::F] = Point<int16_t>(180, 132);
		keys_pos[KeyConfig::Key::G] = Point<int16_t>(214, 132);
		keys_pos[KeyConfig::Key::H] = Point<int16_t>(248, 132);
		keys_pos[KeyConfig::Key::J] = Point<int16_t>(282, 132);
		keys_pos[KeyConfig::Key::K] = Point<int16_t>(316, 132);
		keys_pos[KeyConfig::Key::L] = Point<int16_t>(350, 132);
		keys_pos[KeyConfig::Key::SEMICOLON] = Point<int16_t>(384, 132);
		keys_pos[KeyConfig::Key::APOSTROPHE] = Point<int16_t>(418, 132);

		// ZXCV row (y=165)
		keys_pos[KeyConfig::Key::LEFT_SHIFT] = Point<int16_t>(36, 165);
		keys_pos[KeyConfig::Key::Z] = Point<int16_t>(95, 165);
		keys_pos[KeyConfig::Key::X] = Point<int16_t>(129, 165);
		keys_pos[KeyConfig::Key::C] = Point<int16_t>(163, 165);
		keys_pos[KeyConfig::Key::V] = Point<int16_t>(197, 165);
		keys_pos[KeyConfig::Key::B] = Point<int16_t>(231, 165);
		keys_pos[KeyConfig::Key::N] = Point<int16_t>(265, 165);
		keys_pos[KeyConfig::Key::M] = Point<int16_t>(299, 165);
		keys_pos[KeyConfig::Key::COMMA] = Point<int16_t>(333, 165);
		keys_pos[KeyConfig::Key::PERIOD] = Point<int16_t>(367, 165);
		keys_pos[KeyConfig::Key::RIGHT_SHIFT] = Point<int16_t>(407, 165);

		// Bottom row (y=198)
		keys_pos[KeyConfig::Key::LEFT_CONTROL] = Point<int16_t>(19, 198);
		keys_pos[KeyConfig::Key::LEFT_ALT] = Point<int16_t>(120, 198);
		keys_pos[KeyConfig::Key::SPACE] = Point<int16_t>(231, 198);
		keys_pos[KeyConfig::Key::RIGHT_ALT] = Point<int16_t>(362, 198);
		keys_pos[KeyConfig::Key::RIGHT_CONTROL] = Point<int16_t>(453, 198);

		// Quickslot keys
		keys_pos[KeyConfig::Key::INSERT] = Point<int16_t>(511, 65);
		keys_pos[KeyConfig::Key::HOME] = Point<int16_t>(545, 65);
		keys_pos[KeyConfig::Key::PAGE_UP] = Point<int16_t>(579, 65);
		keys_pos[KeyConfig::Key::DELETE] = Point<int16_t>(511, 99);
		keys_pos[KeyConfig::Key::END] = Point<int16_t>(545, 99);
		keys_pos[KeyConfig::Key::PAGE_DOWN] = Point<int16_t>(579, 99);

		keys_pos[KeyConfig::Key::SCROLL_LOCK] = Point<int16_t>(511, 28);
	}

	void UIKeyConfig::load_unbound_actions_pos()
	{
		int16_t row_x = 5;
		int16_t row_y = 267;

		int16_t slot_width = 34;
		int16_t slot_height = 34;

		// Row 0
		unbound_actions_pos[KeyAction::Id::MAPLECHAT] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::TOGGLECHAT] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::WHISPER] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::MEDALS] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::BOSSPARTY] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::PROFESSION] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::EQUIPMENT] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::ITEMS] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::CHARINFO] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::MENU] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::QUICKSLOTS] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::PICKUP] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::SIT] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::ATTACK] = Point<int16_t>(row_x + (slot_width * 13), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::JUMP] = Point<int16_t>(row_x + (slot_width * 14), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::INTERACT_HARVEST] = Point<int16_t>(row_x + (slot_width * 15), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::MONSTERBOOK] = Point<int16_t>(row_x + (slot_width * 16), row_y + (slot_height * 0));

		// Row 1
		unbound_actions_pos[KeyAction::Id::SAY] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::PARTYCHAT] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::FRIENDSCHAT] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::ITEMPOT] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::EVENT] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::SOULWEAPON] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::STATS] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::SKILLS] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::QUESTLOG] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::CHANGECHANNEL] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::GUILD] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::PARTY] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::NOTIFIER] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::FRIENDS] = Point<int16_t>(row_x + (slot_width * 13), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::WORLDMAP] = Point<int16_t>(row_x + (slot_width * 14), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::MINIMAP] = Point<int16_t>(row_x + (slot_width * 15), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::KEYBINDINGS] = Point<int16_t>(row_x + (slot_width * 16), row_y + (slot_height * 1));

		// Row 2
		unbound_actions_pos[KeyAction::Id::GUILDCHAT] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::ALLIANCECHAT] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::CASHSHOP] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MAINMENU] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::SCREENSHOT] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::FAMILY] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::SILENTCRUSADE] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MANAGELEGION] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::BATTLEANALYSIS] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::GUIDE] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::ENHANCEEQUIP] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MONSTERCOLLECTION] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MAPLENEWS] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 2));

		// Row 3: Faces
		unbound_actions_pos[KeyAction::Id::FACE1] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE2] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE3] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE4] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE5] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE6] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE7] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 3));
	}

	void UIKeyConfig::load_action_mappings()
	{
		// Menu actions (type 4)
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::EQUIPMENT), KeyAction::Id::EQUIPMENT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::ITEMS), KeyAction::Id::ITEMS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::STATS), KeyAction::Id::STATS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::SKILLS), KeyAction::Id::SKILLS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FRIENDS), KeyAction::Id::FRIENDS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::WORLDMAP), KeyAction::Id::WORLDMAP));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MAPLECHAT), KeyAction::Id::MAPLECHAT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MINIMAP), KeyAction::Id::MINIMAP));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::QUESTLOG), KeyAction::Id::QUESTLOG));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::KEYBINDINGS), KeyAction::Id::KEYBINDINGS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::SAY), KeyAction::Id::SAY));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::WHISPER), KeyAction::Id::WHISPER));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::PARTYCHAT), KeyAction::Id::PARTYCHAT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FRIENDSCHAT), KeyAction::Id::FRIENDSCHAT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MENU), KeyAction::Id::MENU));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::QUICKSLOTS), KeyAction::Id::QUICKSLOTS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::TOGGLECHAT), KeyAction::Id::TOGGLECHAT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::GUILD), KeyAction::Id::GUILD));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::GUILDCHAT), KeyAction::Id::GUILDCHAT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::PARTY), KeyAction::Id::PARTY));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::NOTIFIER), KeyAction::Id::NOTIFIER));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::CHARINFO), KeyAction::Id::CHARINFO));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MONSTERBOOK), KeyAction::Id::MONSTERBOOK));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::CASHSHOP), KeyAction::Id::CASHSHOP));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::ALLIANCECHAT), KeyAction::Id::ALLIANCECHAT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::CHANGECHANNEL), KeyAction::Id::CHANGECHANNEL));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MEDALS), KeyAction::Id::MEDALS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::BOSSPARTY), KeyAction::Id::BOSSPARTY));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MAINMENU), KeyAction::Id::MAINMENU));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::SCREENSHOT), KeyAction::Id::SCREENSHOT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::PROFESSION), KeyAction::Id::PROFESSION));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::ITEMPOT), KeyAction::Id::ITEMPOT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::EVENT), KeyAction::Id::EVENT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::SILENTCRUSADE), KeyAction::Id::SILENTCRUSADE));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MANAGELEGION), KeyAction::Id::MANAGELEGION));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::BATTLEANALYSIS), KeyAction::Id::BATTLEANALYSIS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::GUIDE), KeyAction::Id::GUIDE));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::ENHANCEEQUIP), KeyAction::Id::ENHANCEEQUIP));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MONSTERCOLLECTION), KeyAction::Id::MONSTERCOLLECTION));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::SOULWEAPON), KeyAction::Id::SOULWEAPON));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::MAPLENEWS), KeyAction::Id::MAPLENEWS));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FAMILY), KeyAction::Id::FAMILY));
		// Game actions (type 5)
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::PICKUP), KeyAction::Id::PICKUP));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::SIT), KeyAction::Id::SIT));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::ATTACK), KeyAction::Id::ATTACK));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::JUMP), KeyAction::Id::JUMP));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::INTERACT_HARVEST), KeyAction::Id::INTERACT_HARVEST));
		// Face actions (type 6)
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE1), KeyAction::Id::FACE1));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE2), KeyAction::Id::FACE2));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE3), KeyAction::Id::FACE3));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE4), KeyAction::Id::FACE4));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE5), KeyAction::Id::FACE5));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE6), KeyAction::Id::FACE6));
		action_mappings.push_back(Keyboard::Mapping(get_keytype(KeyAction::Id::FACE7), KeyAction::Id::FACE7));
	}

	void UIKeyConfig::load_action_icons()
	{
		// v83 icon indices match action IDs in UIWindow.img/KeyConfig/icon
		// Menu actions (0-41)
		std::vector<std::pair<KeyAction::Id, int32_t>> menu_actions = {
			{ KeyAction::Id::EQUIPMENT, 0 },
			{ KeyAction::Id::ITEMS, 1 },
			{ KeyAction::Id::STATS, 2 },
			{ KeyAction::Id::SKILLS, 3 },
			{ KeyAction::Id::FRIENDS, 4 },
			{ KeyAction::Id::WORLDMAP, 5 },
			{ KeyAction::Id::MAPLECHAT, 6 },
			{ KeyAction::Id::MINIMAP, 7 },
			{ KeyAction::Id::QUESTLOG, 8 },
			{ KeyAction::Id::KEYBINDINGS, 9 },
			{ KeyAction::Id::SAY, 10 },
			{ KeyAction::Id::WHISPER, 11 },
			{ KeyAction::Id::PARTYCHAT, 12 },
			{ KeyAction::Id::FRIENDSCHAT, 13 },
			{ KeyAction::Id::MENU, 14 },
			{ KeyAction::Id::QUICKSLOTS, 15 },
			{ KeyAction::Id::TOGGLECHAT, 16 },
			{ KeyAction::Id::GUILD, 17 },
			{ KeyAction::Id::GUILDCHAT, 18 },
			{ KeyAction::Id::PARTY, 19 },
			{ KeyAction::Id::NOTIFIER, 20 },
			{ KeyAction::Id::CHARINFO, 21 },
			{ KeyAction::Id::MONSTERBOOK, 22 },
			{ KeyAction::Id::CASHSHOP, 23 },
			{ KeyAction::Id::ALLIANCECHAT, 24 },
			{ KeyAction::Id::CHANGECHANNEL, 25 },
			{ KeyAction::Id::MEDALS, 26 },
			{ KeyAction::Id::BOSSPARTY, 27 },
			{ KeyAction::Id::MAINMENU, 28 },
			{ KeyAction::Id::SCREENSHOT, 29 },
			{ KeyAction::Id::PROFESSION, 30 },
			{ KeyAction::Id::ITEMPOT, 31 },
			{ KeyAction::Id::EVENT, 32 },
			{ KeyAction::Id::SILENTCRUSADE, 33 },
			{ KeyAction::Id::MANAGELEGION, 34 },
			{ KeyAction::Id::BATTLEANALYSIS, 35 },
			{ KeyAction::Id::GUIDE, 37 },
			{ KeyAction::Id::ENHANCEEQUIP, 38 },
			{ KeyAction::Id::MONSTERCOLLECTION, 39 },
			{ KeyAction::Id::SOULWEAPON, 40 },
			{ KeyAction::Id::MAPLENEWS, 41 },
			// v83 NX is missing icons 31..49, so FAMILY (nominal idx 42)
			// falls back to the BOSSPARTY sprite at 27 — a group-of-people
			// icon is visually distinct from GUILD (17) and present in v83.
			{ KeyAction::Id::FAMILY, 27 },
		};

		for (auto& [action, idx] : menu_actions)
		{
			if (icon[idx])
				action_icons[action] = std::make_unique<Icon>(std::make_unique<MappingIcon>(action), icon[idx], -1);
		}

		// Game actions (50-54)
		std::vector<std::pair<KeyAction::Id, int32_t>> action_actions = {
			{ KeyAction::Id::PICKUP, 50 },
			{ KeyAction::Id::SIT, 51 },
			{ KeyAction::Id::ATTACK, 52 },
			{ KeyAction::Id::JUMP, 53 },
			{ KeyAction::Id::INTERACT_HARVEST, 54 },
		};

		for (auto& [action, idx] : action_actions)
		{
			if (icon[idx])
				action_icons[action] = std::make_unique<Icon>(std::make_unique<MappingIcon>(action), icon[idx], -1);
		}

		// Face actions (100-106)
		for (int32_t i = 0; i < 7; i++)
		{
			KeyAction::Id face = static_cast<KeyAction::Id>(static_cast<int32_t>(KeyAction::Id::FACE1) + i);
			if (icon[100 + i])
				action_icons[face] = std::make_unique<Icon>(std::make_unique<MappingIcon>(face), icon[100 + i], -1);
		}
	}

	void UIKeyConfig::load_item_icons()
	{
		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping mapping = it.second;

			if (mapping.type == KeyType::Id::ITEM)
			{
				int32_t item_id = mapping.action;
				int16_t count = inventory.get_total_item_count(item_id);
				Texture tx = get_item_texture(item_id);

				item_icons[item_id] = std::make_unique<Icon>(std::make_unique<CountableMappingIcon>(mapping, count), tx, count);
			}
		}
	}

	void UIKeyConfig::load_skill_icons()
	{
		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping mapping = it.second;

			if (mapping.type == KeyType::Id::SKILL)
			{
				int32_t skill_id = mapping.action;
				Texture tx = get_skill_texture(skill_id);

				skill_icons[skill_id] = std::make_unique<Icon>(std::make_unique<MappingIcon>(mapping), tx, -1);
			}
		}
	}

	void UIKeyConfig::load_macro_icons()
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["SkillMacro"]["Macroicon"];
		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping mapping = it.second;

			if (mapping.type == KeyType::Id::MACRO)
			{
				int32_t idx = mapping.action;
				Texture tx = src[std::to_string(idx)]["icon"];
				macro_icons[idx] = std::make_unique<Icon>(std::make_unique<MappingIcon>(mapping), tx, -1);
			}
		}
	}

	/// UI: General
	void UIKeyConfig::draw(float inter) const
	{
		UIElement::draw(inter);

		Point<int16_t> icon_offset(0, 0);

		for (auto const& iter : staged_mappings)
		{
			KeyConfig::Key key_slot = KeyConfig::actionbyid(iter.first);
			Keyboard::Mapping mapping = iter.second;

			Icon* ficon = nullptr;

			if (mapping.type == KeyType::Id::ITEM)
			{
				auto it = item_icons.find(mapping.action);
				if (it != item_icons.end())
					ficon = it->second.get();
			}
			else if (mapping.type == KeyType::Id::SKILL)
			{
				auto it = skill_icons.find(mapping.action);
				if (it != skill_icons.end())
					ficon = it->second.get();
			}
			else if (mapping.type == KeyType::Id::MACRO)
			{
				auto it = macro_icons.find(mapping.action);
				if (it != macro_icons.end())
					ficon = it->second.get();
			}
			else if (is_action_mapping(mapping))
			{
				KeyAction::Id action = KeyAction::actionbyid(mapping.action);

				for (auto const& it : action_icons)
				{
					if (it.first == action)
					{
						ficon = it.second.get();
						break;
					}
				}
			}

			if (ficon)
			{
				ficon->draw(position + keys_pos[key_slot] - icon_offset);
			}
		}

		Point<int16_t> unbound_center(1, 1); // center 32x32 icon in 34x34 cell

		for (auto const& ubicon : action_icons)
			if (ubicon.second)
				if (std::find(bound_actions.begin(), bound_actions.end(), ubicon.first) == bound_actions.end())
					ubicon.second->draw(position + unbound_actions_pos[ubicon.first] + unbound_center);

	}

	void UIKeyConfig::close()
	{
		clear_tooltip();
		deactivate();
		reset();
	}

	Button::State UIKeyConfig::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::CLOSE:
		case Buttons::CANCEL:
			close();
			break;
		case Buttons::DEFAULT:
		{
			constexpr char* message = "Would you like to revert to default settings?";

			auto onok = [&](bool ok)
			{
				if (ok)
				{
					auto keysel_onok = [&](bool alternate)
					{
						clear();

						if (alternate)
							staged_mappings = alternate_keys;
						else
							staged_mappings = basic_keys;

						bind_staged_action_keys();
					};

					UI::get().emplace<UIKeySelect>(keysel_onok, false);
				}
			};

			UI::get().emplace<UIOk>(message, onok);
			break;
		}
		case Buttons::DELETE:
		{
			constexpr char* message = "Would you like to clear all key bindings?";

			auto onok = [&](bool ok)
			{
				if (ok)
					clear();
			};

			UI::get().emplace<UIOk>(message, onok);
			break;
		}
		case Buttons::KEYSETTING:
			break;
		case Buttons::OK:
		{
			save_staged_mappings();
			close();
			break;
		}
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	Cursor::State UIKeyConfig::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		KeyAction::Id icon_slot = unbound_action_by_position(cursorpos);

		if (icon_slot != KeyAction::Id::LENGTH)
		{
			if (auto icon = action_icons[icon_slot].get())
			{
				if (clicked)
				{
					icon->start_drag(cursorpos - position - unbound_actions_pos[icon_slot]);
					UI::get().drag_icon(icon);

					return Cursor::State::GRABBING;
				}
				else
				{
					return Cursor::State::CANGRAB;
				}
			}
		}

		clear_tooltip();

		KeyConfig::Key key_slot = key_by_position(cursorpos);

		if (key_slot != KeyConfig::Key::LENGTH)
		{
			Keyboard::Mapping mapping = get_staged_mapping(key_slot);

			if (mapping.type != KeyType::Id::NONE)
			{
				Icon* ficon = nullptr;

				if (mapping.type == KeyType::Id::ITEM)
				{
					int32_t item_id = mapping.action;
					ficon = item_icons[item_id].get();

					show_item(item_id);
				}
				else if (mapping.type == KeyType::Id::SKILL)
				{
					int32_t skill_id = mapping.action;
					ficon = skill_icons[skill_id].get();

					show_skill(skill_id);
				}
				else if (mapping.type == KeyType::Id::MACRO)
				{
					int32_t idx = mapping.action;
					ficon = macro_icons[idx].get();
				}
				else if (is_action_mapping(mapping))
				{
					KeyAction::Id action = KeyAction::actionbyid(mapping.action);
					ficon = action_icons[action].get();
				}
				else
				{
					// Unknown mapping type, skip
				}

				if (ficon)
				{
					if (clicked)
					{
						clear_tooltip();

						ficon->start_drag(cursorpos - position - keys_pos[key_slot]);
						UI::get().drag_icon(ficon);

						return Cursor::State::GRABBING;
					}
					else
					{
						return Cursor::State::CANGRAB;
					}
				}
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	bool UIKeyConfig::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		Point<int16_t> relative = cursorpos - position;

		// If cursor is in the unbound actions area (below the keyboard), remove the binding
		if (relative.y() >= 250)
		{
			icon.drop_on_bindings(cursorpos, true);
			return true;
		}

		// Otherwise try to place on a key
		KeyConfig::Key fkey = key_by_position(cursorpos);

		if (fkey != KeyConfig::Key::LENGTH)
			icon.drop_on_bindings(cursorpos, false);

		return true;
	}

	void UIKeyConfig::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			safe_close();
	}

	UIElement::Type UIKeyConfig::get_type() const
	{
		return TYPE;
	}

	void UIKeyConfig::safe_close()
	{
		if (dirty)
		{
			constexpr char* message = "Do you want to save your changes?";

			auto onok = [&](bool ok)
			{
				if (ok)
				{
					save_staged_mappings();
					close();
				}
				else
				{
					close();
				}
			};

			UI::get().emplace<UIOk>(message, onok);
		}
		else
		{
			close();
		}
	}

	/// UI: Tooltip
	void UIKeyConfig::show_item(int32_t item_id)
	{
		UI::get().show_item(Tooltip::Parent::KEYCONFIG, item_id);
	}

	void UIKeyConfig::show_skill(int32_t skill_id)
	{
		int32_t level = skillbook.get_level(skill_id);
		int32_t masterlevel = skillbook.get_masterlevel(skill_id);
		int64_t expiration = skillbook.get_expiration(skill_id);

		UI::get().show_skill(Tooltip::Parent::KEYCONFIG, skill_id, level, masterlevel, expiration);
	}

	void UIKeyConfig::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::KEYCONFIG);
	}

	/// Keymap Staging
	void UIKeyConfig::stage_mapping(Point<int16_t> cursorposition, Keyboard::Mapping mapping)
	{
		KeyConfig::Key key = key_by_position(cursorposition);
		Keyboard::Mapping prior_staged = staged_mappings[key];

		if (prior_staged == mapping)
			return;

		unstage_mapping(prior_staged);

		if (is_action_mapping(mapping))
		{
			KeyAction::Id action = KeyAction::actionbyid(mapping.action);
			auto iter = std::find(bound_actions.begin(), bound_actions.end(), action);

			if (iter == bound_actions.end())
				bound_actions.emplace_back(action);
		}

		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping staged_mapping = it.second;

			if (staged_mapping == mapping)
			{
				if (it.first == KeyConfig::Key::LEFT_CONTROL || it.first == KeyConfig::Key::RIGHT_CONTROL)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_CONTROL);
					staged_mappings.erase(KeyConfig::Key::RIGHT_CONTROL);
				}
				else if (it.first == KeyConfig::Key::LEFT_ALT || it.first == KeyConfig::Key::RIGHT_ALT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_ALT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_ALT);
				}
				else if (it.first == KeyConfig::Key::LEFT_SHIFT || it.first == KeyConfig::Key::RIGHT_SHIFT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_SHIFT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_SHIFT);
				}
				else
				{
					staged_mappings.erase(it.first);
				}

				break;
			}
		}

		if (key == KeyConfig::Key::LEFT_CONTROL || key == KeyConfig::Key::RIGHT_CONTROL)
		{
			staged_mappings[KeyConfig::Key::LEFT_CONTROL] = mapping;
			staged_mappings[KeyConfig::Key::RIGHT_CONTROL] = mapping;
		}
		else if (key == KeyConfig::Key::LEFT_ALT || key == KeyConfig::Key::RIGHT_ALT)
		{
			staged_mappings[KeyConfig::Key::LEFT_ALT] = mapping;
			staged_mappings[KeyConfig::Key::RIGHT_ALT] = mapping;
		}
		else if (key == KeyConfig::Key::LEFT_SHIFT || key == KeyConfig::Key::RIGHT_SHIFT)
		{
			staged_mappings[KeyConfig::Key::LEFT_SHIFT] = mapping;
			staged_mappings[KeyConfig::Key::RIGHT_SHIFT] = mapping;
		}
		else
		{
			staged_mappings[key] = mapping;
		}

		if (mapping.type == KeyType::Id::ITEM)
		{
			int32_t item_id = mapping.action;

			if (item_icons.find(item_id) == item_icons.end())
			{
				int16_t count = inventory.get_total_item_count(item_id);
				Texture tx = get_item_texture(item_id);
				item_icons[item_id] = std::make_unique<Icon>(std::make_unique<CountableMappingIcon>(mapping, count), tx, count);
			}
		}
		else if (mapping.type == KeyType::Id::SKILL)
		{
			int32_t skill_id = mapping.action;

			if (skill_icons.find(skill_id) == skill_icons.end())
			{
				Texture tx = get_skill_texture(skill_id);
				skill_icons[skill_id] = std::make_unique<Icon>(std::make_unique<MappingIcon>(mapping), tx, -1);
			}
		}

		dirty = true;
	}

	void UIKeyConfig::unstage_mapping(Keyboard::Mapping mapping)
	{
		if (is_action_mapping(mapping))
		{
			KeyAction::Id action = KeyAction::actionbyid(mapping.action);
			auto iter = std::find(bound_actions.begin(), bound_actions.end(), action);

			if (iter != bound_actions.end())
				bound_actions.erase(iter);
		}

		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping staged_mapping = it.second;

			if (staged_mapping == mapping)
			{
				if (it.first == KeyConfig::Key::LEFT_CONTROL || it.first == KeyConfig::Key::RIGHT_CONTROL)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_CONTROL);
					staged_mappings.erase(KeyConfig::Key::RIGHT_CONTROL);
				}
				else if (it.first == KeyConfig::Key::LEFT_ALT || it.first == KeyConfig::Key::RIGHT_ALT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_ALT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_ALT);
				}
				else if (it.first == KeyConfig::Key::LEFT_SHIFT || it.first == KeyConfig::Key::RIGHT_SHIFT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_SHIFT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_SHIFT);
				}
				else
				{
					staged_mappings.erase(it.first);
				}

				if (staged_mapping.type == KeyType::Id::ITEM)
				{
					int32_t item_id = staged_mapping.action;
					item_icons.erase(item_id);
				}
				else if (staged_mapping.type == KeyType::Id::SKILL)
				{
					int32_t skill_id = staged_mapping.action;
					skill_icons.erase(skill_id);
				}

				dirty = true;

				break;
			}
		}
	}

	void UIKeyConfig::save_staged_mappings()
	{
		std::vector<std::tuple<KeyConfig::Key, KeyType::Id, int32_t>> updated_actions;

		for (auto key : staged_mappings)
		{
			KeyConfig::Key k = KeyConfig::actionbyid(key.first);
			Keyboard::Mapping mapping = key.second;
			Keyboard::Mapping saved_mapping = keyboard->get_maple_mapping(key.first);

			if (mapping != saved_mapping)
				updated_actions.emplace_back(std::make_tuple(k, mapping.type, mapping.action));
		}

		auto maplekeys = keyboard->get_maplekeys();

		for (auto key : maplekeys)
		{
			bool keyFound = false;
			KeyConfig::Key keyConfig = KeyConfig::actionbyid(key.first);

			for (auto tkey : staged_mappings)
			{
				KeyConfig::Key tKeyConfig = KeyConfig::actionbyid(tkey.first);

				if (keyConfig == tKeyConfig)
				{
					keyFound = true;
					break;
				}
			}

			if (!keyFound)
				updated_actions.emplace_back(std::make_tuple(keyConfig, KeyType::Id::NONE, KeyAction::Id::LENGTH));
		}

		if (updated_actions.size() > 0)
			ChangeKeyMapPacket(updated_actions).dispatch();

		for (auto action : updated_actions)
		{
			KeyConfig::Key key = std::get<0>(action);
			KeyType::Id type = std::get<1>(action);
			int32_t keyAction = std::get<2>(action);

			if (type == KeyType::Id::NONE)
				keyboard->remove(key);
			else
				keyboard->assign(key, type, keyAction);
		}

		dirty = false;
	}

	void UIKeyConfig::bind_staged_action_keys()
	{
		for (auto fkey : keys_pos)
		{
			Keyboard::Mapping mapping = get_staged_mapping(fkey.first);

			if (mapping.type != KeyType::Id::NONE)
			{
				KeyAction::Id action = KeyAction::actionbyid(mapping.action);

				if (action)
					bound_actions.emplace_back(action);
			}
		}
	}

	void UIKeyConfig::clear()
	{
		item_icons.clear();
		skill_icons.clear();
		bound_actions.clear();
		staged_mappings = {};
		dirty = true;
	}

	void UIKeyConfig::reset()
	{
		clear();
		staged_mappings = keyboard->get_maplekeys();
		load_item_icons();
		load_skill_icons();
		load_macro_icons();
		bind_staged_action_keys();
		dirty = false;
	}

	/// Helpers
	Texture UIKeyConfig::get_item_texture(int32_t item_id) const
	{
		const ItemData& data = ItemData::get(item_id);
		return data.get_icon(false);
	}

	Texture UIKeyConfig::get_skill_texture(int32_t skill_id) const
	{
		const SkillData& data = SkillData::get(skill_id);
		return data.get_icon(SkillData::Icon::NORMAL);
	}

	KeyConfig::Key UIKeyConfig::key_by_position(Point<int16_t> cursorpos) const
	{
		Point<int16_t> relative = cursorpos - position;
		KeyConfig::Key closest = KeyConfig::Key::LENGTH;
		int32_t best_dist = 25 * 25;

		for (auto iter : keys_pos)
		{
			if (iter.second.x() == 0 && iter.second.y() == 0)
				continue;

			Point<int16_t> center = iter.second + Point<int16_t>(16, 16);
			int32_t dx = relative.x() - center.x();
			int32_t dy = relative.y() - center.y();
			int32_t dist = dx * dx + dy * dy;

			if (dist < best_dist)
			{
				best_dist = dist;
				closest = iter.first;
			}
		}

		return closest;
	}

	KeyAction::Id UIKeyConfig::unbound_action_by_position(Point<int16_t> cursorpos) const
	{
		for (auto iter : unbound_actions_pos)
		{
			if (std::find(bound_actions.begin(), bound_actions.end(), iter.first) != bound_actions.end())
				continue;

			Rectangle<int16_t> icon_rect = Rectangle<int16_t>(
				position + iter.second,
				position + iter.second + Point<int16_t>(36, 36)
				);

			if (icon_rect.contains(cursorpos))
				return iter.first;
		}

		return KeyAction::Id::LENGTH;
	}

	Keyboard::Mapping UIKeyConfig::get_staged_mapping(int32_t keycode) const
	{
		auto iter = staged_mappings.find(keycode);

		if (iter == staged_mappings.end())
			return {};

		return iter->second;
	}

	bool UIKeyConfig::is_action_mapping(Keyboard::Mapping mapping) const
	{
		return std::find(action_mappings.begin(), action_mappings.end(), mapping) != action_mappings.end();
	}

	KeyType::Id UIKeyConfig::get_keytype(KeyAction::Id action)
	{
		switch (action)
		{
		case KeyAction::Id::EQUIPMENT:
		case KeyAction::Id::ITEMS:
		case KeyAction::Id::STATS:
		case KeyAction::Id::SKILLS:
		case KeyAction::Id::FRIENDS:
		case KeyAction::Id::WORLDMAP:
		case KeyAction::Id::MAPLECHAT:
		case KeyAction::Id::MINIMAP:
		case KeyAction::Id::QUESTLOG:
		case KeyAction::Id::KEYBINDINGS:
		case KeyAction::Id::TOGGLECHAT:
		case KeyAction::Id::WHISPER:
		case KeyAction::Id::SAY:
		case KeyAction::Id::PARTYCHAT:
		case KeyAction::Id::MENU:
		case KeyAction::Id::QUICKSLOTS:
		case KeyAction::Id::GUILD:
		case KeyAction::Id::FRIENDSCHAT:
		case KeyAction::Id::PARTY:
		case KeyAction::Id::NOTIFIER:
		case KeyAction::Id::CASHSHOP:
		case KeyAction::Id::GUILDCHAT:
		case KeyAction::Id::MEDALS:
		case KeyAction::Id::BITS:
		case KeyAction::Id::ALLIANCECHAT:
		case KeyAction::Id::MAPLENEWS:
		case KeyAction::Id::MANAGELEGION:
		case KeyAction::Id::PROFESSION:
		case KeyAction::Id::BOSSPARTY:
		case KeyAction::Id::ITEMPOT:
		case KeyAction::Id::EVENT:
		case KeyAction::Id::SILENTCRUSADE:
		case KeyAction::Id::BATTLEANALYSIS:
		case KeyAction::Id::GUIDE:
		case KeyAction::Id::ENHANCEEQUIP:
		case KeyAction::Id::MONSTERCOLLECTION:
		case KeyAction::Id::SOULWEAPON:
		case KeyAction::Id::CHARINFO:
		case KeyAction::Id::CHANGECHANNEL:
		case KeyAction::Id::MAINMENU:
		case KeyAction::Id::SCREENSHOT:
		case KeyAction::Id::MONSTERBOOK:
		case KeyAction::Id::FAMILY:
			return KeyType::Id::MENU;
		case KeyAction::Id::PICKUP:
		case KeyAction::Id::SIT:
		case KeyAction::Id::ATTACK:
		case KeyAction::Id::JUMP:
		case KeyAction::Id::INTERACT_HARVEST:
			return KeyType::Id::ACTION;
		case KeyAction::Id::FACE1:
		case KeyAction::Id::FACE2:
		case KeyAction::Id::FACE3:
		case KeyAction::Id::FACE4:
		case KeyAction::Id::FACE5:
		case KeyAction::Id::FACE6:
		case KeyAction::Id::FACE7:
			return KeyType::Id::FACE;
		case KeyAction::Id::LEFT:
		case KeyAction::Id::RIGHT:
		case KeyAction::Id::UP:
		case KeyAction::Id::DOWN:
		case KeyAction::Id::BACK:
		case KeyAction::Id::TAB:
		case KeyAction::Id::RETURN:
		case KeyAction::Id::ESCAPE:
		case KeyAction::Id::SPACE:
		case KeyAction::Id::DELETE:
		case KeyAction::Id::HOME:
		case KeyAction::Id::END:
		case KeyAction::Id::COPY:
		case KeyAction::Id::PASTE:
		case KeyAction::Id::LENGTH:
		default:
			return KeyType::Id::NONE;
		}
	}

	/// Item count
	void UIKeyConfig::update_item_count(InventoryType::Id type, int16_t slot, int16_t change)
	{
		int32_t item_id = inventory.get_item_id(type, slot);

		if (item_icons.find(item_id) == item_icons.end())
			return;

		int16_t item_count = item_icons[item_id]->get_count();
		item_icons[item_id]->set_count(item_count + change);
	}

	/// MappingIcon
	UIKeyConfig::MappingIcon::MappingIcon(Keyboard::Mapping m) : mapping(m) {}

	UIKeyConfig::MappingIcon::MappingIcon(KeyAction::Id action)
	{
		KeyType::Id type = UIKeyConfig::get_keytype(action);
		mapping = Keyboard::Mapping(type, action);
	}

	void UIKeyConfig::MappingIcon::drop_on_stage() const
	{
		if (mapping.type == KeyType::Id::ITEM || mapping.type == KeyType::Id::SKILL)
		{
			auto keyconfig = UI::get().get_element<UIKeyConfig>();
			keyconfig->unstage_mapping(mapping);
		}
	}

	void UIKeyConfig::MappingIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
	{
		auto keyconfig = UI::get().get_element<UIKeyConfig>();

		if (remove)
			keyconfig->unstage_mapping(mapping);
		else
			keyconfig->stage_mapping(cursorposition, mapping);
	}

	Icon::IconType UIKeyConfig::MappingIcon::get_type()
	{
		return Icon::IconType::KEY;
	}

	UIKeyConfig::CountableMappingIcon::CountableMappingIcon(Keyboard::Mapping m, int16_t c) : UIKeyConfig::MappingIcon(m), count(c) {}

	void UIKeyConfig::CountableMappingIcon::set_count(int16_t c)
	{
		count = c;
	}
}