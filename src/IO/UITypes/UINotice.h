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
#pragma once

#include "../UIDragElement.h"

#include "../Components/Textfield.h"

namespace ms
{
	class UINotice : public UIDragElement<PosNOTICE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::NOTICE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

	protected:
		enum NoticeType : uint8_t
		{
			YESNO,
			ENTERNUMBER,
			OK
		};

		UINotice(std::string message, NoticeType type, Text::Alignment alignment);
		UINotice(std::string message, NoticeType type);

		void draw(bool textfield) const;

		int16_t box2offset(bool textfield) const;

	private:
		Texture top;
		Texture center;
		Texture centerbox;
		Texture box;
		Texture box2;
		Texture bottom;
		Texture bottombox;
		Text question;
		int16_t height;
		NoticeType type;
		Text::Alignment alignment;
	};

	class UIYesNo : public UINotice
	{
	public:
		UIYesNo(std::string message, std::function<void(bool yes)> yesnohandler,
			Text::Alignment alignment,
			int16_t button_x_offset = 0,
			int16_t button_y_offset = 0);
		UIYesNo(std::string message, std::function<void(bool yes)> yesnohandler);

		void draw(float alpha) const override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : int16_t
		{
			YES, NO
		};

		std::function<void(bool yes)> yesnohandler;
	};

	class UIEnterNumber : public UINotice
	{
	public:
		UIEnterNumber(std::string message, std::function<void(int32_t number)> numhandler, int32_t max, int32_t quantity,
			int16_t field_y_offset = -41,
			int16_t button_x_offset = 0,
			int16_t button_y_offset = 0);

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool pressed, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void handlestring(std::string numstr);

		enum Buttons : int16_t
		{
			OK, CANCEL
		};

		std::function<void(int32_t number)> numhandler;
		Textfield numfield;
		int32_t max;
	};

	class UIEnterText : public UINotice
	{
	public:
		UIEnterText(std::string message, std::function<void(const std::string& text)> texthandler, int32_t maxlength = 15,
			int16_t field_y_offset = 0, int16_t button_x_offset = 0, int16_t button_y_offset = 0);

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool pressed, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : int16_t
		{
			OK, CANCEL
		};

		std::function<void(const std::string& text)> texthandler;
		Textfield textfield;
	};

	// Wide 286x146 modal used specifically for the "You have died"
	// popup. Sources the backdrop from UIWindow.img/Notice/0 so the
	// death dialog reads as a proper standalone panel instead of the
	// small generic Basic.img/Notice6 frame used by UIOk.
	class UIDeathNotice : public UIDragElement<PosNOTICE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::NOTICE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

		UIDeathNotice(std::string message, std::function<void(bool ok)> okhandler);

		void draw(float alpha) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : int16_t { OK };

		std::function<void(bool ok)> okhandler;
		Texture backdrop;
		Text question;
	};

	// Small v83 "invite banner" that appears above the notice button.
	// Sourced from UIWindow.img/FadeYesNo — a single flat backdrop with
	// a type-icon and tiny OK/Cancel buttons. 12 backdrop/icon variants
	// (backgrnd..backgrnd11 / icon0..icon11) correspond to the
	// different invite types in v83.
	class UIAlarmInvite : public UIDragElement<PosNOTICE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::NOTICE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

		// variant  = which backgrnd/icon pair to use (0..11).
		UIAlarmInvite(std::string message,
			std::function<void(bool yes)> handler,
			int variant = 0);

		void draw(float alpha) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : int16_t { ACCEPT, DECLINE };

		std::function<void(bool yes)> handler;
		Texture backdrop;
		Texture icon;
		Text label;
	};

	// A button-less notice window used for "Waiting for partner..."
	// and similar blocking-but-passive status prompts. The owner has
	// to remove it explicitly via UI::remove(UIElement::Type::NOTICE).
	class UIWaitNotice : public UINotice
	{
	public:
		UIWaitNotice(std::string message);

		void draw(float alpha) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t) override { return Button::State::NORMAL; }
	};

	class UIOk : public UINotice
	{
	public:
		UIOk(std::string message, std::function<void(bool ok)> okhandler);

		void draw(float alpha) const override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : int16_t
		{
			OK
		};

		std::function<void(bool ok)> okhandler;
	};
}