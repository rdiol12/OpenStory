//////////////////////////////////////////////////////////////////////////////////
//	Loads a character effect/aura node from the NX and renders it with the		//
//	correct layering, handling the different structures effects use:			//
//	  - direct frames:      <id>/{0,1,2,...} (bitmaps)							//
//	  - z-layer groups:     <id>/0/{frames}, <id>/1/{frames}					//
//	  - per-frame layers:   <id>/<frame>/{0=back, 1=front}						//
//	Layer 0 draws behind the character, layer 1 in front, so the aura wraps.		//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Animation.h"

#ifdef USE_NX
#include <nlnx/node.hpp>
#endif

namespace ms
{
	class CharacterAura
	{
	public:
		// Build from an effect node. An empty/invalid node clears the aura.
		void load(nl::node effect);
		void clear();

		bool is_active() const { return active; }

		void update();
		// Behind the character.
		void draw_back(const DrawArgument& args, float alpha) const;
		// In front of the character.
		void draw_front(const DrawArgument& args, float alpha) const;

	private:
		Animation back;
		Animation front;
		bool active = false;
		bool has_back = false;
	};
}
