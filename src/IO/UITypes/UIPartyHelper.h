#pragma once

#include "../UIDragElement.h"

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include "../../Configuration.h"

#include <cstdint>
#include <vector>

namespace ms
{
	// Persistent quest-helper-style side panel that lists current
	// party member names. Reuses the QuestAlarm chrome
	// (UIWindow.img/QuestAlarm/backgrndmin/center/bottom) so the
	// layout reads as a sibling of the Quest Helper. Hides itself
	// while not in a party.
	class UIPartyHelper : public UIDragElement<PosPARTYHELPER>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYHELPER;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIPartyHelper();

		void draw(float inter) const override;

		UIElement::Type get_type() const override;

	private:
		Texture bg_min;
		Texture bg_center;
		Texture bg_bottom;

		mutable Text title;
		mutable Text row_label;
	};
}
