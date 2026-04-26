#pragma once

#include "../UIDragElement.h"

#include "../../Graphics/Text.h"
#include "../../Configuration.h"

#include <cstdint>

namespace ms
{
	// Party settings popup — UIWindow2.img/UserList/PopupSettings
	// (260x103). Two title sprites are baked into the dialog, one
	// for "Make" (creating a party) and one for "Settings" (editing
	// an existing one); we pick the right one at construction.
	class UIPartySettings : public UIDragElement<PosPARTYSETTINGS>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYSETTINGS;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIPartySettings(bool make_mode);

		void draw(float inter) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_SAVE,
			BT_CANCEL
		};

		bool make_mode;
		Texture title_make;
		Texture title_settings;
	};
}
