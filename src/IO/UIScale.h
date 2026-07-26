#pragma once

#include "../Constants.h"
#include "../Graphics/DrawArgument.h"
#include "../Graphics/Texture.h"

#include <cmath>

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

		// Exact screen rect for a texture stretched from logical 800x600 space:
		// both edges rounded, so adjacent pieces share edges with no seams.
		static inline DrawArgument stretch_args(const Texture& t, int16_t x, int16_t y)
		{
			Point<int16_t> o = t.get_origin();
			Point<int16_t> d = t.get_dimensions();
			int32_t lx = x - o.x();
			int32_t ly = y - o.y();
			int16_t sl = static_cast<int16_t>(std::lround(lx * scale_x()));
			int16_t st = static_cast<int16_t>(std::lround(ly * scale_y()));
			int16_t sr = static_cast<int16_t>(std::lround((lx + d.x()) * scale_x()));
			int16_t sb = static_cast<int16_t>(std::lround((ly + d.y()) * scale_y()));

			return DrawArgument(Point<int16_t>(sl, st) + o, Point<int16_t>(static_cast<int16_t>(sr - sl), static_cast<int16_t>(sb - st)));
		}

		// Uniform-scale point anchored at a screen point: offsets measured from
		// a reference point, rounded in uniform space.
		static inline Point<int16_t> uniform_point(Point<int16_t> anchor, Point<int16_t> ref, int16_t x, int16_t y, float u)
		{
			return Point<int16_t>(
				static_cast<int16_t>(anchor.x() + std::lround((x - ref.x()) * u)),
				static_cast<int16_t>(anchor.y() + std::lround((y - ref.y()) * u)));
		}

		// Uniform-scale rect anchored at a screen point: both edges rounded from
		// the reference point so stacked pieces stay seam-free, origin preserved.
		static inline DrawArgument uniform_args(const Texture& t, Point<int16_t> anchor, Point<int16_t> ref, int16_t x, int16_t y, float u)
		{
			Point<int16_t> o = t.get_origin();
			Point<int16_t> d = t.get_dimensions();
			int32_t lx = x - ref.x() - o.x();
			int32_t ly = y - ref.y() - o.y();
			int16_t sl = static_cast<int16_t>(anchor.x() + std::lround(lx * u));
			int16_t st = static_cast<int16_t>(anchor.y() + std::lround(ly * u));
			int16_t sr = static_cast<int16_t>(anchor.x() + std::lround((lx + d.x()) * u));
			int16_t sb = static_cast<int16_t>(anchor.y() + std::lround((ly + d.y()) * u));

			return DrawArgument(Point<int16_t>(sl, st) + o, Point<int16_t>(static_cast<int16_t>(sr - sl), static_cast<int16_t>(sb - st)));
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
