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
#include "UICygnusCreation.h"

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

#include <cmath>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UICygnusCreation::UICygnusCreation() : UICreationBase(0)
	{
		gender = false;
		charSet = false;
		named = false;

		ui_scale = std::min(UIScale::scale_x(), UIScale::scale_y());
		box = Point<int16_t>(
			static_cast<int16_t>((UIScale::view_width() - 800.0f * ui_scale) / 2.0f),
			static_cast<int16_t>((UIScale::view_height() - 600.0f * ui_scale) / 2.0f));

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		nl::node Login = nl::nx::ui["Login.img"];
		nl::node CustomizeChar = Login["CustomizeChar"]["1000"];
		nl::node back = nl::nx::map["Back"]["login.img"]["back"];
		nl::node Common = Login["Common"];
		nl::node board = CustomizeChar["board"];
		nl::node genderSelect = CustomizeChar["genderSelect"];

		// v83 login-map Ereve band: shared sky gradient + clouds, the Cygnus
		// gazebo (back/30) with its vine decorations (31/32), map-authored spots.
		sky = Texture(back["2"]);
		cloud = Texture(back["27"]);

		// Gazebo drawn aspect-correct: uniform scale anchored at its map spot,
		// decorations offset in uniform space so they stay glued to it.
		Point<int16_t> gaz_a(
			static_cast<int16_t>(std::lround(265 * UIScale::scale_x())),
			static_cast<int16_t>(std::lround(300 * UIScale::scale_y())));
		auto gaz = [&](nl::node n, int16_t dx, int16_t dy)
		{
			sprites.emplace_back(n, UIScale::uniform_args(Texture(n), gaz_a, Point<int16_t>(0, 0), dx, dy, ui_scale));
		};
		gaz(back["30"], 0, 0);
		gaz(back["31"], 430, -221);
		gaz(back["32"], 376, -150);
		sprites.emplace_back(Common["frame"], UIScale::stretch_args(Texture(Common["frame"]), 400, 300));

		// The v83 Cygnus boards self-place: every piece and row carries an
		// origin measured from the board anchors used below.
		gender_plates.emplace_back(Texture(board["genderTop"]), Point<int16_t>(423, 104));

		for (int16_t f = 0; f < 7; f++)
			gender_plates.emplace_back(Texture(board["boardMid"]), Point<int16_t>(423, static_cast<int16_t>(222 + (18 * f))));

		gender_plates.emplace_back(Texture(board["boardBottom"]), Point<int16_t>(423, 348));

		look_plates.emplace_back(Texture(board["avatarTop"]), Point<int16_t>(415, 89));

		for (int16_t f = 0; f < 8; f++)
			look_plates.emplace_back(Texture(board["boardMid"]), Point<int16_t>(415, static_cast<int16_t>(207 + (18 * f))));

		look_plates.emplace_back(Texture(board["boardBottom"]), Point<int16_t>(415, 351));

		for (size_t i = 0; i <= 6; i++)
			sprites_lookboard.emplace_back(CustomizeChar["avatarSel"][i]["normal"], DrawArgument(lay(416, 98), ui_scale, ui_scale));

		buttons[Buttons::BT_CHARC_GENDER_M] = std::make_unique<MapleButton>(genderSelect["male"], lay(425, 107));
		buttons[Buttons::BT_CHARC_GEMDER_F] = std::make_unique<MapleButton>(genderSelect["female"], lay(423, 107));
		buttons[Buttons::BT_CHARC_FACEL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(418, 98));
		buttons[Buttons::BT_CHARC_FACER] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(415, 98));
		buttons[Buttons::BT_CHARC_HAIRL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(418, 116));
		buttons[Buttons::BT_CHARC_HAIRR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(415, 116));
		buttons[Buttons::BT_CHARC_SKINL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(418, 153));
		buttons[Buttons::BT_CHARC_SKINR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(415, 153));
		buttons[Buttons::BT_CHARC_WEPL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(418, 225));
		buttons[Buttons::BT_CHARC_WEPR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(415, 225));

		for (size_t i = 0; i <= 7; i++)
		{
			buttons[Buttons::BT_CHARC_HAIRC0 + i] = std::make_unique<MapleButton>(CustomizeChar["hairSelect"][i], lay(static_cast<int16_t>(553 + (i * 15)), 238));
			buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(false);
		}

		buttons[Buttons::BT_CHARC_FACEL]->set_active(false);
		buttons[Buttons::BT_CHARC_FACER]->set_active(false);
		buttons[Buttons::BT_CHARC_HAIRL]->set_active(false);
		buttons[Buttons::BT_CHARC_HAIRR]->set_active(false);
		buttons[Buttons::BT_CHARC_SKINL]->set_active(false);
		buttons[Buttons::BT_CHARC_SKINR]->set_active(false);
		buttons[Buttons::BT_CHARC_WEPL]->set_active(false);
		buttons[Buttons::BT_CHARC_WEPR]->set_active(false);

		buttons[Buttons::BT_CHARC_OK] = std::make_unique<MapleButton>(CustomizeChar["BtYes"], lay(510, 396));
		buttons[Buttons::BT_CHARC_CANCEL] = std::make_unique<MapleButton>(CustomizeChar["BtNo"], lay(615, 396));

		nameboard = CustomizeChar["charName"];
		namechar = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::BLACK, Rectangle<int16_t>(lay(539, 209), lay(631, 252)), 12);

		buttons[Buttons::BT_BACK] = std::make_unique<MapleButton>(Login["Common"]["BtStart"], lay(0, 515));

		for (auto& btit : buttons)
			btit.second->set_scale(ui_scale);

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
		botname = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		shoename = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		wepname = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);

		// Cosmic validates Cygnus appearance against the PremiumChar pools.
		nl::node mkinfo = nl::nx::etc["MakeCharInfo.img"];

		for (size_t i = 0; i < 2; i++)
		{
			bool f;
			nl::node CharGender;

			if (i == 0)
			{
				f = true;
				CharGender = mkinfo["PremiumCharFemale"];
			}
			else
			{
				f = false;
				CharGender = mkinfo["PremiumCharMale"];
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

	void UICygnusCreation::draw(float inter) const
	{
		if (sky.is_valid())
		{
			Point<int16_t> o = sky.get_origin();
			sky.draw(DrawArgument(
				o, o,
				Point<int16_t>(
					static_cast<int16_t>(UIScale::view_width()),
					static_cast<int16_t>(UIScale::view_height())),
				1.0f, 1.0f, 1.0f, 0.0f));
		}

		int16_t cloudx = static_cast<int16_t>(cloudfx) % 800;
		cloud.draw(UIScale::stretch_args(cloud, static_cast<int16_t>(cloudx - 800), 300));
		cloud.draw(UIScale::stretch_args(cloud, cloudx, 300));
		cloud.draw(UIScale::stretch_args(cloud, static_cast<int16_t>(cloudx + 800), 300));

		UIElement::draw_sprites(inter);

		DrawArgument charargs(lay(394, 339), ui_scale, ui_scale);

		if (!gender)
		{
			for (const auto& pc : gender_plates)
				pc.first.draw(UIScale::uniform_args(pc.first, box, Point<int16_t>(0, 0), pc.second.x(), pc.second.y(), ui_scale));

			newchar.draw(charargs, inter);

			UIElement::draw_buttons(inter);
		}
		else
		{
			if (!charSet)
			{
			for (const auto& pc : look_plates)
					pc.first.draw(UIScale::uniform_args(pc.first, box, Point<int16_t>(0, 0), pc.second.x(), pc.second.y(), ui_scale));

				for (auto& sprite : sprites_lookboard)
					sprite.draw(Point<int16_t>(0, 0), inter);

				facename.draw(lay(620, 200));
				hairname.draw(lay(620, 218));
				bodyname.draw(lay(620, 254));
				topname.draw(lay(620, 272));
				botname.draw(lay(620, 290));
				shoename.draw(lay(620, 308));
				wepname.draw(lay(620, 326));

				newchar.draw(charargs, inter);

				UIElement::draw_buttons(inter);
			}
			else
			{
				if (!named)
				{
					nameboard.draw(DrawArgument(lay(423, 104), ui_scale, ui_scale));

					namechar.draw(Point<int16_t>(0, 0));
					newchar.draw(charargs, inter);

					UIElement::draw_buttons(inter);
				}
				else
				{
					nameboard.draw(DrawArgument(lay(423, 104), ui_scale, ui_scale));

					UIElement::draw_buttons(inter);

				}
			}
		}

		version.draw(lay(707, 4));
	}

	void UICygnusCreation::update()
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

	Button::State UICygnusCreation::button_pressed(uint16_t buttonid)
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

					buttons[Buttons::BT_CHARC_FACEL]->set_active(true);
					buttons[Buttons::BT_CHARC_FACER]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRL]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRR]->set_active(true);
					buttons[Buttons::BT_CHARC_SKINL]->set_active(true);
					buttons[Buttons::BT_CHARC_SKINR]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPL]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPR]->set_active(true);

					for (size_t i = 0; i <= 7; i++)
						buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(true);

					buttons[Buttons::BT_CHARC_OK]->set_position(lay(502, 381));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(607, 381));

					return Button::State::NORMAL;
				}
				else
				{
					if (!charSet)
					{
						charSet = true;

						buttons[Buttons::BT_CHARC_FACEL]->set_active(false);
						buttons[Buttons::BT_CHARC_FACER]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRL]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRR]->set_active(false);
						buttons[Buttons::BT_CHARC_SKINL]->set_active(false);
						buttons[Buttons::BT_CHARC_SKINR]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPL]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPR]->set_active(false);

						for (size_t i = 0; i <= 7; i++)
							buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(false);

						buttons[Buttons::BT_CHARC_OK]->set_position(lay(510, 289));
						buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(615, 289));

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

					buttons[Buttons::BT_CHARC_FACEL]->set_active(true);
					buttons[Buttons::BT_CHARC_FACER]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRL]->set_active(true);
					buttons[Buttons::BT_CHARC_HAIRR]->set_active(true);
					buttons[Buttons::BT_CHARC_SKINL]->set_active(true);
					buttons[Buttons::BT_CHARC_SKINR]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPL]->set_active(true);
					buttons[Buttons::BT_CHARC_WEPR]->set_active(true);

					for (size_t i = 0; i <= 7; i++)
						buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(true);

					buttons[Buttons::BT_CHARC_OK]->set_position(lay(502, 381));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(607, 381));

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

						buttons[Buttons::BT_CHARC_FACEL]->set_active(false);
						buttons[Buttons::BT_CHARC_FACER]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRL]->set_active(false);
						buttons[Buttons::BT_CHARC_HAIRR]->set_active(false);
						buttons[Buttons::BT_CHARC_SKINL]->set_active(false);
						buttons[Buttons::BT_CHARC_SKINR]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPL]->set_active(false);
						buttons[Buttons::BT_CHARC_WEPR]->set_active(false);

						for (size_t i = 0; i <= 7; i++)
							buttons[Buttons::BT_CHARC_HAIRC0 + i]->set_active(false);

						buttons[Buttons::BT_CHARC_OK]->set_position(lay(510, 396));
						buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(615, 396));

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

	void UICygnusCreation::randomize_look()
	{
		hair = randomizer.next_int(hairs[female].size());
		face = randomizer.next_int(faces[female].size());
		skin = randomizer.next_int(skins[female].size());
		haircolor = randomizer.next_int(haircolors[female].size());
		top = 0;
		bot = 0;
		shoe = 0;
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
		botname.change_text(get_equipname(EquipSlot::Id::BOTTOM));
		shoename.change_text(get_equipname(EquipSlot::Id::SHOES));
		wepname.change_text(get_equipname(EquipSlot::Id::WEAPON));
	}
}
