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
#include "UICreationBase.h"

#include "UICharSelect.h"
#include "UILoginNotice.h"
#include "UIRaceSelect.h"

#include "../UI.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"

#include "../../Net/Packets/CharCreationPackets.h"

namespace ms
{
	UICreationBase::UICreationBase(uint16_t creation_job) : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600)), creation_job(creation_job)
	{
	}

	Cursor::State UICreationBase::send_cursor(bool clicked, Point<int16_t> cursorpos)
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

	void UICreationBase::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
				button_pressed(Buttons::BT_CHARC_CANCEL);
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(Buttons::BT_CHARC_OK);
		}
	}

	UIElement::Type UICreationBase::get_type() const
	{
		return TYPE;
	}

	void UICreationBase::send_naming_result(bool nameused)
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

				CreateCharPacket(cname, creation_job, cface, chair, chairc, cskin, ctop, cbot, cshoe, cwep, female).dispatch();

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

	Point<int16_t> UICreationBase::lay(int16_t x, int16_t y) const
	{
		return box + Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	Button::State UICreationBase::naming_ok_pressed()
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

	const std::string& UICreationBase::get_equipname(EquipSlot::Id slot) const
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