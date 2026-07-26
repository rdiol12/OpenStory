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
#include "UIExplorerCreation.h"

#include "../../Graphics/Geometry.h"

#include <cmath>

#include "UICharSelect.h"
#include "UILoginNotice.h"
#include "UIRaceSelect.h"

#include "../UI.h"
#include "../UIScale.h"

#include "../Components/MapleButton.h"

#include "../../Configuration.h"
#include "../../Constants.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"

#include "../../Net/Packets/CharCreationPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIExplorerCreation::UIExplorerCreation() : UICreationBase(1)
	{
		gender = false;
		charSet = false;
		named = false;

		ui_scale = std::min(UIScale::scale_x(), UIScale::scale_y());
		ui_scale_x = UIScale::scale_x();
		ui_scale_y = UIScale::scale_y();

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		nl::node Login = nl::nx::ui["Login.img"];
		nl::node Common = Login["Common"];
		nl::node CustomizeChar = Login["CustomizeChar"]["000"];
		nl::node back = nl::nx::map["Back"]["login.img"]["back"];
		nl::node signboard = nl::nx::mapLatest["Obj"]["login.img"]["NewChar"]["signboard"];
		nl::node board = CustomizeChar["board"];
		nl::node genderSelect = CustomizeChar["genderSelect"];
		nl::node frame = nl::nx::mapLatest["Obj"]["login.img"]["Common"]["frame"]["2"]["0"];

		// Reference v83 composition (HeavenClient): sky strip back/2 tiled,
		// cloud back/27 drifting, scene pieces 15-18 and the Common frame.
		sky = Texture(back["2"]);
		cloud = back["27"];

		auto scenepc = [&](nl::node src, int16_t x, int16_t y)
		{
			if (src)
				scenery.emplace_back(Texture(src), Point<int16_t>(x, y));
		};
		scenepc(back["15"], 153, 685);
		scenepc(back["16"], 200, 400);
		scenepc(back["17"], 160, 263);
		scenepc(back["18"], 349, 1220);
		scenepc(Common["frame"], 400, 300);

		genderboard.emplace_back(Texture(board["genderTop"]), Point<int16_t>(452, 107));

		for (int16_t f = 0; f < 5; f++)
			genderboard.emplace_back(Texture(board["boardMid"]), Point<int16_t>(452, static_cast<int16_t>(221 + (24 * f))));

		genderboard.emplace_back(Texture(board["boardBottom"]), Point<int16_t>(452, 341));
		sprites_lookboard.emplace_back(CustomizeChar["charSet"], sign_args(Texture(CustomizeChar["charSet"]), 452, 112));

		for (size_t i = 0; i <= 5; i++)
		{
			size_t f = i;

			if (i >= 2)
				f++;

			sprites_lookboard.emplace_back(CustomizeChar["avatarSel"][i]["normal"], DrawArgument(slay(463, 214 + (f * 18)), ui_scale, ui_scale));
		}

		// stat table (STR/DEX/INT/LUK frame) and the randomize dice, inside the
		// CHARACTER SETTINGS board.
		sprites_lookboard.emplace_back(CustomizeChar["dice"]["2"], DrawArgument(slay(656, 137), ui_scale, ui_scale));

		buttons[Buttons::BT_CHARC_GENDER_M] = std::make_unique<MapleButton>(genderSelect["male"], lay(453, 121));
		buttons[Buttons::BT_CHARC_GEMDER_F] = std::make_unique<MapleButton>(genderSelect["female"], lay(451, 121));
		buttons[Buttons::BT_CHARC_FACEL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], slay(518, 215 + (0 * 18)));
		buttons[Buttons::BT_CHARC_FACER] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], slay(650, 215 + (0 * 18)));
		buttons[Buttons::BT_CHARC_HAIRL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], slay(518, 215 + (1 * 18)));
		buttons[Buttons::BT_CHARC_HAIRR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], slay(650, 215 + (1 * 18)));
		buttons[Buttons::BT_CHARC_SKINL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], slay(518, 215 + (3 * 18)));
		buttons[Buttons::BT_CHARC_SKINR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], slay(650, 215 + (3 * 18)));
		buttons[Buttons::BT_CHARC_TOPL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], slay(518, 215 + (4 * 18)));
		buttons[Buttons::BT_CHARC_TOPR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], slay(650, 215 + (4 * 18)));
		buttons[Buttons::BT_CHARC_SHOESL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], slay(518, 215 + (5 * 18)));
		buttons[Buttons::BT_CHARC_SHOESR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], slay(650, 215 + (5 * 18)));
		buttons[Buttons::BT_CHARC_WEPL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], slay(518, 215 + (6 * 18)));
		buttons[Buttons::BT_CHARC_WEPR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], slay(650, 215 + (6 * 18)));

		for (size_t i = 0; i <= 7; i++)
		{
			buttons[Buttons::BT_CHARC_HAIRC0 + i] = std::make_unique<MapleButton>(CustomizeChar["hairSelect"][i], slay(515 + (i * 15), 251));
			buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(false);
		}

		buttons[Buttons::BT_CHARC_FACEL]->set_active(false);
		buttons[Buttons::BT_CHARC_FACER]->set_active(false);
		buttons[Buttons::BT_CHARC_HAIRL]->set_active(false);
		buttons[Buttons::BT_CHARC_HAIRR]->set_active(false);
		buttons[Buttons::BT_CHARC_SKINL]->set_active(false);
		buttons[Buttons::BT_CHARC_SKINR]->set_active(false);
		buttons[Buttons::BT_CHARC_TOPL]->set_active(false);
		buttons[Buttons::BT_CHARC_TOPR]->set_active(false);
		buttons[Buttons::BT_CHARC_SHOESL]->set_active(false);
		buttons[Buttons::BT_CHARC_SHOESR]->set_active(false);
		buttons[Buttons::BT_CHARC_WEPL]->set_active(false);
		buttons[Buttons::BT_CHARC_WEPR]->set_active(false);

		buttons[Buttons::BT_CHARC_OK] = std::make_unique<MapleButton>(CustomizeChar["BtYes"], slay(480, 406));
		buttons[Buttons::BT_CHARC_CANCEL] = std::make_unique<MapleButton>(CustomizeChar["BtNo"], slay(556, 406));

		nameboard = CustomizeChar["charName"];
		namechar = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE, Rectangle<int16_t>(slay(488, 207), slay(596, 265)), 12);

		buttons[Buttons::BT_BACK] = std::make_unique<MapleButton>(Login["Common"]["BtStart"], lay(0, 515));

		// Uniform-scale every button (draw + hit bounds).
		for (auto& btit : buttons)
			btit.second->set_scale(ui_scale);

		// Cards span +20..+182 from a shared anchor (origins bake the offsets);
		// board is 201 wide, so anchoring at board_x - 0.5 centers the pair.
		Point<int16_t> gender_pos = slay(451, 121);
		buttons[Buttons::BT_CHARC_GENDER_M]->set_position(gender_pos);
		buttons[Buttons::BT_CHARC_GEMDER_F]->set_position(gender_pos);

		namechar.set_state(Textfield::DISABLED);

		namechar.set_enter_callback(
			[&](std::string)
			{
				button_pressed(Buttons::BT_CHARC_OK);
			}
		);

		namechar.set_key_callback(
			KeyAction::Id::ESCAPE,
			[&]()
			{
				button_pressed(Buttons::BT_CHARC_CANCEL);
			}
		);

		facename = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		hairname = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		bodyname = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		topname = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		shoename = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		wepname = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);

		nl::node mkinfo = nl::nx::etc["MakeCharInfo.img"]["Info"];

		for (size_t i = 0; i < 2; i++)
		{
			bool f;
			nl::node CharGender;

			if (i == 0)
			{
				f = true;
				CharGender = mkinfo["CharFemale"];
			}
			else
			{
				f = false;
				CharGender = mkinfo["CharMale"];
			}

			for (auto node : CharGender)
			{
				int num = stoi(node.name());

				for (auto idnode : node)
				{
					int32_t value = idnode;

					switch (num)
					{
						case 0:
							faces[f].push_back(value);
							break;
						case 1:
							hairs[f].push_back(value);
							break;
						case 2:
							haircolors[f].push_back(static_cast<uint8_t>(value));
							break;
						case 3:
							skins[f].push_back(static_cast<uint8_t>(value));
							break;
						case 4:
							tops[f].push_back(value);
							break;
						case 5:
							bots[f].push_back(value);
							break;
						case 6:
							shoes[f].push_back(value);
							break;
						case 7:
							weapons[f].push_back(value);
							break;
					}
				}
			}
		}

		female = false;
		randomize_look();

		newchar.set_direction(true);

		cloudfx = 200.0f;
	}

	Point<int16_t> UIExplorerCreation::lay(int16_t x, int16_t y) const
	{
		return Point<int16_t>(
			static_cast<int16_t>(std::lround(x * ui_scale_x)),
			static_cast<int16_t>(std::lround(y * ui_scale_y)));
	}

	Point<int16_t> UIExplorerCreation::slay(int16_t x, int16_t y) const
	{
		return UIScale::uniform_point(lay(455, 110), Point<int16_t>(452, 107), x, y, ui_scale);
	}

	DrawArgument UIExplorerCreation::sign_args(const Texture& t, int16_t x, int16_t y) const
	{
		return UIScale::uniform_args(t, lay(455, 110), Point<int16_t>(452, 107), x, y, ui_scale);
	}

	void UIExplorerCreation::draw(float inter) const
	{
		// One gradient across the whole screen — the 20px strip is horizontally
		// uniform, so a single stretched quad covers it seamlessly.
		Point<int16_t> skyo = sky.get_origin();
		sky.draw(DrawArgument(skyo, Point<int16_t>(UIScale::view_width(), UIScale::view_height())));

		int16_t cloudx = static_cast<int16_t>(cloudfx) % 800;
		cloud.draw(UIScale::stretch_args(cloud, static_cast<int16_t>(cloudx - 800), 300));
		cloud.draw(UIScale::stretch_args(cloud, cloudx, 300));
		cloud.draw(UIScale::stretch_args(cloud, static_cast<int16_t>(cloudx + 800), 300));

		// scenic stage (tree / house / greenery / bridge) sits on top of the sky
		for (const auto& pc : scenery)
			pc.first.draw(UIScale::stretch_args(pc.first, pc.second.x(), pc.second.y()));

		UIElement::draw_sprites(inter);

		// feet on the mushroom platform, reference position
		DrawArgument charargs(lay(360, 348), ui_scale_x, ui_scale_y);

		if (!gender)
		{
			for (const auto& pc : genderboard)
				pc.first.draw(sign_args(pc.first, pc.second.x(), pc.second.y()));

			newchar.draw(charargs, inter);

			UIElement::draw_buttons(inter);
		}
		else
		{
			if (!charSet)
			{
				for (auto& sprite : sprites_lookboard)
					sprite.draw(Point<int16_t>(0, 0), inter);

				facename.draw(slay(591, 210 + (0 * 18)));
				hairname.draw(slay(591, 210 + (1 * 18)));
				bodyname.draw(slay(591, 210 + (3 * 18)));
				topname.draw(slay(591, 210 + (4 * 18)));
				shoename.draw(slay(591, 210 + (5 * 18)));
				wepname.draw(slay(591, 210 + (6 * 18)));

				newchar.draw(charargs, inter);

				UIElement::draw_buttons(inter);
			}
			else
			{
				if (!named)
				{
					nameboard.draw(sign_args(nameboard, 452, 107));

					namechar.draw(Point<int16_t>(0, 0));
					newchar.draw(charargs, inter);

					UIElement::draw_buttons(inter);
				}
				else
				{
					nameboard.draw(sign_args(nameboard, 452, 107));

					UIElement::draw_buttons(inter);
				}
			}
		}

		version.draw(lay(707, 4));
	}

	void UIExplorerCreation::update()
	{

		if (!gender)
		{
			newchar.update(Constants::TIMESTEP);
		}
		else
		{
			if (!charSet)
			{
				for (auto& sprite : sprites_lookboard)
					sprite.update();

				newchar.update(Constants::TIMESTEP);
			}
			else
			{
				if (!named)
				{
					namechar.update(get_draw_position());
					newchar.update(Constants::TIMESTEP);
				}
				else
				{
					namechar.set_state(Textfield::State::DISABLED);
				}
			}
		}

		UIElement::update();

		cloudfx += 0.25f;
	}

	Button::State UIExplorerCreation::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case Buttons::BT_CHARC_OK:
			{
				if (!gender)
				{
					gender = true;

					buttons[Buttons::BT_CHARC_GENDER_M]->set_active(false);
					buttons[Buttons::BT_CHARC_GEMDER_F]->set_active(false);

					buttons[Buttons::BT_CHARC_SKINL]->set_active(true);
					buttons[Buttons::BT_CHARC_SKINR]->set_active(true);

					buttons[Buttons::BT_CHARC_FACEL]->set_active(true);
					buttons[Buttons::BT_CHARC_FACER]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRL]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRR]->set_active(true);
					buttons[Buttons::BT_CHARC_TOPL]->set_active(true);
					buttons[Buttons::BT_CHARC_TOPR]->set_active(true);
					buttons[Buttons::BT_CHARC_SHOESL]->set_active(true);
					buttons[Buttons::BT_CHARC_SHOESR]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPL]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPR]->set_active(true);

					for (size_t i = 0; i <= 7; i++)
						buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(true);

					buttons[Buttons::BT_CHARC_OK]->set_position(slay(489, 437));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(slay(563, 437));

					return Button::State::NORMAL;
				}
				else
				{
					if (!charSet)
					{
						charSet = true;

						buttons[Buttons::BT_CHARC_SKINL]->set_active(false);
						buttons[Buttons::BT_CHARC_SKINR]->set_active(false);

						buttons[Buttons::BT_CHARC_FACEL]->set_active(false);
						buttons[Buttons::BT_CHARC_FACER]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRL]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRR]->set_active(false);
						buttons[Buttons::BT_CHARC_TOPL]->set_active(false);
						buttons[Buttons::BT_CHARC_TOPR]->set_active(false);
						buttons[Buttons::BT_CHARC_SHOESL]->set_active(false);
						buttons[Buttons::BT_CHARC_SHOESR]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPL]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPR]->set_active(false);

						for (size_t i = 0; i <= 7; i++)
							buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(false);

						buttons[Buttons::BT_CHARC_OK]->set_position(slay(479, 290));
						buttons[Buttons::BT_CHARC_CANCEL]->set_position(slay(553, 290));

						namechar.set_state(Textfield::State::FOCUSED);

						return Button::State::NORMAL;
					}
					else
					{
						return naming_ok_pressed();
					}
				}
			}
			case BT_BACK:
			{
				Sound(Sound::Name::SCROLLUP).play();

				UI::get().remove(UIElement::Type::CLASSCREATION);
				UI::get().emplace<UIRaceSelect>();

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_CANCEL:
			{
				if (charSet)
				{
					charSet = false;

					buttons[Buttons::BT_CHARC_SKINL]->set_active(true);
					buttons[Buttons::BT_CHARC_SKINR]->set_active(true);

					buttons[Buttons::BT_CHARC_FACEL]->set_active(true);
					buttons[Buttons::BT_CHARC_FACER]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRL]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRR]->set_active(true);
					buttons[Buttons::BT_CHARC_TOPL]->set_active(true);
					buttons[Buttons::BT_CHARC_TOPR]->set_active(true);
					buttons[Buttons::BT_CHARC_SHOESL]->set_active(true);
					buttons[Buttons::BT_CHARC_SHOESR]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPL]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPR]->set_active(true);

					for (size_t i = 0; i <= 7; i++)
						buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(true);

					buttons[Buttons::BT_CHARC_OK]->set_position(slay(489, 437));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(slay(563, 437));

					namechar.set_state(Textfield::State::DISABLED);

					return Button::State::NORMAL;
				}
				else
				{
					if (gender)
					{
						gender = false;

						buttons[Buttons::BT_CHARC_GENDER_M]->set_active(true);
						buttons[Buttons::BT_CHARC_GEMDER_F]->set_active(true);

						buttons[Buttons::BT_CHARC_SKINL]->set_active(false);
						buttons[Buttons::BT_CHARC_SKINR]->set_active(false);

						buttons[Buttons::BT_CHARC_FACEL]->set_active(false);
						buttons[Buttons::BT_CHARC_FACER]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRL]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRR]->set_active(false);
						buttons[Buttons::BT_CHARC_TOPL]->set_active(false);
						buttons[Buttons::BT_CHARC_TOPR]->set_active(false);
						buttons[Buttons::BT_CHARC_SHOESL]->set_active(false);
						buttons[Buttons::BT_CHARC_SHOESR]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPL]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPR]->set_active(false);

						for (size_t i = 0; i <= 7; i++)
							buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(false);

						buttons[Buttons::BT_CHARC_OK]->set_position(slay(480, 406));
						buttons[Buttons::BT_CHARC_CANCEL]->set_position(slay(556, 406));

						return Button::State::NORMAL;
					}
					else
					{
						button_pressed(Buttons::BT_BACK);

						return Button::State::NORMAL;
					}
				}
			}
			case Buttons::BT_CHARC_FACEL:
			{
				face = (face > 0) ? face - 1 : faces[female].size() - 1;
				newchar.set_face(faces[female][face]);
				facename.change_text(newchar.get_face()->get_name());

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_FACER:
			{
				face = (face < faces[female].size() - 1) ? face + 1 : 0;
				newchar.set_face(faces[female][face]);
				facename.change_text(newchar.get_face()->get_name());

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_HAIRL:
			{
				hair = (hair > 0) ? hair - 1 : hairs[female].size() - 1;
				newchar.set_hair(hairs[female][hair] + haircolors[female][haircolor]);
				hairname.change_text(newchar.get_hair()->get_name());

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_HAIRR:
			{
				hair = (hair < hairs[female].size() - 1) ? hair + 1 : 0;
				newchar.set_hair(hairs[female][hair] + haircolors[female][haircolor]);
				hairname.change_text(newchar.get_hair()->get_name());

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_HAIRC0:
			case Buttons::BT_CHARC_HAIRC1:
			case Buttons::BT_CHARC_HAIRC2:
			case Buttons::BT_CHARC_HAIRC3:
			case Buttons::BT_CHARC_HAIRC4:
			case Buttons::BT_CHARC_HAIRC5:
			case Buttons::BT_CHARC_HAIRC6:
			case Buttons::BT_CHARC_HAIRC7:
			{
				// Direct color selection: button index maps to color index
				size_t colorindex = buttonid - Buttons::BT_CHARC_HAIRC0;

				if (colorindex < haircolors[female].size())
				{
					haircolor = colorindex;
					newchar.set_hair(hairs[female][hair] + haircolors[female][haircolor]);
					hairname.change_text(newchar.get_hair()->get_name());
				}

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_SKINL:
			{
				skin = (skin > 0) ? skin - 1 : skins[female].size() - 1;
				newchar.set_body(skins[female][skin]);
				bodyname.change_text(newchar.get_body()->get_name());

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_SKINR:
			{
				skin = (skin < skins[female].size() - 1) ? skin + 1 : 0;
				newchar.set_body(skins[female][skin]);
				bodyname.change_text(newchar.get_body()->get_name());

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_TOPL:
			{
				top = (top > 0) ? top - 1 : tops[female].size() - 1;
				newchar.add_equip(tops[female][top]);
				topname.change_text(get_equipname(EquipSlot::Id::TOP));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_TOPR:
			{
				top = (top < tops[female].size() - 1) ? top + 1 : 0;
				newchar.add_equip(tops[female][top]);
				topname.change_text(get_equipname(EquipSlot::Id::TOP));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_BOTL:
			{
				bot = (bot > 0) ? bot - 1 : bots[female].size() - 1;
				newchar.add_equip(bots[female][bot]);

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_BOTR:
			{
				bot = (bot < bots[female].size() - 1) ? bot + 1 : 0;
				newchar.add_equip(bots[female][bot]);

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_SHOESL:
			{
				shoe = (shoe > 0) ? shoe - 1 : shoes[female].size() - 1;
				newchar.add_equip(shoes[female][shoe]);
				shoename.change_text(get_equipname(EquipSlot::Id::SHOES));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_SHOESR:
			{
				shoe = (shoe < shoes[female].size() - 1) ? shoe + 1 : 0;
				newchar.add_equip(shoes[female][shoe]);
				shoename.change_text(get_equipname(EquipSlot::Id::SHOES));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_WEPL:
			{
				weapon = (weapon > 0) ? weapon - 1 : weapons[female].size() - 1;
				newchar.add_equip(weapons[female][weapon]);
				wepname.change_text(get_equipname(EquipSlot::Id::WEAPON));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_WEPR:
			{
				weapon = (weapon < weapons[female].size() - 1) ? weapon + 1 : 0;
				newchar.add_equip(weapons[female][weapon]);
				wepname.change_text(get_equipname(EquipSlot::Id::WEAPON));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_GENDER_M:
			{
				if (female)
				{
					female = false;
					randomize_look();
				}

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_GEMDER_F:
			{
				if (!female)
				{
					female = true;
					randomize_look();
				}

				return Button::State::NORMAL;
			}
		}

		return Button::State::PRESSED;
	}

	void UIExplorerCreation::randomize_look()
	{
		hair = randomizer.next_int(hairs[female].size());
		face = randomizer.next_int(faces[female].size());
		skin = randomizer.next_int(skins[female].size());
		haircolor = randomizer.next_int(haircolors[female].size());
		top = randomizer.next_int(tops[female].size());
		bot = 0;
		shoe = randomizer.next_int(shoes[female].size());
		weapon = randomizer.next_int(weapons[female].size());

		newchar.set_body(skins[female][skin]);
		newchar.set_face(faces[female][face]);
		newchar.set_hair(hairs[female][hair] + haircolors[female][haircolor]);
		newchar.add_equip(tops[female][top]);
		newchar.add_equip(bots[female][bot]);
		newchar.add_equip(shoes[female][shoe]);
		newchar.add_equip(weapons[female][weapon]);

		bodyname.change_text(newchar.get_body()->get_name());
		facename.change_text(newchar.get_face()->get_name());
		hairname.change_text(newchar.get_hair()->get_name());
		topname.change_text(get_equipname(EquipSlot::Id::TOP));
		shoename.change_text(get_equipname(EquipSlot::Id::SHOES));
		wepname.change_text(get_equipname(EquipSlot::Id::WEAPON));
	}
}