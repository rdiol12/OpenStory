#include "UIPartySettings.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPartySettings::UIPartySettings(bool make_mode_)
		: UIDragElement<PosPARTYSETTINGS>(Point<int16_t>(260, 20)),
		  make_mode(make_mode_)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["UserList"]["PopupSettings"];

		sprites.emplace_back(src["backgrnd"]);
		title_make     = Texture(src["titleMake"]);
		title_settings = Texture(src["titleSettings"]);

		buttons[BT_SAVE]   = std::make_unique<MapleButton>(src["BtSave"]);
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);

		dimension = Point<int16_t>(260, 103);
		dragarea  = Point<int16_t>(260, 22);
	}

	void UIPartySettings::draw(float inter) const
	{
		UIElement::draw(inter);

		// Active title sprite — already origin-shifted in NX so we
		// just draw at `position`.
		(make_mode ? title_make : title_settings).draw(position);
	}

	void UIPartySettings::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIPartySettings::get_type() const
	{
		return TYPE;
	}

	Button::State UIPartySettings::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_SAVE:
			// Cosmic v83 has no persisted party settings to write back —
			// retail used this for /Party Search/ visibility + level
			// range, neither of which is server-side configurable here.
			deactivate();
			break;
		case BT_CANCEL:
			deactivate();
			break;
		}
		return Button::State::NORMAL;
	}
}
