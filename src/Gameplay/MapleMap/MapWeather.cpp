#include "MapWeather.h"

#include "../../Graphics/DrawArgument.h"

namespace ms
{
	MapWeather::MapWeather() : active(false), mode(Mode::NONE), fog_offset(0.0f), fog_speed(0.5f), fog_alpha(0.5f), rng(std::random_device{}())
	{
		screen_w = Constants::Constants::get().get_viewwidth();
		screen_h = Constants::Constants::get().get_viewheight();
	}

	void MapWeather::set(nl::node src, const std::string& path, const std::string& message)
	{
		if (!src)
			return;

		// Weather nodes contain numbered frames (0, 1, 2...) — use frame 0 as the particle texture
		nl::node frame = src["0"];

		if (!frame)
			frame = src;

		texture = Texture(frame);
		msg = message;
		active = true;

		// Detect fog vs particles based on path
		bool is_fog = path.find("ghost") != std::string::npos
			|| path.find("fog") != std::string::npos
			|| path.find("mist") != std::string::npos;

		if (is_fog)
		{
			mode = Mode::FOG;
			fog_offset = 0.0f;
			fog_speed = 0.5f;
			fog_alpha = 0.45f;
			particles.clear();
		}
		else
		{
			mode = Mode::PARTICLES;
			fog_offset = 0.0f;
			particles.clear();

			for (int i = 0; i < 120; i++)
				spawn_particle();

			// Spread them vertically so they don't all start at the top
			std::uniform_real_distribution<float> ydist(0.0f, static_cast<float>(screen_h));

			for (auto& p : particles)
				p.y = ydist(rng);
		}
	}

	void MapWeather::clear()
	{
		active = false;
		mode = Mode::NONE;
		particles.clear();
	}

	void MapWeather::spawn_particle()
	{
		std::uniform_real_distribution<float> xdist(0.0f, static_cast<float>(screen_w));
		std::uniform_real_distribution<float> speeddist(0.3f, 1.0f);
		std::uniform_real_distribution<float> driftdist(-0.3f, 0.3f);
		std::uniform_real_distribution<float> alphadist(0.4f, 0.9f);
		std::uniform_real_distribution<float> scaledist(0.3f, 0.6f);

		Particle p;
		p.x = xdist(rng);
		p.y = -20.0f;
		p.speed = speeddist(rng);
		p.drift = driftdist(rng);
		p.alpha = alphadist(rng);
		p.scale = scaledist(rng);

		particles.push_back(p);
	}

	void MapWeather::update()
	{
		if (!active)
			return;

		if (mode == Mode::FOG)
		{
			fog_offset += fog_speed;

			Point<int16_t> dims = texture.get_dimensions();

			if (dims.x() > 0 && fog_offset >= static_cast<float>(dims.x()))
				fog_offset -= static_cast<float>(dims.x());
		}
		else if (mode == Mode::PARTICLES)
		{
			for (auto& p : particles)
			{
				p.y += p.speed;
				p.x += p.drift;
			}

			particles.erase(
				std::remove_if(particles.begin(), particles.end(),
					[this](const Particle& p) { return p.y > screen_h + 20 || p.x < -20 || p.x > screen_w + 20; }),
				particles.end()
			);

			while (particles.size() < 120)
				spawn_particle();
		}
	}

	void MapWeather::draw(float alpha) const
	{
		if (!active)
			return;

		if (mode == Mode::FOG)
		{
			Point<int16_t> dims = texture.get_dimensions();

			if (dims.x() <= 0 || dims.y() <= 0)
				return;

			int16_t tex_w = dims.x();
			int16_t tex_h = dims.y();
			int16_t start_x = -static_cast<int16_t>(fog_offset);

			// Tile horizontally across the screen
			for (int16_t x = start_x; x < screen_w; x += tex_w)
			{
				// Tile vertically to cover screen height
				for (int16_t y = 0; y < screen_h; y += tex_h)
				{
					Point<int16_t> pos(x, y);
					texture.draw(DrawArgument(pos, fog_alpha * alpha));
				}
			}
		}
		else if (mode == Mode::PARTICLES)
		{
			for (const auto& p : particles)
			{
				Point<int16_t> pos(static_cast<int16_t>(p.x), static_cast<int16_t>(p.y));
				texture.draw(DrawArgument(pos, p.scale, p.scale, p.alpha * alpha));
			}
		}
	}

	bool MapWeather::is_active() const
	{
		return active;
	}
}
