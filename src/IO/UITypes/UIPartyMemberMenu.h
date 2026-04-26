#pragma once

#include "../UIElement.h"
#include "../../Graphics/Texture.h"

#include <cstdint>
#include <string>

namespace ms
{
	// Right-click popup for a party member row. Backdrop is the
	// 3-piece SideMenu strip from
	// UIWindow2.img/UserList/Main/Party/SideMenu, with the seven
	// buttons (Whisper, Friend, Chat, Message, Follow, Master,
	// KickOut). Master + KickOut only show when the local player
	// holds party leader.
	class UIPartyMemberMenu : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYMEMBERMENU;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIPartyMemberMenu(int32_t cid, const std::string& name,
			Point<int16_t> spawn, bool is_leader, bool target_is_self);

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_WHISPER,
			BT_FRIEND,
			BT_CHAT,
			BT_MESSAGE,
			BT_FOLLOW,
			BT_MASTER,
			BT_KICKOUT,
			NUM_BUTTONS
		};

		int32_t target_cid;
		std::string target_name;

		Texture top;
		Texture mid;
		Texture bottom;

		size_t visible_count = 0;
		int16_t mid_h = 0;
		int16_t height = 0;

		static constexpr int16_t WIDTH    = 88;
		static constexpr int16_t TOP_H    = 22;
		static constexpr int16_t BOTTOM_H = 22;
		static constexpr int16_t BUTTON_H = 14;
		static constexpr int16_t BUTTON_X = 4;
	};
}
