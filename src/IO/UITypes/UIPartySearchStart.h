#pragma once

#include "../UIDragElement.h"
#include "../Components/Textfield.h"

#include "../../Graphics/Text.h"
#include "../../Configuration.h"

#include <cstdint>

namespace ms
{
	// Mini-form for kicking off a Cosmic party search. Backdrop reuses
	// UIWindow2.img/UserList/PopupSettings (260x103) since the v83
	// retail client never shipped a dedicated party-search-start node;
	// the `titleMake` swap on PopupSettings is the closest existing
	// chrome for a recruiting form.
	//
	// Submitting fires PARTY_SEARCH_START (0xE1) with min/max level +
	// member count + job bitmask. Cosmic's PartySearchCoordinator does
	// the rest — auto-pushing partySearchInvite to matching players
	// (handled as the standard party-invite popup elsewhere).
	class UIPartySearchStart : public UIDragElement<PosPARTYSETTINGS>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYSEARCHSTART;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIPartySearchStart();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_START,
			BT_CANCEL
		};

		void dispatch_start();

		Texture title_make;          // "PARTY MAKE" sprite reused as form title
		Textfield min_field;
		Textfield max_field;

		Text min_label;
		Text max_label;
		Text hint_label;
	};
}
