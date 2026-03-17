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
#include "UICharInfo.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/PlayerInteractionPackets.h"
#include "../../Net/Packets/TradePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UICharInfo::UICharInfo(int32_t cid) : UIDragElement<PosCHARINFO>(), is_loading(true), timestep(Constants::TIMESTEP), personality_enabled(false), collect_enabled(false), damage_enabled(false), item_enabled(false)
	{
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node UserInfo = nl::nx::ui["UIWindow2.img"]["UserInfo"];
		nl::node character = UserInfo["character"];
		nl::node backgrnd = character["backgrnd"];

		// v83: button bitmaps are in UIWindow.img/UserInfo, not UIWindow2.img
		nl::node UserInfoBtns = nl::nx::ui["UIWindow.img"]["UserInfo"];

		/// Main Window
		sprites.emplace_back(backgrnd);
		sprites.emplace_back(character["backgrnd2"]);
		sprites.emplace_back(character["name"]);

		Point<int16_t> backgrnd_dim = Texture(backgrnd).get_dimensions();
		Point<int16_t> close_dimensions = Point<int16_t>(backgrnd_dim.x() - 21, 6);

		buttons[Buttons::BtClose] = std::make_unique<MapleButton>(close, close_dimensions);

		// Only create buttons if their NX nodes exist
		auto add_button = [&](uint16_t id, nl::node src) {
			if (src.size() > 0)
				buttons[id] = std::make_unique<MapleButton>(src);
		};

		// v83 button names differ from post-BB names
		add_button(Buttons::BtCollect, UserInfoBtns["BtCollectionShow"]);
		add_button(Buttons::BtFamily, UserInfoBtns["BtFamily"]);
		add_button(Buttons::BtItem, UserInfoBtns["BtItem"]);
		add_button(Buttons::BtParty, UserInfoBtns["BtParty"]);
		add_button(Buttons::BtPet, UserInfoBtns["BtPetShow"]);
		add_button(Buttons::BtRide, UserInfoBtns["BtTamingShow"]);
		add_button(Buttons::BtTrad, UserInfoBtns["BtTrade"]);
		add_button(Buttons::BtWish, UserInfoBtns["BtWish"]);

		name = Text(Text::Font::A12M, Text::Alignment::CENTER, Color::Name::WHITE);
		job = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		level = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		fame = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		guild = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		alliance = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);

		// Pet and mount buttons are disabled until mount/pet state is known
		if (buttons.count(Buttons::BtPet))
			buttons[Buttons::BtPet]->set_state(Button::State::DISABLED);
		if (buttons.count(Buttons::BtRide))
			buttons[Buttons::BtRide]->set_state(Button::State::DISABLED);
		if (buttons.count(Buttons::BtWish))
			buttons[Buttons::BtWish]->set_state(Button::State::DISABLED);

		/// Farm (post-BB, may not exist in v83)
		nl::node farm = UserInfo["farm"];
		nl::node farm_backgrnd = farm["backgrnd"];

		if (farm_backgrnd.size() > 0)
		{
			loading = farm["loading"];

			farm_dim = Texture(farm_backgrnd).get_dimensions();
			farm_adj = Point<int16_t>(-farm_dim.x(), 0);

			sprites.emplace_back(farm_backgrnd, farm_adj);
			sprites.emplace_back(farm["backgrnd2"], farm_adj);
			sprites.emplace_back(farm["default"], farm_adj);
			sprites.emplace_back(farm["cover"], farm_adj);

			add_button(Buttons::BtFriend, farm["btFriend"]);
			add_button(Buttons::BtVisit, farm["btVisit"]);

			// Apply farm offset to buttons if created
			// Note: farm buttons position from NX relative to farm panel
		}
		else
		{
			farm_dim = Point<int16_t>(0, 0);
			farm_adj = Point<int16_t>(0, 0);
		}

		farm_name = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::SUPERNOVA);
		farm_level = Charset(farm["number"], Charset::Alignment::LEFT);

#pragma region BottomWindow
		bottom_window_adj = Point<int16_t>(0, backgrnd_dim.y() + 1);

		/// Personality (post-BB, may not exist in v83)
		nl::node personality = UserInfo["personality"];
		nl::node personality_backgrnd = personality["backgrnd"];

		if (personality_backgrnd.size() > 0)
		{
			personality_sprites.emplace_back(personality_backgrnd, bottom_window_adj);
			personality_sprites.emplace_back(personality["backgrnd2"], bottom_window_adj);

			personality_sprites_enabled[true].emplace_back(personality["backgrnd3"], bottom_window_adj);
			personality_sprites_enabled[true].emplace_back(personality["backgrnd4"], bottom_window_adj);
			personality_sprites_enabled[true].emplace_back(personality["center"], bottom_window_adj);
			personality_sprites_enabled[false].emplace_back(personality["before30level"], bottom_window_adj);
		}

		personality_dimensions = Texture(personality_backgrnd).get_dimensions();

		/// Collect (post-BB, may not exist in v83)
		nl::node collect = UserInfo["collect"];
		nl::node collect_backgrnd = collect["backgrnd"];

		if (collect_backgrnd.size() > 0)
		{
			collect_sprites.emplace_back(collect_backgrnd, bottom_window_adj);
			collect_sprites.emplace_back(collect["backgrnd2"], bottom_window_adj);

			default_medal = collect["icon1"];

			add_button(Buttons::BtArrayGet, collect["BtArrayGet"]);
			add_button(Buttons::BtArrayName, collect["BtArrayName"]);
		}

		medal_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "Junior Adventurer");
		medal_total = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "2");

		collect_dimensions = Texture(collect_backgrnd).get_dimensions();

		/// Damage (post-BB, may not exist in v83)
		nl::node damage = UserInfo["damage"];
		nl::node damage_backgrnd = damage["backgrnd"];

		if (damage_backgrnd.size() > 0)
		{
			damage_sprites.emplace_back(damage_backgrnd, bottom_window_adj);
			damage_sprites.emplace_back(damage["backgrnd2"], bottom_window_adj);
			damage_sprites.emplace_back(damage["backgrnd3"], bottom_window_adj);

			add_button(Buttons::BtFAQ, damage["BtFAQ"]);
			add_button(Buttons::BtRegist, damage["BtRegist"]);
		}

		damage_dimensions = Texture(damage_backgrnd).get_dimensions();
#pragma endregion

#pragma region RightWindow
		right_window_adj = Point<int16_t>(backgrnd_dim.x(), 0);

		/// Item (post-BB, may not exist in v83)
		nl::node item = UserInfo["item"];
		nl::node item_backgrnd = item["backgrnd"];

		if (item_backgrnd.size() > 0)
		{
			item_sprites.emplace_back(item_backgrnd, right_window_adj);
			item_sprites.emplace_back(item["backgrnd2"], right_window_adj);
		}

		item_dimensions = Texture(item_backgrnd).get_dimensions();
#pragma endregion


		dimension = backgrnd_dim;
		dragarea = Point<int16_t>(dimension.x(), 20);

		target_character = Stage::get().get_character(cid).get();

		CharInfoRequestPacket(cid).dispatch();
	}

	void UICharInfo::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		for (size_t i = 0; i < Buttons::BtArrayGet; i++)
		{
			auto it = buttons.find(static_cast<uint16_t>(i));
			if (it != buttons.end() && it->second)
				it->second->draw(position);
		}

		/// Main Window
		int16_t row_height = 18;
		Point<int16_t> text_pos = position + Point<int16_t>(153, 65);

		if (target_character)
			target_character->draw_preview(position + Point<int16_t>(63, 129), inter);

		name.draw(position + Point<int16_t>(59, 131));
		level.draw(text_pos + Point<int16_t>(0, row_height * 0));
		job.draw(text_pos + Point<int16_t>(0, row_height * 1));
		fame.draw(text_pos + Point<int16_t>(0, row_height * 2));
		guild.draw(text_pos + Point<int16_t>(0, row_height * 3) + Point<int16_t>(0, 1));
		alliance.draw(text_pos + Point<int16_t>(0, row_height * 4));

		/// Farm
		Point<int16_t> farm_pos = position + farm_adj;

		if (is_loading)
			loading.draw(farm_pos, inter);

		farm_name.draw(farm_pos + Point<int16_t>(136, 51));
		farm_level.draw(farm_level_text, farm_pos + Point<int16_t>(126, 34));

		/// Personality
		if (personality_enabled)
		{
			for (Sprite sprite : personality_sprites)
				sprite.draw(position, inter);

			bool show_personality = (target_character && target_character->get_level() >= 30);

			for (Sprite sprite : personality_sprites_enabled[show_personality])
				sprite.draw(position, inter);
		}

		/// Collect
		if (collect_enabled)
		{
			for (Sprite sprite : collect_sprites)
				sprite.draw(position, inter);

			for (size_t i = 0; i < 15; i++)
			{
				div_t div = std::div(i, 5);
				default_medal.draw(position + bottom_window_adj + Point<int16_t>(61, 66) + Point<int16_t>(38 * div.rem, 38 * div.quot), inter);
			}

			for (size_t i = Buttons::BtArrayGet; i < Buttons::BtFAQ; i++)
			{
				auto it = buttons.find(static_cast<uint16_t>(i));
				if (it != buttons.end() && it->second)
					it->second->draw(position);
			}

			Point<int16_t> text_pos = Point<int16_t>(121, 8);

			medal_text.draw(position + bottom_window_adj + text_pos);
			medal_total.draw(position + bottom_window_adj + text_pos + Point<int16_t>(0, 19));
		}

		/// Damage
		if (damage_enabled)
		{
			for (Sprite sprite : damage_sprites)
				sprite.draw(position, inter);

			for (size_t i = Buttons::BtFAQ; i <= Buttons::BtRegist; i++)
			{
				auto it = buttons.find(static_cast<uint16_t>(i));
				if (it != buttons.end() && it->second)
					it->second->draw(position);
			}
		}

		/// Item
		if (item_enabled)
			for (Sprite sprite : item_sprites)
				sprite.draw(position, inter);
	}

	void UICharInfo::update()
	{
		if (timestep >= Constants::TIMESTEP * UCHAR_MAX)
		{
			is_loading = false;
		}
		else
		{
			loading.update();
			timestep += Constants::TIMESTEP;
		}
	}

	Button::State UICharInfo::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BtClose:
			deactivate();
			return Button::State::NORMAL;
		case Buttons::BtFamily:
		case Buttons::BtParty:
			break;
		case Buttons::BtItem:
			show_right_window(buttonid);
			return Button::State::NORMAL;
		case Buttons::BtCollect:
		case Buttons::BtRide:
		case Buttons::BtPet:
			show_bottom_window(buttonid);
			return Button::State::NORMAL;
		case Buttons::BtTrad:
			// Request trade with this player
			if (target_character)
			{
				TradeCreatePacket().dispatch();
				TradeInvitePacket(target_character->get_oid()).dispatch();
			}
			deactivate();
			return Button::State::NORMAL;
		case Buttons::BtWish:
		case Buttons::BtFriend:
		case Buttons::BtVisit:
		default:
			break;
		}

		return Button::State::DISABLED;
	}

	bool UICharInfo::is_in_range(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(position, position + dimension);

		Rectangle<int16_t> farm_bounds = Rectangle<int16_t>(position, position + farm_dim);
		farm_bounds.shift(farm_adj);

		Rectangle<int16_t> bottom_bounds = Rectangle<int16_t>(Point<int16_t>(0, 0), Point<int16_t>(0, 0));
		Rectangle<int16_t> right_bounds = Rectangle<int16_t>(Point<int16_t>(0, 0), Point<int16_t>(0, 0));

		int16_t cur_x = cursorpos.x();
		int16_t cur_y = cursorpos.y();

		if (personality_enabled)
		{
			bottom_bounds = Rectangle<int16_t>(position, position + personality_dimensions);
			bottom_bounds.shift(bottom_window_adj);
		}

		if (collect_enabled)
		{
			bottom_bounds = Rectangle<int16_t>(position, position + collect_dimensions);
			bottom_bounds.shift(bottom_window_adj);
		}

		if (damage_enabled)
		{
			bottom_bounds = Rectangle<int16_t>(position, position + damage_dimensions);
			bottom_bounds.shift(bottom_window_adj);
		}

		if (item_enabled)
		{
			right_bounds = Rectangle<int16_t>(position, position + item_dimensions);
			right_bounds.shift(right_window_adj);
		}

		return bounds.contains(cursorpos) || farm_bounds.contains(cursorpos) || bottom_bounds.contains(cursorpos) || right_bounds.contains(cursorpos);
	}

	void UICharInfo::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UICharInfo::get_type() const
	{
		return TYPE;
	}

	void UICharInfo::update_stats(int32_t character_id, int16_t job_id, int8_t lv, int16_t f, std::string g, std::string a)
	{
		int32_t player_id = Stage::get().get_player().get_oid();

		auto disable_button = [&](uint16_t id) {
			if (buttons.count(id) && buttons[id])
				buttons[id]->set_state(Button::State::DISABLED);
		};

		if (character_id == player_id)
		{
			disable_button(Buttons::BtParty);
			disable_button(Buttons::BtFriend);
		}

		Job character_job = Job(job_id);

		if (target_character)
			name.change_text(target_character->get_name());

		job.change_text(character_job.get_name());
		level.change_text(std::to_string(lv));
		fame.change_text(std::to_string(f));
		guild.change_text((g == "" ? "-" : g));
		alliance.change_text(a);

		// Enable pet button if the character has an active pet
		if (target_character && target_character->has_pet())
			buttons[Buttons::BtPet]->set_state(Button::State::NORMAL);
		else
			buttons[Buttons::BtPet]->set_state(Button::State::DISABLED);

		// Enable mount button if the character has a taming mob equipped
		if (target_character && target_character->has_mount())
			buttons[Buttons::BtRide]->set_state(Button::State::NORMAL);
		else
			buttons[Buttons::BtRide]->set_state(Button::State::DISABLED);

		farm_name.change_text("");
		farm_level_text = "1";
	}

	void UICharInfo::show_bottom_window(uint16_t buttonid)
	{
		personality_enabled = false;
		collect_enabled = false;
		damage_enabled = false;

		switch (buttonid)
		{
		case Buttons::BtCollect:
			collect_enabled = true;
			break;
		}
	}

	void UICharInfo::show_right_window(uint16_t buttonid)
	{
		item_enabled = false;

		switch (buttonid)
		{
		case Buttons::BtItem:
			item_enabled = true;
			break;
		}
	}
}