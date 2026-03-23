#pragma once

#include "../../Graphics/Texture.h"
#include "../../Constants.h"

#include <random>
#include <vector>

namespace ms
{
	class MapWeather
	{
	public:
		MapWeather();

		void set(nl::node src, const std::string& path, const std::string& message);
		void clear();

		void update();
		void draw(float alpha) const;

		bool is_active() const;

	private:
		enum class Mode
		{
			NONE,
			PARTICLES,
			FOG
		};

		struct Particle
		{
			float x, y;
			float speed;
			float drift;
			float alpha;
			float scale;
		};

		void spawn_particle();

		Mode mode;
		bool active;
		Texture texture;
		std::string msg;
		std::vector<Particle> particles;
		std::mt19937 rng;
		int16_t screen_w;
		int16_t screen_h;

		// Fog state
		float fog_offset;
		float fog_speed;
		float fog_alpha;
	};
}
