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
#include "UIAranCreation.h"

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
	UIAranCreation::UIAranCreation() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600))
	{
		// The v83 Aran board has no separate gender screen: gender is the last
		// row on the Character Setting scroll.
		gender = true;
		charSet = false;
		named = false;

		ui_scale = std::min(UIScale::scale_x(), UIScale::scale_y());
		box = Point<int16_t>(
			static_cast<int16_t>((UIScale::view_width() - 800.0f * ui_scale) / 2.0f),
			static_cast<int16_t>((UIScale::view_height() - 600.0f * ui_scale) / 2.0f));

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		nl::node Login = nl::nx::ui["Login.img"];
		nl::node CustomizeChar = Login["CustomizeChar"]["2000"];
		nl::node back = nl::nx::map["Back"]["Rien.img"]["back"];

		// Aran are created on Rien: icy texture base with the frozen cavern
		// scene (back/21, centered origin) over it.
		sky = Texture(back["18"]);
		cloud = back["18"];

		sprites.emplace_back(back["21"], DrawArgument(lay(400, 329), ui_scale, ui_scale));

		// snow-capped rock ledge the new Aran stands on
		sprites.emplace_back(back["29"], DrawArgument(lay(394, 452), ui_scale, ui_scale));

		sprites_lookboard.emplace_back(CustomizeChar["charSet"], DrawArgument(lay(473, 103), ui_scale, ui_scale));

		for (size_t i = 0; i <= 8; i++)
			sprites_lookboard.emplace_back(CustomizeChar["avatarSel"][i]["normal"], DrawArgument(lay(504, static_cast<int16_t>(187 + (i * 18))), ui_scale, ui_scale));

		// rows: 0 face, 1 hair style, 2 hair color, 3 skin, 4 top, 5 bottom,
		// 6 shoes, 7 weapon, 8 gender
		auto arrow_pair = [&](uint16_t left, uint16_t right, int16_t row)
		{
			buttons[left] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(562, static_cast<int16_t>(187 + (row * 18))));
			buttons[right] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(699, static_cast<int16_t>(187 + (row * 18))));
		};
		arrow_pair(Buttons::BT_CHARC_FACEL, Buttons::BT_CHARC_FACER, 0);
		arrow_pair(Buttons::BT_CHARC_HAIRL, Buttons::BT_CHARC_HAIRR, 1);
		arrow_pair(Buttons::BT_CHARC_HAIRC0, Buttons::BT_CHARC_HAIRC1, 2);
		arrow_pair(Buttons::BT_CHARC_SKINL, Buttons::BT_CHARC_SKINR, 3);
		arrow_pair(Buttons::BT_CHARC_TOPL, Buttons::BT_CHARC_TOPR, 4);
		arrow_pair(Buttons::BT_CHARC_BOTL, Buttons::BT_CHARC_BOTR, 5);
		arrow_pair(Buttons::BT_CHARC_SHOESL, Buttons::BT_CHARC_SHOESR, 6);
		arrow_pair(Buttons::BT_CHARC_WEPL, Buttons::BT_CHARC_WEPR, 7);
		arrow_pair(Buttons::BT_CHARC_GENDER_M, Buttons::BT_CHARC_GEMDER_F, 8);

		buttons[Buttons::BT_CHARC_OK] = std::make_unique<MapleButton>(CustomizeChar["BtYes"], lay(520, 397));
		buttons[Buttons::BT_CHARC_CANCEL] = std::make_unique<MapleButton>(CustomizeChar["BtNo"], lay(594, 397));

		nameboard = CustomizeChar["charName"];
		namechar = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE, Rectangle<int16_t>(lay(524, 196), lay(630, 253)), 12);

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
		gendername = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);

		// Cosmic validates Aran appearance against the OrientChar pools.
		nl::node mkinfo = nl::nx::etc["MakeCharInfo.img"];

		for (size_t i = 0; i < 2; i++)
		{
			bool f;
			nl::node CharGender;

			if (i == 0)
			{
				f = true;
				CharGender = mkinfo["OrientCharFemale"];
			}
			else
			{
				f = false;
				CharGender = mkinfo["OrientCharMale"];
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

	Point<int16_t> UIAranCreation::lay(int16_t x, int16_t y) const
	{
		return box + Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	Point<int16_t> UIAranCreation::scl(int16_t x, int16_t y) const
	{
		return Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	void UIAranCreation::draw(float inter) const
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

		UIElement::draw_sprites(inter);

		DrawArgument charargs(lay(394, 339), ui_scale, ui_scale);

		if (!charSet)
		{
			for (auto& sprite : sprites_lookboard)
				sprite.draw(Point<int16_t>(0, 0), inter);

			facename.draw(lay(647, 183 + (0 * 18)));
			hairname.draw(lay(647, 183 + (1 * 18)));
			bodyname.draw(lay(647, 183 + (3 * 18)));
			topname.draw(lay(647, 183 + (4 * 18)));
			botname.draw(lay(647, 183 + (5 * 18)));
			shoename.draw(lay(647, 183 + (6 * 18)));
			wepname.draw(lay(647, 183 + (7 * 18)));
			gendername.draw(lay(647, 183 + (8 * 18)));

			newchar.draw(charargs, inter);

			UIElement::draw_buttons(inter);
		}
		else
		{
			if (!named)
			{
				nameboard.draw(DrawArgument(lay(489, 106), ui_scale, ui_scale));

				namechar.draw(Point<int16_t>(0, 0));
				newchar.draw(charargs, inter);

				UIElement::draw_buttons(inter);
			}
			else
			{
				nameboard.draw(DrawArgument(lay(489, 106), ui_scale, ui_scale));

				UIElement::draw_buttons(inter);

				for (auto& sprite : sprites_keytype)
					sprite.draw(Point<int16_t>(0, 0), inter);
			}
		}

		version.draw(lay(707, 4));
	}

	void UIAranCreation::update()
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
				for (auto& sprite : sprites_keytype)
					sprite.update();

				namechar.set_state(Textfield::State::DISABLED);
			}
		}

		UIElement::update();

		cloudfx += 0.25f;
	}

	Cursor::State UIAranCreation::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (namechar.get_state() == Textfield::State::NORMAL)
		{
			if (namechar.get_bounds().contains(cursorpos))
			{
				if (clicked)
				{
					namechar.set_state(Textfield::State::FOCUSED);

					return Cursor::State::CLICKING;
				}
				else
				{
					return Cursor::State::IDLE;
				}
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIAranCreation::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
				button_pressed(Buttons::BT_CHARC_CANCEL);
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(Buttons::BT_CHARC_OK);
		}
	}

	UIElement::Type UIAranCreation::get_type() const
	{
		return TYPE;
	}

	void UIAranCreation::send_naming_result(bool nameused)
	{
		if (!named)
		{
			if (!nameused)
			{
				named = true;

				std::string cname = namechar.get_text();
				int32_t cface = faces[female][face];
				int32_t chair = hairs[female][hair];
				uint8_t chairc = haircolors[female][haircolor];
				uint8_t cskin = skins[female][skin];
				int32_t ctop = tops[female][top];
				int32_t cbot = bots[female][bot];
				int32_t cshoe = shoes[female][shoe];
				int32_t cwep = weapons[female][weapon];

				CreateCharPacket(cname, 2, cface, chair, chairc, cskin, ctop, cbot, cshoe, cwep, female).dispatch();

				auto onok = [&](bool alternate)
				{
					Sound(Sound::Name::SCROLLUP).play();

					UI::get().remove(UIElement::Type::LOGINNOTICE_CONFIRM);
					UI::get().remove(UIElement::Type::LOGINNOTICE);
					UI::get().remove(UIElement::Type::CLASSCREATION);
					UI::get().remove(UIElement::Type::RACESELECT);

					if (auto charselect = UI::get().get_element<UICharSelect>())
						charselect->post_add_character();
				};

				UI::get().emplace<UIKeySelect>(onok, true);
			}
			else
			{
				auto onok = [&]()
				{
					namechar.set_state(Textfield::State::FOCUSED);

					buttons[Buttons::BT_CHARC_OK]->set_state(Button::State::NORMAL);
					buttons[Buttons::BT_CHARC_CANCEL]->set_state(Button::State::NORMAL);
				};

				UI::get().emplace<UILoginNotice>(UILoginNotice::Message::NAME_IN_USE, onok);
			}
		}
	}

	Button::State UIAranCreation::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case Buttons::BT_CHARC_OK:
			{
				if (!charSet)
				{
					charSet = true;

					set_row_buttons(false);

					buttons[Buttons::BT_CHARC_OK]->set_position(lay(523, 243));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(597, 243));

					namechar.set_state(Textfield::State::FOCUSED);

					return Button::State::NORMAL;
				}
				else
				{
					if (!named)
					{
						std::string name = namechar.get_text();

						if (name.size() <= 0)
						{
							return Button::State::NORMAL;
						}
						else if (name.size() >= 4)
						{
							namechar.set_state(Textfield::State::DISABLED);

							buttons[Buttons::BT_CHARC_OK]->set_state(Button::State::DISABLED);
							buttons[Buttons::BT_CHARC_CANCEL]->set_state(Button::State::DISABLED);

							if (auto raceselect = UI::get().get_element<UIRaceSelect>())
							{
								if (raceselect->check_name(name))
								{
									NameCharPacket(name).dispatch();

									return Button::State::IDENTITY;
								}
							}

							std::function<void()> okhandler = [&]()
							{
								namechar.set_state(Textfield::State::FOCUSED);

								buttons[Buttons::BT_CHARC_OK]->set_state(Button::State::NORMAL);
								buttons[Buttons::BT_CHARC_CANCEL]->set_state(Button::State::NORMAL);
							};

							UI::get().emplace<UILoginNotice>(UILoginNotice::Message::ILLEGAL_NAME, okhandler);

							return Button::State::NORMAL;
						}
						else
						{
							namechar.set_state(Textfield::State::DISABLED);

							buttons[Buttons::BT_CHARC_OK]->set_state(Button::State::DISABLED);
							buttons[Buttons::BT_CHARC_CANCEL]->set_state(Button::State::DISABLED);

							std::function<void()> okhandler = [&]()
							{
								namechar.set_state(Textfield::State::FOCUSED);

								buttons[Buttons::BT_CHARC_OK]->set_state(Button::State::NORMAL);
								buttons[Buttons::BT_CHARC_CANCEL]->set_state(Button::State::NORMAL);
							};

							UI::get().emplace<UILoginNotice>(UILoginNotice::Message::ILLEGAL_NAME, okhandler);

							return Button::State::IDENTITY;
						}
					}
					else
					{
						return Button::State::NORMAL;
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

					set_row_buttons(true);

					buttons[Buttons::BT_CHARC_OK]->set_position(lay(520, 397));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(594, 397));

					namechar.set_state(Textfield::State::DISABLED);

					return Button::State::NORMAL;
				}
				else
				{
					button_pressed(Buttons::BT_BACK);

					return Button::State::NORMAL;
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
			{
				haircolor = (haircolor > 0) ? haircolor - 1 : haircolors[female].size() - 1;
				newchar.set_hair(hairs[female][hair] + haircolors[female][haircolor]);

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_HAIRC1:
			{
				haircolor = (haircolor < haircolors[female].size() - 1) ? haircolor + 1 : 0;
				newchar.set_hair(hairs[female][hair] + haircolors[female][haircolor]);

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
				botname.change_text(get_equipname(EquipSlot::Id::BOTTOM));

				return Button::State::NORMAL;
			}
			case Buttons::BT_CHARC_BOTR:
			{
				bot = (bot < bots[female].size() - 1) ? bot + 1 : 0;
				newchar.add_equip(bots[female][bot]);
				botname.change_text(get_equipname(EquipSlot::Id::BOTTOM));

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
			case Buttons::BT_CHARC_GEMDER_F:
			{
				female = !female;
				randomize_look();

				return Button::State::NORMAL;
			}
		}

		return Button::State::PRESSED;
	}

	void UIAranCreation::set_row_buttons(bool active)
	{
		const uint16_t rows[] = {
			Buttons::BT_CHARC_FACEL, Buttons::BT_CHARC_FACER,
			Buttons::BT_CHARC_HAIRL, Buttons::BT_CHARC_HAIRR,
			Buttons::BT_CHARC_HAIRC0, Buttons::BT_CHARC_HAIRC1,
			Buttons::BT_CHARC_SKINL, Buttons::BT_CHARC_SKINR,
			Buttons::BT_CHARC_TOPL, Buttons::BT_CHARC_TOPR,
			Buttons::BT_CHARC_BOTL, Buttons::BT_CHARC_BOTR,
			Buttons::BT_CHARC_SHOESL, Buttons::BT_CHARC_SHOESR,
			Buttons::BT_CHARC_WEPL, Buttons::BT_CHARC_WEPR,
			Buttons::BT_CHARC_GENDER_M, Buttons::BT_CHARC_GEMDER_F
		};

		for (uint16_t id : rows)
			buttons[id]->set_active(active);
	}

	void UIAranCreation::randomize_look()
	{
		hair = randomizer.next_int(hairs[female].size());
		face = randomizer.next_int(faces[female].size());
		skin = randomizer.next_int(skins[female].size());
		haircolor = randomizer.next_int(haircolors[female].size());
		top = 0;
		bot = 0;
		shoe = 0;
		weapon = 0;

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
		gendername.change_text(female ? "Female" : "Male");
	}

	const std::string& UIAranCreation::get_equipname(EquipSlot::Id slot) const
	{
		if (int32_t item_id = newchar.get_equips().get_equip(slot))
		{
			return ItemData::get(item_id).get_name();
		}
		else
		{
			static const std::string& nullstr = "Missing name.";

			return nullstr;
		}
	}
}
