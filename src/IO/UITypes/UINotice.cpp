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
#include "UINotice.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Audio/Audio.h"
#include "../../Constants.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UINotice::UINotice(std::string message, NoticeType t, Text::Alignment a) : UIDragElement<PosNOTICE>(Point<int16_t>(250, 20)), type(t), alignment(a)
	{
		nl::node src = nl::nx::ui["Basic.img"]["Notice6"];

		top = src["t"];
		center = src["c"];
		centerbox = src["c_box"];
		box = src["box"];
		box2 = src["box2"];
		bottom = src["s"];
		bottombox = src["s_box"];

		if (type == NoticeType::YESNO)
		{
			question = Text(Text::Font::A11M, alignment, Color::Name::WHITE, message, 200);
		}
		else if (type == NoticeType::ENTERNUMBER)
		{
			question = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, message, 200);
		}
		else if (type == NoticeType::OK)
		{
			uint16_t maxwidth = top.width() - 6;
			question = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, message, maxwidth);
		}

		height = question.height();
		dimension = Point<int16_t>(top.width(), top.height() + height + bottom.height());

		// Settings store the top-left corner already (that's what
		// UIDragElement saves). Re-centring on every open caused the
		// popup to drift left and up each time it was reopened.
		dragarea = Point<int16_t>(dimension.x(), 20);

		if (type != NoticeType::ENTERNUMBER)
			Sound(Sound::Name::DLGNOTICE).play();
	}

	UINotice::UINotice(std::string message, NoticeType t) : UINotice(message, t, Text::Alignment::CENTER) {}

	void UINotice::draw(bool textfield) const
	{
		Point<int16_t> start = position;

		top.draw(start);
		start.shift_y(top.height());

		if (textfield)
		{
			center.draw(start);
			start.shift_y(center.height());
			centerbox.draw(start);
			start.shift_y(centerbox.height() - 1);
			box2.draw(start);
			start.shift_y(box2.height());
			box.draw(DrawArgument(start, Point<int16_t>(0, 29)));
			start.shift_y(29);

			question.draw(position + Point<int16_t>(13, 8));
		}
		else
		{
			int16_t pos_y = height >= 32 ? height : 32;

			center.draw(DrawArgument(start, Point<int16_t>(0, pos_y)));
			start.shift_y(pos_y);
			centerbox.draw(start);
			start.shift_y(centerbox.height());
			box.draw(start);
			start.shift_y(box.height());

			if (type == NoticeType::YESNO && alignment == Text::Alignment::LEFT)
				question.draw(position + Point<int16_t>(31, 14));
			else
				question.draw(position + Point<int16_t>(131, 14));
		}

		bottombox.draw(start);
	}

	int16_t UINotice::box2offset(bool textfield) const
	{
		int16_t offset = top.height() + centerbox.height() + box.height() + height - (textfield ? 0 : 16);

		if (type == NoticeType::OK)
			if (height < 34)
				offset += 15;

		return offset;
	}

	UIYesNo::UIYesNo(std::string message, std::function<void(bool yes)> yh, Text::Alignment alignment,
		int16_t button_x_offset, int16_t button_y_offset) : UINotice(message, NoticeType::YESNO, alignment)
	{
		yesnohandler = yh;

		int16_t belowtext = box2offset(false) + button_y_offset;

		nl::node src = nl::nx::ui["Basic.img"];

		buttons[Buttons::YES] = std::make_unique<MapleButton>(src["BtOK4"], Point<int16_t>(156 + button_x_offset, belowtext));
		buttons[Buttons::NO] = std::make_unique<MapleButton>(src["BtCancel4"], Point<int16_t>(198 + button_x_offset, belowtext));
	}

	UIYesNo::UIYesNo(std::string message, std::function<void(bool yes)> yesnohandler) : UIYesNo(message, yesnohandler, Text::Alignment::CENTER) {}

	void UIYesNo::draw(float alpha) const
	{
		UINotice::draw(false);
		UIElement::draw(alpha);
	}

	void UIYesNo::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed)
			return;

		if (keycode == KeyAction::Id::RETURN)
		{
			yesnohandler(true);
			deactivate();
		}
		else if (escape)
		{
			yesnohandler(false);
			deactivate();
		}
	}

	UIElement::Type UIYesNo::get_type() const
	{
		return TYPE;
	}

	Button::State UIYesNo::button_pressed(uint16_t buttonid)
	{
		deactivate();

		switch (buttonid)
		{
		case Buttons::YES:
			yesnohandler(true);
			break;
		case Buttons::NO:
			yesnohandler(false);
			break;
		}

		return Button::State::PRESSED;
	}

	UIEnterNumber::UIEnterNumber(std::string message, std::function<void(int32_t)> nh, int32_t m, int32_t quantity,
		int16_t field_y_offset, int16_t button_x_offset, int16_t button_y_offset) : UINotice(message, NoticeType::ENTERNUMBER)
	{
		numhandler = nh;
		max = m;

		int16_t belowtext = box2offset(true) + field_y_offset;
		int16_t pos_y = belowtext + 35 + button_y_offset;

		nl::node src = nl::nx::ui["Basic.img"];

		buttons[Buttons::OK] = std::make_unique<MapleButton>(src["BtOK4"], 156 + button_x_offset, pos_y);
		buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(src["BtCancel4"], 198 + button_x_offset, pos_y);

		numfield = Textfield(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, Rectangle<int16_t>(24, 232, belowtext, belowtext + 20), 10);
		numfield.change_text(std::to_string(quantity));

		numfield.set_enter_callback(
			[&](std::string numstr)
			{
				handlestring(numstr);
			}
		);

		numfield.set_key_callback(
			KeyAction::Id::ESCAPE,
			[&]()
			{
				deactivate();
			}
		);

		numfield.set_state(Textfield::State::FOCUSED);
	}

	void UIEnterNumber::draw(float alpha) const
	{
		UINotice::draw(true);
		UIElement::draw(alpha);

		numfield.draw(position);
	}

	void UIEnterNumber::update()
	{
		UIElement::update();

		numfield.update(position);
	}

	Cursor::State UIEnterNumber::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (numfield.get_state() == Textfield::State::NORMAL)
		{
			Cursor::State nstate = numfield.send_cursor(cursorpos, clicked);

			if (nstate != Cursor::State::IDLE)
				return nstate;
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIEnterNumber::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (keycode == KeyAction::Id::RETURN)
		{
			handlestring(numfield.get_text());
			deactivate();
		}
		else if (escape)
		{
			deactivate();
		}
	}

	UIElement::Type UIEnterNumber::get_type() const
	{
		return TYPE;
	}

	Button::State UIEnterNumber::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::OK:
			handlestring(numfield.get_text());
			break;
		case Buttons::CANCEL:
			deactivate();
			break;
		}

		return Button::State::NORMAL;
	}

	void UIEnterNumber::handlestring(std::string numstr)
	{
		int num = -1;
		bool has_only_digits = (numstr.find_first_not_of("0123456789") == std::string::npos);

		// okhandler must NOT capture `this` — UIOk and UIEnterNumber
		// share Type::NOTICE, so `emplace<UIOk>` destroys this very
		// window. Any reference to `numfield` / `buttons` in the
		// callback would fire against a freed object → crash on click.
		auto okhandler = [](bool) {};

		if (!has_only_digits)
		{
			UI::get().emplace<UIOk>("Only numbers are allowed.", okhandler);
			return;
		}
		else
		{
			num = std::stoi(numstr);
		}

		if (num < 1)
		{
			UI::get().emplace<UIOk>("You may only enter a number equal to or higher than 1.", okhandler);
			return;
		}
		else if (num > max)
		{
			UI::get().emplace<UIOk>("You may only enter a number equal to or lower than " + std::to_string(max) + ".", okhandler);
			return;
		}
		else
		{
			numhandler(num);
			deactivate();
		}

		buttons[Buttons::OK]->set_state(Button::State::NORMAL);
	}

	UIEnterText::UIEnterText(std::string message, std::function<void(const std::string&)> th, int32_t maxlength,
		int16_t field_y_offset, int16_t button_x_offset, int16_t button_y_offset) : UINotice(message, NoticeType::ENTERNUMBER)
	{
		texthandler = th;

		int16_t belowtext = box2offset(true) - 21 + field_y_offset;
		int16_t pos_y = belowtext + 35 + button_y_offset;

		nl::node src = nl::nx::ui["Basic.img"];

		buttons[Buttons::OK]     = std::make_unique<MapleButton>(src["BtOK4"],     156 + button_x_offset, pos_y);
		buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(src["BtCancel4"], 198 + button_x_offset, pos_y);

		textfield = Textfield(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, Rectangle<int16_t>(24, 232, belowtext, belowtext + 20), maxlength);

		textfield.set_enter_callback(
			[&](std::string str)
			{
				if (!str.empty())
				{
					texthandler(str);
					deactivate();
				}
			}
		);

		textfield.set_key_callback(
			KeyAction::Id::ESCAPE,
			[&]()
			{
				deactivate();
			}
		);

		textfield.set_state(Textfield::State::FOCUSED);
	}

	void UIEnterText::draw(float alpha) const
	{
		UINotice::draw(true);
		UIElement::draw(alpha);

		textfield.draw(position);
	}

	void UIEnterText::update()
	{
		UIElement::update();

		textfield.update(position);
	}

	Cursor::State UIEnterText::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (textfield.get_state() == Textfield::State::NORMAL)
		{
			Cursor::State nstate = textfield.send_cursor(cursorpos, clicked);

			if (nstate != Cursor::State::IDLE)
				return nstate;
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIEnterText::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIEnterText::get_type() const
	{
		return TYPE;
	}

	Button::State UIEnterText::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::OK:
		{
			std::string str = textfield.get_text();
			if (!str.empty())
			{
				texthandler(str);
				deactivate();
			}
			break;
		}
		case Buttons::CANCEL:
			deactivate();
			break;
		}

		return Button::State::NORMAL;
	}

	// ---------- UIWaitNotice ----------

	UIWaitNotice::UIWaitNotice(std::string message) : UINotice(message, NoticeType::OK)
	{
		// No buttons — purely informational. The trade/quest handler
		// will UI::remove() us when the pending state resolves.
	}

	void UIWaitNotice::draw(float alpha) const
	{
		UINotice::draw(false);
		UIElement::draw(alpha);
	}

	void UIWaitNotice::send_key(int32_t, bool, bool) { }
	UIElement::Type UIWaitNotice::get_type() const { return TYPE; }

	UIOk::UIOk(std::string message, std::function<void(bool ok)> oh) : UINotice(message, NoticeType::OK)
	{
		okhandler = oh;

		nl::node src = nl::nx::ui["Basic.img"];

		buttons[Buttons::OK] = std::make_unique<MapleButton>(src["BtOK4"], 197, box2offset(false));
	}

	void UIOk::draw(float alpha) const
	{
		UINotice::draw(false);
		UIElement::draw(alpha);
	}

	void UIOk::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (keycode == KeyAction::Id::RETURN)
			{
				okhandler(true);
				deactivate();
			}
			else if (escape)
			{
				okhandler(false);
				deactivate();
			}
		}
	}

	UIElement::Type UIOk::get_type() const
	{
		return TYPE;
	}

	Button::State UIOk::button_pressed(uint16_t buttonid)
	{
		deactivate();

		switch (buttonid)
		{
		case Buttons::OK:
			okhandler(true);
			break;
		}

		return Button::State::NORMAL;
	}

	// ---------- UIAlarmInvite ----------

	UIAlarmInvite::UIAlarmInvite(std::string message, std::function<void(bool yes)> h, int variant)
		: UIDragElement<PosNOTICE>(Point<int16_t>(0, 0))
	{
		handler = h;

		nl::node root = nl::nx::ui["UIWindow.img"]["FadeYesNo"];

		// Variant 0 = "backgrnd"; 1..11 = "backgrnd1".."backgrnd11".
		std::string bg_name = (variant == 0) ? "backgrnd"
			: "backgrnd" + std::to_string(variant);
		backdrop = Texture(root[bg_name]);
		if (!backdrop.is_valid())
			backdrop = Texture(root["backgrnd"]);

		icon = Texture(root["icon" + std::to_string(variant)]);
		if (!icon.is_valid())
			icon = Texture(root["icon0"]);

		Point<int16_t> dims = backdrop.get_dimensions();
		dimension = dims;
		dragarea  = Point<int16_t>(dims.x(), 20);

		// Anchor above the notice button, clamped to the viewport.
		int16_t vw = Constants::Constants::get().get_viewwidth();
		int16_t vh = Constants::Constants::get().get_viewheight();
		int16_t notice_x = 512 + 56;
		int16_t anchor_x = notice_x - dims.x() / 2 - 75;
		if (anchor_x < 4) anchor_x = 4;
		if (anchor_x + dims.x() > vw - 4) anchor_x = vw - dims.x() - 4;
		int16_t anchor_y = vh - dims.y() - 60;           // 10px down
		position = Point<int16_t>(anchor_x, anchor_y);

		// Text next to the icon (icon on the left, text to its right).
		int16_t icon_w = icon.is_valid() ? icon.get_dimensions().x() : 18;
		int16_t text_x = 6 + icon_w + 6;
		int16_t text_area = dims.x() - text_x - 6;
		label = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::WHITE, message, text_area);

		// OK / Cancel are 12x12 tucked into the bottom-right corner.
		buttons[Buttons::ACCEPT]  = std::make_unique<MapleButton>(
			root["BtOK"],     Point<int16_t>(dims.x() - 34, dims.y() - 18));
		buttons[Buttons::DECLINE] = std::make_unique<MapleButton>(
			root["BtCancel"], Point<int16_t>(dims.x() - 18, dims.y() - 18));

		Sound(Sound::Name::DLGNOTICE).play();
	}

	void UIAlarmInvite::draw(float alpha) const
	{
		backdrop.draw(DrawArgument(position));
		if (icon.is_valid())
			icon.draw(DrawArgument(position + Point<int16_t>(6, 6)));

		int16_t icon_w = icon.is_valid() ? icon.get_dimensions().x() : 18;
		label.draw(position + Point<int16_t>(6 + icon_w + 6, 6));

		UIElement::draw(alpha);
	}

	void UIAlarmInvite::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			handler(false);
			deactivate();
		}
	}

	UIElement::Type UIAlarmInvite::get_type() const { return TYPE; }

	Button::State UIAlarmInvite::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case Buttons::ACCEPT:
			handler(true);
			deactivate();
			break;
		case Buttons::DECLINE:
			handler(false);
			deactivate();
			break;
		}
		return Button::State::NORMAL;
	}

	// ---------- UIDeathNotice ----------

	UIDeathNotice::UIDeathNotice(std::string message, std::function<void(bool ok)> oh)
		: UIDragElement<PosNOTICE>(Point<int16_t>(0, 0))
	{
		okhandler = oh;

		nl::node notice_root = nl::nx::ui["UIWindow.img"]["Notice"];
		backdrop = Texture(notice_root["0"]);
		dimension = backdrop.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);

		question = Text(Text::Font::A12M, Text::Alignment::CENTER,
			Color::Name::WHITE, message, dimension.x() - 30);

		// OK button — reuse the Basic.img/BtOK4 sprite. Place it centered
		// near the bottom of the 286x146 frame.
		nl::node basic = nl::nx::ui["Basic.img"];
		buttons[Buttons::OK] = std::make_unique<MapleButton>(
			basic["BtOK4"],
			Point<int16_t>(dimension.x() / 2 - 30, dimension.y() - 32));

		Sound(Sound::Name::DLGNOTICE).play();
	}

	void UIDeathNotice::draw(float alpha) const
	{
		backdrop.draw(DrawArgument(position));
		question.draw(position + Point<int16_t>(dimension.x() / 2, 48));
		UIElement::draw(alpha);
	}

	void UIDeathNotice::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && (keycode == KeyAction::Id::RETURN || escape))
		{
			okhandler(true);
			deactivate();
		}
	}

	UIElement::Type UIDeathNotice::get_type() const
	{
		return TYPE;
	}

	Button::State UIDeathNotice::button_pressed(uint16_t buttonid)
	{
		if (buttonid == Buttons::OK)
		{
			okhandler(true);
			deactivate();
		}
		return Button::State::NORMAL;
	}
}