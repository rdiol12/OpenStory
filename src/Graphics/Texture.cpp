//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "Texture.h"

#include "GraphicsGL.h"

#include <unordered_map>

#include <stb_image.h>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		struct RawImage
		{
			size_t id = 0;
			int16_t width = 0;
			int16_t height = 0;
			std::shared_ptr<std::vector<uint8_t>> pixels;
		};

		// Synthetic ids from a high range so they never collide with NX bitmap ids
		size_t next_raw_id = size_t(1) << 48;

		// Path → loaded image. Keeps atlas ids stable across reloads and keeps the
		// pixel buffers alive for atlas re-uploads. Only successful loads are
		// cached, so a file dropped in later is picked up on the next load attempt.
		std::unordered_map<std::string, RawImage>& raw_registry()
		{
			static std::unordered_map<std::string, RawImage> registry;

			return registry;
		}

		const RawImage* load_raw(const std::string& path)
		{
			auto& registry = raw_registry();
			auto iter = registry.find(path);

			if (iter != registry.end())
				return &iter->second;

			int width = 0;
			int height = 0;
			int channels = 0;
			stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

			if (!data)
				return nullptr;

			// stb gives RGBA, the atlas uploads BGRA — swap R and B
			auto pixels = std::make_shared<std::vector<uint8_t>>(data, data + width * height * 4);
			stbi_image_free(data);

			for (size_t i = 0; i < pixels->size(); i += 4)
				std::swap((*pixels)[i], (*pixels)[i + 2]);

			RawImage image;
			image.id = next_raw_id++;
			image.width = static_cast<int16_t>(width);
			image.height = static_cast<int16_t>(height);
			image.pixels = std::move(pixels);

			return &registry.emplace(path, std::move(image)).first->second;
		}
	}
	Texture::Texture(nl::node src)
	{
		if (src.data_type() == nl::node::type::bitmap)
		{
			origin = src["origin"];

			if (src.root() == nl::nx::map001)
			{
				const std::string _outlink = (std::string)src["_outlink"];

				if (!_outlink.empty())
				{
					size_t first = _outlink.find_first_of('/');

					if (first != std::string::npos)
					{
						const std::string& first_part = _outlink.substr(0, first);

						if (first_part == "Map")
						{
							const std::string& path = _outlink.substr(first + 1);
							nl::node foundOutlink = nl::nx::mapLatest.resolve(path);

							if (foundOutlink)
								src = foundOutlink;
						}
					}
				}
			}

			bitmap = src;
			dimensions = Point<int16_t>(bitmap.width(), bitmap.height());

			GraphicsGL::get().addbitmap(bitmap);
		}
	}

	void Texture::draw(const DrawArgument& args) const
	{
		draw(args, Range<int16_t>(0, 0));
	}

	Texture Texture::from_file(const std::string& path, Point<int16_t> origin)
	{
		Texture texture;

		if (const RawImage* image = load_raw(path))
		{
			texture.rawid = image->id;
			texture.rawpixels = image->pixels;
			texture.dimensions = Point<int16_t>(image->width, image->height);
			texture.origin = origin;
		}

		return texture;
	}

	Texture Texture::from_pixels(const std::string& key, int16_t width, int16_t height, std::vector<uint8_t> bgra, Point<int16_t> origin)
	{
		Texture texture;

		if (width <= 0 || height <= 0 || bgra.size() != size_t(width) * height * 4)
			return texture;

		auto& registry = raw_registry();
		auto iter = registry.find(key);

		if (iter == registry.end())
		{
			RawImage image;
			image.id = next_raw_id++;
			image.width = width;
			image.height = height;
			image.pixels = std::make_shared<std::vector<uint8_t>>(std::move(bgra));

			iter = registry.emplace(key, std::move(image)).first;
		}

		texture.rawid = iter->second.id;
		texture.rawpixels = iter->second.pixels;
		texture.dimensions = Point<int16_t>(iter->second.width, iter->second.height);
		texture.origin = origin;

		return texture;
	}

	void Texture::draw(const DrawArgument& args, const Range<int16_t>& vertical) const
	{
		if (rawid > 0)
		{
			GraphicsGL::get().drawraw(
				rawid,
				dimensions.x(),
				dimensions.y(),
				rawpixels->data(),
				args.get_rectangle(origin, dimensions),
				vertical,
				args.get_color(),
				args.get_angle()
			);

			return;
		}

		if (!is_valid())
			return;

		GraphicsGL::get().draw(
			bitmap,
			args.get_rectangle(origin, dimensions),
			vertical,
			args.get_color(),
			args.get_angle()
		);
	}

	void Texture::shift(Point<int16_t> amount)
	{
		origin -= amount;
	}

	bool Texture::is_valid() const
	{
		return bitmap.id() > 0 || rawid > 0;
	}

	int16_t Texture::width() const
	{
		return dimensions.x();
	}

	int16_t Texture::height() const
	{
		return dimensions.y();
	}

	Point<int16_t> Texture::get_origin() const
	{
		return origin;
	}

	Point<int16_t> Texture::get_dimensions() const
	{
		return dimensions;
	}
}