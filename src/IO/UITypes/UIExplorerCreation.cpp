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
	UIExplorerCreation::UIExplorerCreation() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600))
	{
		gender = false;
		charSet = false;
		named = false;

		// Uniform 800x600 content scaling, centered (same treatment as
		// UIWorldSelect / UICharSelect). Everything below is laid out in design
		// coords and mapped through lay()/scl().
		ui_scale = std::min(UIScale::scale_x(), UIScale::scale_y());
		box = Point<int16_t>(
			static_cast<int16_t>((UIScale::view_width() - 800.0f * ui_scale) / 2.0f),
			static_cast<int16_t>((UIScale::view_height() - 600.0f * ui_scale) / 2.0f));

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

		// Authentic char-creation background = the same scenic login stage as char
		// select: sky backdrop + giant tree / mushroom house / greenery / the
		// rope-bridge the character stands on. (back/1 is the sky gradient strip.)
		sky = Texture(back["1"]);
		cloud = back["27"];

		auto scenepc = [&](nl::node src, int16_t x, int16_t y)
		{
			if (src)
				sprites.emplace_back(src, DrawArgument(lay(x, y), ui_scale, ui_scale));
		};
		// native-size pieces aligned to the bridge chunk (see UICharSelect)
		scenepc(back["10"], 137, 32);   // mushroom house
		scenepc(back["15"], 203, 81);   // tree
		scenepc(back["13"], 201, 695);  // greenery / tree base (2px under the bridge, hides the seam)
		scenepc(back["14"], 375, 393);  // rope-bridge, deck top y 374

		sprites_gender_select.emplace_back(board["genderTop"], DrawArgument(lay(486, 95), ui_scale, ui_scale));
		sprites_gender_select.emplace_back(board["boardMid"], DrawArgument(lay(486, 209), ui_scale, ui_scale));
		sprites_gender_select.emplace_back(board["boardBottom"], DrawArgument(lay(486, 329), ui_scale, ui_scale));
		sprites_lookboard.emplace_back(CustomizeChar["charSet"], DrawArgument(lay(486, 95), ui_scale, ui_scale));

		for (size_t i = 0; i <= 5; i++)
		{
			size_t f = i;

			if (i >= 2)
				f++;

			sprites_lookboard.emplace_back(CustomizeChar["avatarSel"][i]["normal"], DrawArgument(lay(497, 197 + (f * 18)), ui_scale, ui_scale));
		}

		// stat table (STR/DEX/INT/LUK frame) and the randomize dice, inside the
		// CHARACTER SETTINGS board.
		sprites_lookboard.emplace_back(CustomizeChar["statTb"], DrawArgument(lay(506, 330), ui_scale, ui_scale));
		sprites_lookboard.emplace_back(CustomizeChar["dice"]["2"], DrawArgument(lay(690, 120), ui_scale, ui_scale));

		buttons[Buttons::BT_CHARC_GENDER_M] = std::make_unique<MapleButton>(genderSelect["male"], lay(487, 109));
		buttons[Buttons::BT_CHARC_GEMDER_F] = std::make_unique<MapleButton>(genderSelect["female"], lay(485, 109));
		buttons[Buttons::BT_CHARC_FACEL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(552, 198 + (0 * 18)));
		buttons[Buttons::BT_CHARC_FACER] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(684, 198 + (0 * 18)));
		buttons[Buttons::BT_CHARC_HAIRL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(552, 198 + (1 * 18)));
		buttons[Buttons::BT_CHARC_HAIRR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(684, 198 + (1 * 18)));
		buttons[Buttons::BT_CHARC_SKINL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(552, 198 + (3 * 18)));
		buttons[Buttons::BT_CHARC_SKINR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(684, 198 + (3 * 18)));
		buttons[Buttons::BT_CHARC_TOPL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(552, 198 + (4 * 18)));
		buttons[Buttons::BT_CHARC_TOPR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(684, 198 + (4 * 18)));
		buttons[Buttons::BT_CHARC_SHOESL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(552, 198 + (5 * 18)));
		buttons[Buttons::BT_CHARC_SHOESR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(684, 198 + (5 * 18)));
		buttons[Buttons::BT_CHARC_WEPL] = std::make_unique<MapleButton>(CustomizeChar["BtLeft"], lay(552, 198 + (6 * 18)));
		buttons[Buttons::BT_CHARC_WEPR] = std::make_unique<MapleButton>(CustomizeChar["BtRight"], lay(684, 198 + (6 * 18)));

		for (size_t i = 0; i <= 7; i++)
		{
			buttons[Buttons::BT_CHARC_HAIRC0 + i] = std::make_unique<MapleButton>(CustomizeChar["hairSelect"][i], lay(549 + (i * 15), 234));
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

		buttons[Buttons::BT_CHARC_OK] = std::make_unique<MapleButton>(CustomizeChar["BtYes"], lay(514, 394));
		buttons[Buttons::BT_CHARC_CANCEL] = std::make_unique<MapleButton>(CustomizeChar["BtNo"], lay(590, 394));

		nameboard = CustomizeChar["charName"];
		namechar = Textfield(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE, Rectangle<int16_t>(lay(522, 195), lay(630, 253)), 12);

		buttons[Buttons::BT_BACK] = std::make_unique<MapleButton>(Login["Common"]["BtStart"], lay(0, 515));

		// Uniform-scale every button (draw + hit bounds).
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
		return box + Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	Point<int16_t> UIExplorerCreation::scl(int16_t x, int16_t y) const
	{
		return Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	void UIExplorerCreation::draw(float inter) const
	{
		// backdrop: the 20px-wide v83 sky strip, tiled sideways at scene scale
		// (stretching it smears the columns into vertical bands)
		if (sky.is_valid())
		{
			Point<int16_t> o = sky.get_origin();
			int16_t vw = static_cast<int16_t>(UIScale::view_width());
			int16_t vh = static_cast<int16_t>(UIScale::view_height());
			int16_t tw = static_cast<int16_t>(sky.get_dimensions().x() * ui_scale);
			if (tw < 1)
				tw = 1;
			for (int16_t x = 0; x < vw; x = static_cast<int16_t>(x + tw))
			{
				Point<int16_t> p = o + Point<int16_t>(x, 0);
				sky.draw(DrawArgument(p, p, Point<int16_t>(tw, vh), 1.0f, 1.0f, 1.0f, 0.0f));
			}
		}

		// scenic stage (tree / house / greenery / bridge) sits on top of the sky
		UIElement::draw_sprites(inter);

		// feet on the bridge deck, same line as char select
		DrawArgument charargs(lay(394, 374), ui_scale, ui_scale);

		if (!gender)
		{
			// gender-select board: top plate, mid rows (repeated), bottom plate
			for (size_t i = 0; i < sprites_gender_select.size(); i++)
			{
				if (i == 1)
				{
					for (size_t f = 0; f <= 4; f++)
						sprites_gender_select[i].draw(scl(0, 24 * static_cast<int16_t>(f)), inter);
				}
				else
				{
					sprites_gender_select[i].draw(Point<int16_t>(0, 0), inter);
				}
			}

			newchar.draw(charargs, inter);

			UIElement::draw_buttons(inter);
		}
		else
		{
			if (!charSet)
			{
				for (auto& sprite : sprites_lookboard)
					sprite.draw(Point<int16_t>(0, 0), inter);

				facename.draw(lay(625, 193 + (0 * 18)));
				hairname.draw(lay(625, 193 + (1 * 18)));
				bodyname.draw(lay(625, 193 + (3 * 18)));
				topname.draw(lay(625, 193 + (4 * 18)));
				shoename.draw(lay(625, 193 + (5 * 18)));
				wepname.draw(lay(625, 193 + (6 * 18)));

				newchar.draw(charargs, inter);

				UIElement::draw_buttons(inter);
			}
			else
			{
				if (!named)
				{
					nameboard.draw(DrawArgument(lay(486, 95), ui_scale, ui_scale));

					namechar.draw(Point<int16_t>(0, 0));
					newchar.draw(charargs, inter);

					UIElement::draw_buttons(inter);
				}
				else
				{
					nameboard.draw(DrawArgument(lay(486, 95), ui_scale, ui_scale));

					UIElement::draw_buttons(inter);

					for (auto& sprite : sprites_keytype)
						sprite.draw(Point<int16_t>(0, 0), inter);
				}
			}
		}

		version.draw(lay(707, 4));
	}

	void UIExplorerCreation::update()
	{
		if (!gender)
		{
			for (auto& sprite : sprites_gender_select)
				sprite.update();

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
					for (auto& sprite : sprites_keytype)
						sprite.update();

					namechar.set_state(Textfield::State::DISABLED);
				}
			}
		}

		UIElement::update();

		cloudfx += 0.25f;
	}

	Cursor::State UIExplorerCreation::send_cursor(bool clicked, Point<int16_t> cursorpos)
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

	void UIExplorerCreation::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
				button_pressed(Buttons::BT_CHARC_CANCEL);
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(Buttons::BT_CHARC_OK);
		}
	}

	UIElement::Type UIExplorerCreation::get_type() const
	{
		return TYPE;
	}

	void UIExplorerCreation::send_naming_result(bool nameused)
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

				CreateCharPacket(cname, 1, cface, chair, chairc, cskin, ctop, cbot, cshoe, cwep, female).dispatch();

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

					buttons[Buttons::BT_CHARC_OK]->set_position(lay(523, 425));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(597, 425));

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

						buttons[Buttons::BT_CHARC_OK]->set_position(lay(513, 273));
						buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(587, 273));

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

					buttons[Buttons::BT_CHARC_OK]->set_position(lay(523, 425));
					buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(597, 425));

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

						buttons[Buttons::BT_CHARC_OK]->set_position(lay(514, 394));
						buttons[Buttons::BT_CHARC_CANCEL]->set_position(lay(590, 394));

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

	const std::string& UIExplorerCreation::get_equipname(EquipSlot::Id slot) const
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