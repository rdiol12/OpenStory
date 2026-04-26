#include "UIPartyMemberMenu.h"

#include "UIWhisper.h"
#include "UINotice.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../../Net/Packets/BuddyPackets.h"
#include "../../Net/Packets/GameplayPackets.h"

#include <vector>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPartyMemberMenu::UIPartyMemberMenu(int32_t cid, const std::string& name,
		Point<int16_t> spawn, bool is_leader, bool target_is_self)
		: UIElement(spawn, Point<int16_t>(WIDTH, 0), true),
		  target_cid(cid),
		  target_name(name)
	{
		nl::node menu = nl::nx::ui["UIWindow2.img"]["UserList"]["Main"]["Party"]["SideMenu"];

		top    = menu["top"];
		mid    = menu["center"];
		bottom = menu["bottom"];

		const char* button_names[NUM_BUTTONS] = {
			"BtWhisper", "BtFriend", "BtChat", "BtMessage",
			"BtFollow", "BtMaster", "BtKickOut"
		};

		// Compose the visible button list. Master / KickOut only when
		// local player is leader and target isn't ourselves. Whisper /
		// Friend hide on self.
		std::vector<uint16_t> visible;
		if (!target_is_self)
		{
			visible.push_back(BT_WHISPER);
			visible.push_back(BT_FRIEND);
		}
		visible.push_back(BT_CHAT);
		visible.push_back(BT_MESSAGE);
		if (!target_is_self)
			visible.push_back(BT_FOLLOW);
		if (is_leader && !target_is_self)
		{
			visible.push_back(BT_MASTER);
			visible.push_back(BT_KICKOUT);
		}

		int16_t y = TOP_H;
		for (uint16_t id : visible)
		{
			buttons[id] = std::make_unique<MapleButton>(
				menu[button_names[id]],
				Point<int16_t>(BUTTON_X, y));
			y += BUTTON_H;
		}

		visible_count = visible.size();
		mid_h  = static_cast<int16_t>(visible_count) * BUTTON_H;
		height = TOP_H + mid_h + BOTTOM_H;
		dimension = Point<int16_t>(WIDTH, height);

		position = spawn;
	}

	void UIPartyMemberMenu::draw(float inter) const
	{
		Point<int16_t> p = position;

		top.draw(DrawArgument(p));
		p.shift_y(TOP_H);

		for (size_t row = 0; row < visible_count; row++)
			mid.draw(DrawArgument(p + Point<int16_t>(0, static_cast<int16_t>(row) * BUTTON_H)));

		p.shift_y(mid_h);
		bottom.draw(DrawArgument(p));

		UIElement::draw_buttons(inter);
	}

	Cursor::State UIPartyMemberMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (clicked)
		{
			Rectangle<int16_t> area(position, position + Point<int16_t>(WIDTH, height));
			if (!area.contains(cursorpos))
			{
				deactivate();
				return Cursor::State::IDLE;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	Button::State UIPartyMemberMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_WHISPER:
			UI::get().emplace<UIWhisper>(target_name);
			break;
		case BT_FRIEND:
			AddBuddyPacket(target_name).dispatch();
			break;
		case BT_MASTER:
		{
			int32_t cid = target_cid;
			std::string nm = target_name;
			UI::get().emplace<UIYesNo>(
				"Pass party leader to " + nm + "?",
				[cid](bool yes)
				{
					if (yes) ChangePartyLeaderPacket(cid).dispatch();
				});
			break;
		}
		case BT_KICKOUT:
		{
			int32_t cid = target_cid;
			std::string nm = target_name;
			UI::get().emplace<UIYesNo>(
				"Kick " + nm + " from the party?",
				[cid](bool yes)
				{
					if (yes) ExpelFromPartyPacket(cid).dispatch();
				});
			break;
		}
		case BT_CHAT:
		case BT_MESSAGE:
		case BT_FOLLOW:
			UI::get().emplace<UIOk>(
				"Not implemented on this server.", [](bool) {});
			break;
		}

		deactivate();
		return Button::State::NORMAL;
	}

	UIElement::Type UIPartyMemberMenu::get_type() const
	{
		return TYPE;
	}
}
