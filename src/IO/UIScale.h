#pragma once

#include "../Constants.h"
#include "../Graphics/DrawArgument.h"

namespace ms
{
	struct UIScale
	{
		static constexpr int16_t BASE_WIDTH = 800;
		static constexpr int16_t BASE_HEIGHT = 600;

		static inline int16_t view_width()
		{
			return Constants::Constants::get().get_viewwidth();
		}

		static inline int16_t view_height()
		{
			return Constants::Constants::get().get_viewheight();
		}

		static inline float scale_x()
		{
			return (float)view_width() / (float)BASE_WIDTH;
		}

		static inline float scale_y()
		{
			return (float)view_height() / (float)BASE_HEIGHT;
		}

		static inline Point<int16_t> content_offset()
		{
			return Point<int16_t>((view_width() - BASE_WIDTH) / 2, (view_height() - BASE_HEIGHT) / 2);
		}

		// Background args for sprites that need scaling to fill the screen.
		// Uses 800x600 center so CENTER_OFFSET draw_sprites can add the offset correctly.
		static inline DrawArgument bg_args()
		{
			return DrawArgument(Point<int16_t>(BASE_WIDTH / 2, BASE_HEIGHT / 2), scale_x(), scale_y());
		}

		static inline Point<int16_t> at(int16_t x, int16_t y)
		{
			return Point<int16_t>(x, y) + content_offset();
		}

		static inline Point<int16_t> at(Point<int16_t> pos)
		{
			return pos + content_offset();
		}
	};
}
