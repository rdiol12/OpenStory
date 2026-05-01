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
#include "GraphicsGL.h"

#include "../Configuration.h"

#include FT_BITMAP_H

namespace
{
	// Decodes one UTF-8 codepoint starting at `text[i]`. Sets `consumed` to
	// the byte count (1-4). Falls back to the raw byte on malformed input.
	inline uint32_t utf8_decode(const char* text, size_t length, size_t i, size_t& consumed)
	{
		if (i >= length) { consumed = 0; return 0; }
		uint8_t b0 = static_cast<uint8_t>(text[i]);
		if (b0 < 0x80) { consumed = 1; return b0; }

		auto cont = [&](size_t j) -> uint8_t { return j < length ? static_cast<uint8_t>(text[j]) : 0; };

		if ((b0 & 0xE0) == 0xC0 && i + 1 < length)
		{
			uint8_t b1 = cont(i + 1);
			if ((b1 & 0xC0) == 0x80) { consumed = 2; return ((b0 & 0x1F) << 6) | (b1 & 0x3F); }
		}
		else if ((b0 & 0xF0) == 0xE0 && i + 2 < length)
		{
			uint8_t b1 = cont(i + 1), b2 = cont(i + 2);
			if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80)
			{
				consumed = 3;
				return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
			}
		}
		else if ((b0 & 0xF8) == 0xF0 && i + 3 < length)
		{
			uint8_t b1 = cont(i + 1), b2 = cont(i + 2), b3 = cont(i + 3);
			if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80)
			{
				consumed = 4;
				return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
			}
		}
		consumed = 1;
		return b0;
	}

	inline bool is_emoji_codepoint(uint32_t cp)
	{
		return (cp >= 0x1F300 && cp <= 0x1FAFF)
			|| (cp >= 0x2600  && cp <= 0x27BF)
			|| (cp == 0x2764) || (cp == 0x2728);
	}
}

#ifdef PLATFORM_IOS
// Defined in FontPathIOS.mm
extern const char* ios_font_path_normal();
extern const char* ios_font_path_bold();
#endif

namespace ms
{
	GraphicsGL::GraphicsGL()
	{
		locked = false;

		VWIDTH = Constants::Constants::get().get_viewwidth();
		VHEIGHT = Constants::Constants::get().get_viewheight();
		SCREEN = Rectangle<int16_t>(0, VWIDTH, 0, VHEIGHT);
	}

	Error GraphicsGL::init()
	{
#ifdef PLATFORM_IOS
		// OpenGL ES 3.0 shaders
		const char* vertexShaderSource =
			"#version 300 es\n"
			"in vec4 coord;\n"
			"in vec4 color;\n"
			"out vec2 texpos;\n"
			"out vec4 colormod;\n"
			"uniform vec2 screensize;\n"
			"uniform int yoffset;\n"
			"\n"
			"void main(void)\n"
			"{\n"
			"	float x = -1.0 + coord.x * 2.0 / screensize.x;\n"
			"	float y = 1.0 - (coord.y + float(yoffset)) * 2.0 / screensize.y;\n"
			"	gl_Position = vec4(x, y, 0.0, 1.0);\n"
			"	texpos = coord.zw;\n"
			"	colormod = color;\n"
			"}\n";

		const char* fragmentShaderSource =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 texpos;\n"
			"in vec4 colormod;\n"
			"out vec4 fragColor;\n"
			"uniform sampler2D tex;\n"
			"uniform vec2 atlassize;\n"
			"uniform int fontregion;\n"
			"\n"
			"void main(void)\n"
			"{\n"
			"	if (texpos.y == 0.0)\n"
			"	{\n"
			"		fragColor = colormod;\n"
			"	}\n"
			"	else if (texpos.y <= float(fontregion))\n"
			"	{\n"
			"		fragColor = vec4(1.0, 1.0, 1.0, texture(tex, texpos / atlassize).r) * colormod;\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		fragColor = texture(tex, texpos / atlassize) * colormod;\n"
			"	}\n"
			"}\n";

		const char* uniformTexName = "tex";
#else
		const char* vertexShaderSource =
			"#version 120\n"
			"attribute vec4 coord;"
			"attribute vec4 color;"
			"varying vec2 texpos;"
			"varying vec4 colormod;"
			"uniform vec2 screensize;"
			"uniform int yoffset;"

			"void main(void)"
			"{"
			"	float x = -1.0 + coord.x * 2.0 / screensize.x;"
			"	float y = 1.0 - (coord.y + yoffset) * 2.0 / screensize.y;"
			"   gl_Position = vec4(x, y, 0.0, 1.0);"
			"	texpos = coord.zw;"
			"	colormod = color;"
			"}";

		const char* fragmentShaderSource =
			"#version 120\n"
			"varying vec2 texpos;"
			"varying vec4 colormod;"
			"uniform sampler2D texture;"
			"uniform vec2 atlassize;"
			"uniform int fontregion;"

			"void main(void)"
			"{"
			"	if (texpos.y == 0)"
			"	{"
			"		gl_FragColor = colormod;"
			"	}"
			"	else if (texpos.y <= fontregion)"
			"	{"
			"		gl_FragColor = vec4(1, 1, 1, texture2D(texture, texpos / atlassize).r) * colormod;"
			"	}"
			"	else"
			"	{"
			"		gl_FragColor = texture2D(texture, texpos / atlassize) * colormod;"
			"	}"
			"}";

		const char* uniformTexName = "texture";
#endif

		const GLsizei bufSize = 512;

		GLint success;
		GLchar infoLog[bufSize];

#ifndef PLATFORM_IOS
		if (GLenum error = glewInit())
			return Error(Error::Code::GLEW, (const char*)glewGetErrorString(error));
#endif

		if (FT_Init_FreeType(&ftlibrary))
			return Error::Code::FREETYPE;

		FT_Int ftmajor;
		FT_Int ftminor;
		FT_Int ftpatch;

		FT_Library_Version(ftlibrary, &ftmajor, &ftminor, &ftpatch);

		// Vertex Shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);

		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

		if (success != GL_TRUE)
		{
			glGetShaderInfoLog(vertexShader, bufSize, NULL, infoLog);
			return Error(Error::Code::VERTEX_SHADER, infoLog);
		}

		// Fragment Shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

		if (success != GL_TRUE)
		{
			glGetShaderInfoLog(fragmentShader, bufSize, NULL, infoLog);
			return Error(Error::Code::FRAGMENT_SHADER, infoLog);
		}

		// Link Shaders
		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

		if (success != GL_TRUE)
		{
			glGetProgramInfoLog(shaderProgram, bufSize, NULL, infoLog);
			return Error(Error::Code::SHADER_PROGRAM_LINK, infoLog);
		}

		glValidateProgram(shaderProgram);

		glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);

		if (success != GL_TRUE)
		{
			glGetProgramInfoLog(shaderProgram, bufSize, NULL, infoLog);
			return Error(Error::Code::SHADER_PROGRAM_VALID, infoLog);
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		attribute_coord = glGetAttribLocation(shaderProgram, "coord");
		attribute_color = glGetAttribLocation(shaderProgram, "color");
		uniform_texture = glGetUniformLocation(shaderProgram, uniformTexName);
		uniform_atlassize = glGetUniformLocation(shaderProgram, "atlassize");
		uniform_screensize = glGetUniformLocation(shaderProgram, "screensize");
		uniform_yoffset = glGetUniformLocation(shaderProgram, "yoffset");
		uniform_fontregion = glGetUniformLocation(shaderProgram, "fontregion");

		if (attribute_coord == -1 || attribute_color == -1 || uniform_texture == -1 || uniform_atlassize == -1 || uniform_screensize == -1 || uniform_yoffset == -1)
			return Error::Code::SHADER_VARS;

#ifdef PLATFORM_IOS
		// VAO required by OpenGL ES 3.0
		GLuint VAO;
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
#endif

		// Vertex Buffer Object
		glGenBuffers(1, &VBO);

		glGenTextures(1, &atlas);
		glBindTexture(GL_TEXTURE_2D, atlas);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLASW, ATLASH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		fontborder.set_y(1);

#ifdef PLATFORM_IOS
		const char* FONT_NORMAL_STR = ios_font_path_normal();
		const char* FONT_BOLD_STR = ios_font_path_bold();

		if (!FONT_NORMAL_STR || !FONT_BOLD_STR)
			return Error::Code::FONT_PATH;
#else
		const std::string FONT_NORMAL = Setting<FontPathNormal>().get().load();
		const std::string FONT_BOLD = Setting<FontPathBold>().get().load();

		if (FONT_NORMAL.empty() || FONT_BOLD.empty())
			return Error::Code::FONT_PATH;

		const char* FONT_NORMAL_STR = FONT_NORMAL.c_str();
		const char* FONT_BOLD_STR = FONT_BOLD.c_str();
#endif

		addfont(FONT_NORMAL_STR, Text::Font::A11M, 0, 11);
		addfont(FONT_BOLD_STR, Text::Font::A11B, 0, 11);
		addfont(FONT_NORMAL_STR, Text::Font::A12M, 0, 12);
		addfont(FONT_BOLD_STR, Text::Font::A12B, 0, 12);
		addfont(FONT_NORMAL_STR, Text::Font::A13M, 0, 13);
		addfont(FONT_BOLD_STR, Text::Font::A13B, 0, 13);
		addfont(FONT_BOLD_STR, Text::Font::A14B, 0, 14);
		addfont(FONT_BOLD_STR, Text::Font::A15B, 0, 15);
		addfont(FONT_NORMAL_STR, Text::Font::A18M, 0, 18);

		fontymax += fontborder.y();

#ifndef PLATFORM_IOS
		const std::string FONT_CJK = Setting<FontPathCJK>().get().load();
		cjk_fallback_path = FONT_CJK;

		const std::string FONT_EMOJI = Setting<FontPathEmoji>().get().load();
		if (!FONT_EMOJI.empty())
		{
			if (FT_New_Face(ftlibrary, FONT_EMOJI.c_str(), 0, &emojiface) == 0)
			{
				if (emojiface->num_fixed_sizes > 0)
				{
					int best = 0;
					for (int i = 1; i < emojiface->num_fixed_sizes; ++i)
					{
						if (std::abs(emojiface->available_sizes[i].height - 16)
							< std::abs(emojiface->available_sizes[best].height - 16))
							best = i;
					}
					if (FT_Select_Size(emojiface, best) != 0)
					{
						FT_Done_Face(emojiface);
						emojiface = nullptr;
					}
					else
					{
						emoji_strike_size = emojiface->available_sizes[best].height;
					}
				}
				else
				{
					// Modern emoji fonts (e.g. Windows Segoe UI Emoji on Win10+)
					// are COLR/COLRv1 vector fonts with no bitmap strikes.
					// Use FT_Set_Pixel_Sizes so FT_LOAD_COLOR can rasterize
					// the colored layers into a BGRA bitmap.
					emoji_strike_size = 16;
					if (FT_Set_Pixel_Sizes(emojiface, 0, emoji_strike_size) != 0)
					{
						FT_Done_Face(emojiface);
						emojiface = nullptr;
					}
				}
			}
			else
			{
				emojiface = nullptr;
			}
		}
#endif

		leftovers = QuadTree<size_t, Leftover>(
			[](const Leftover& first, const Leftover& second)
			{
				bool width_comparison = first.width() >= second.width();
				bool height_comparison = first.height() >= second.height();

				if (width_comparison && height_comparison)
					return QuadTree<size_t, Leftover>::Direction::RIGHT;
				else if (width_comparison)
					return QuadTree<size_t, Leftover>::Direction::DOWN;
				else if (height_comparison)
					return QuadTree<size_t, Leftover>::Direction::UP;
				else
					return QuadTree<size_t, Leftover>::Direction::LEFT;
			}
		);

		quads.reserve(4096);

		return Error::Code::NONE;
	}

	bool GraphicsGL::addfont(const char* name, Text::Font id, FT_UInt pixelw, FT_UInt pixelh)
	{
		FT_Face face;

		if (FT_New_Face(ftlibrary, name, 0, &face))
			return false;

		if (FT_Set_Pixel_Sizes(face, pixelw, pixelh))
			return false;

		FT_GlyphSlot g = face->glyph;

		GLshort width = 0;
		GLshort height = 0;

		for (uint8_t c = 32; c < 128; c++)
		{
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
				continue;

			GLshort w = static_cast<GLshort>(g->bitmap.width);
			GLshort h = static_cast<GLshort>(g->bitmap.rows);

			width += w;

			if (h > height)
				height = h;
		}

		if (fontborder.x() + width > ATLASW)
		{
			fontborder.set_x(0);
			fontborder.set_y(fontymax);
			fontymax = 0;
		}

		GLshort x = fontborder.x();
		GLshort y = fontborder.y();

		fontborder.shift_x(width);

		if (height > fontymax)
			fontymax = height;

		fonts[id] = Font(width, height);
		fonts[id].path = name;
		fonts[id].pixelw = pixelw;
		fonts[id].pixelh = pixelh;

		GLshort ox = x;
		GLshort oy = y;

		for (uint8_t c = 32; c < 128; c++)
		{
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
				continue;

			GLshort ax = static_cast<GLshort>(g->advance.x >> 6);
			GLshort ay = static_cast<GLshort>(g->advance.y >> 6);
			GLshort l = static_cast<GLshort>(g->bitmap_left);
			GLshort t = static_cast<GLshort>(g->bitmap_top);
			GLshort w = static_cast<GLshort>(g->bitmap.width);
			GLshort h = static_cast<GLshort>(g->bitmap.rows);

			glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, w, h, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

			Offset offset = Offset(ox, oy, w, h);
			Font::Char ch = { ax, ay, w, h, l, t, offset, false };
			fonts[id].chars.emplace(static_cast<uint32_t>(c), ch);

			ox += w;
		}

		FT_Done_Face(face);
		return true;
	}

	bool GraphicsGL::load_mono_glyph(Font& font, uint32_t codepoint)
	{
		if (font.path.empty()) return false;

		FT_Face face;
		const char* loaded_path = font.path.c_str();
		if (FT_New_Face(ftlibrary, font.path.c_str(), 0, &face))
			return false;

		// If the primary font has no glyph for this codepoint, try the CJK
		// fallback face (e.g. malgun.ttf for Korean). This covers MapleStory
		// v83 quest/NPC names which are often in Korean.
		if (FT_Get_Char_Index(face, codepoint) == 0 && !cjk_fallback_path.empty())
		{
			FT_Face cjkface;
			if (FT_New_Face(ftlibrary, cjk_fallback_path.c_str(), 0, &cjkface) == 0)
			{
				if (FT_Get_Char_Index(cjkface, codepoint) != 0)
				{
					FT_Done_Face(face);
					face = cjkface;
					loaded_path = cjk_fallback_path.c_str();
				}
				else
				{
					FT_Done_Face(cjkface);
				}
			}
		}

		if (FT_Set_Pixel_Sizes(face, font.pixelw, font.pixelh))
		{
			FT_Done_Face(face);
			return false;
		}

		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER))
		{
			FT_Done_Face(face);
			return false;
		}
		(void)loaded_path;

		FT_GlyphSlot g = face->glyph;
		GLshort w = static_cast<GLshort>(g->bitmap.width);
		GLshort h = static_cast<GLshort>(g->bitmap.rows);

		if (fontborder.x() + w > ATLASW)
		{
			fontborder.set_x(0);
			fontborder.set_y(fontymax);
			fontymax = 0;
		}

		GLshort ox = fontborder.x();
		GLshort oy = fontborder.y();

		fontborder.shift_x(w > 0 ? w : 1);

		if (h > fontymax) fontymax = h;

		if (w > 0 && h > 0)
			glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, w, h, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		Font::Char ch;
		ch.ax = static_cast<GLshort>(g->advance.x >> 6);
		ch.ay = static_cast<GLshort>(g->advance.y >> 6);
		ch.bw = w;
		ch.bh = h;
		ch.bl = static_cast<GLshort>(g->bitmap_left);
		ch.bt = static_cast<GLshort>(g->bitmap_top);
		ch.offset = Offset(ox, oy, w, h);
		ch.color = false;
		font.chars.emplace(codepoint, ch);

		FT_Done_Face(face);
		return true;
	}

	bool GraphicsGL::load_emoji_glyph(Font& font, uint32_t codepoint)
	{
		if (emojiface == nullptr) return false;

		FT_UInt gidx = FT_Get_Char_Index(emojiface, codepoint);
		if (gidx == 0)
			return false;

		if (FT_Load_Char(emojiface, codepoint, FT_LOAD_COLOR | FT_LOAD_RENDER))
			return false;

		FT_GlyphSlot g = emojiface->glyph;

		GLshort strike_w = static_cast<GLshort>(g->bitmap.width);
		GLshort strike_h = static_cast<GLshort>(g->bitmap.rows);
		if (strike_w <= 0 || strike_h <= 0)
			return false;

		// Convert bitmap to BGRA. COLRv1 color rendering isn't always available
		// in FreeType builds, so FT_LOAD_COLOR may still return FT_PIXEL_MODE_GRAY
		// (plain grayscale outline). In that case we up-convert to BGRA so the
		// emoji renders as a black silhouette rather than a placeholder box.
		std::vector<uint8_t> rgba_buf;
		const uint8_t* src_pixels = g->bitmap.buffer;

		if (g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
		{
			rgba_buf.resize(static_cast<size_t>(strike_w) * strike_h * 4);
			for (int y = 0; y < strike_h; ++y)
			{
				const uint8_t* row = g->bitmap.buffer + y * g->bitmap.pitch;
				uint8_t* dst = rgba_buf.data() + y * strike_w * 4;
				for (int x = 0; x < strike_w; ++x)
				{
					uint8_t a = row[x];
					dst[x * 4 + 0] = 0;   // B
					dst[x * 4 + 1] = 0;   // G
					dst[x * 4 + 2] = 0;   // R
					dst[x * 4 + 3] = a;   // A
				}
			}
			src_pixels = rgba_buf.data();
		}
		else if (g->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA)
			return false;

		GLshort target = static_cast<GLshort>(font.height > 0 ? font.height + 2 : 14);

		Leftover value = Leftover(0, 0, strike_w, strike_h);
		GLshort gx = 0, gy = 0;

		size_t lid = leftovers.findnode(value,
			[](const Leftover& val, const Leftover& leaf) {
				return val.width() <= leaf.width() && val.height() <= leaf.height();
			});

		if (lid > 0)
		{
			const Leftover& lo = leftovers[lid];
			gx = lo.left;
			gy = lo.top;
			leftovers.erase(lid);
		}
		else
		{
			if (border.x() + strike_w > ATLASW)
			{
				border.set_x(0);
				border.shift_y(yrange.second());
				if (border.y() + strike_h > ATLASH) return false;
				yrange = Range<GLshort>();
			}
			gx = border.x();
			gy = border.y();
			border.shift_x(strike_w);
			if (strike_h > yrange.second())
				yrange = Range<int16_t>(gy + strike_h, strike_h);
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, gx, gy, strike_w, strike_h, GL_BGRA, GL_UNSIGNED_BYTE, src_pixels);

		Font::Char ch;
		ch.ax = target;
		ch.ay = 0;
		ch.bw = target;
		ch.bh = target;
		ch.bl = 0;
		ch.bt = static_cast<GLshort>(font.height);
		ch.offset = Offset(gx, gy, strike_w, strike_h);
		ch.color = true;
		font.chars.emplace(codepoint, ch);
		return true;
	}

	GraphicsGL::Font::Char GraphicsGL::ensure_glyph(Text::Font fontid, uint32_t codepoint)
	{
		Font& font = fonts[fontid];

		auto it = font.chars.find(codepoint);
		if (it != font.chars.end()) return it->second;

		if (is_emoji_codepoint(codepoint) && load_emoji_glyph(font, codepoint))
		{
			it = font.chars.find(codepoint);
			if (it != font.chars.end()) return it->second;
		}

		if (load_mono_glyph(font, codepoint))
		{
			it = font.chars.find(codepoint);
			if (it != font.chars.end()) return it->second;
		}

		Font::Char placeholder{};
		font.chars.emplace(codepoint, placeholder);
		return placeholder;
	}

	void GraphicsGL::reinit()
	{
		int32_t new_width = Constants::Constants::get().get_viewwidth();
		int32_t new_height = Constants::Constants::get().get_viewheight();

		if (VWIDTH != new_width || VHEIGHT != new_height)
		{
			VWIDTH = new_width;
			VHEIGHT = new_height;
			SCREEN = Rectangle<int16_t>(0, VWIDTH, 0, VHEIGHT);
		}

		glUseProgram(shaderProgram);

		glUniform1i(uniform_fontregion, fontymax);
		glUniform2f(uniform_atlassize, ATLASW, ATLASH);
		glUniform2f(uniform_screensize, VWIDTH, VHEIGHT);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glVertexAttribPointer(attribute_coord, 4, GL_SHORT, GL_FALSE, sizeof(Quad::Vertex), 0);
		glVertexAttribPointer(attribute_color, 4, GL_FLOAT, GL_FALSE, sizeof(Quad::Vertex), (const void*)8);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, atlas);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		clearinternal();
	}

	void GraphicsGL::clearinternal()
	{
		border = Point<GLshort>(0, fontymax);
		yrange = Range<GLshort>();

		offsets.clear();
		leftovers.clear();
		rlid = 1;
		wasted = 0;
	}

	void GraphicsGL::clear()
	{
		size_t used = ATLASW * border.y() + border.x() * yrange.second();
		double usedpercent = static_cast<double>(used) / (ATLASW * ATLASH);

		if (usedpercent > 80.0)
			clearinternal();
	}

	void GraphicsGL::addbitmap(const nl::bitmap& bmp)
	{
		getoffset(bmp);
	}

	const GraphicsGL::Offset& GraphicsGL::getoffset(const nl::bitmap& bmp)
	{
		size_t id = bmp.id();
		auto offiter = offsets.find(id);

		if (offiter != offsets.end())
			return offiter->second;

		GLshort x = 0;
		GLshort y = 0;
		GLshort width = bmp.width();
		GLshort height = bmp.height();

		if (width <= 0 || height <= 0)
			return nulloffset;

		Leftover value = Leftover(x, y, width, height);

		size_t lid = leftovers.findnode(
			value,
			[](const Leftover& val, const Leftover& leaf)
			{
				return val.width() <= leaf.width() && val.height() <= leaf.height();
			}
		);

		if (lid > 0)
		{
			const Leftover& leftover = leftovers[lid];

			x = leftover.left;
			y = leftover.top;

			GLshort width_delta = leftover.width() - width;
			GLshort height_delta = leftover.height() - height;

			leftovers.erase(lid);

			wasted -= width * height;

			if (width_delta >= MINLOSIZE && height_delta >= MINLOSIZE)
			{
				leftovers.add(rlid, Leftover(x + width, y + height, width_delta, height_delta));
				rlid++;

				if (width >= MINLOSIZE)
				{
					leftovers.add(rlid, Leftover(x, y + height, width, height_delta));
					rlid++;
				}

				if (height >= MINLOSIZE)
				{
					leftovers.add(rlid, Leftover(x + width, y, width_delta, height));
					rlid++;
				}
			}
			else if (width_delta >= MINLOSIZE)
			{
				leftovers.add(rlid, Leftover(x + width, y, width_delta, height + height_delta));
				rlid++;
			}
			else if (height_delta >= MINLOSIZE)
			{
				leftovers.add(rlid, Leftover(x, y + height, width + width_delta, height_delta));
				rlid++;
			}
		}
		else
		{
			if (border.x() + width > ATLASW)
			{
				border.set_x(0);
				border.shift_y(yrange.second());

				if (border.y() + height > ATLASH)
					clearinternal();
				else
					yrange = Range<GLshort>();
			}

			x = border.x();
			y = border.y();

			border.shift_x(width);

			if (height > yrange.second())
			{
				if (x >= MINLOSIZE && height - yrange.second() >= MINLOSIZE)
				{
					leftovers.add(rlid, Leftover(0, yrange.first(), x, height - yrange.second()));
					rlid++;
				}

				wasted += x * (height - yrange.second());

				yrange = Range<int16_t>(y + height, height);
			}
			else if (height < yrange.first() - y)
			{
				if (width >= MINLOSIZE && yrange.first() - y - height >= MINLOSIZE)
				{
					leftovers.add(rlid, Leftover(x, y + height, width, yrange.first() - y - height));
					rlid++;
				}

				wasted += width * (yrange.first() - y - height);
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, bmp.data());

		return offsets.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(id),
			std::forward_as_tuple(x, y, width, height)
		).first->second;
	}

	void GraphicsGL::draw(const nl::bitmap& bmp, const Rectangle<int16_t>& rect, const Range<int16_t>& vertical, const Color& color, float angle)
	{
		if (locked)
			return;

		if (color.invisible())
			return;

		if (!rect.overlaps(SCREEN))
			return;

		Offset offset = getoffset(bmp);

		offset.top += vertical.first();
		offset.bottom -= vertical.second();

		quads.emplace_back(rect.left(), rect.right(), rect.top() + vertical.first(), rect.bottom() - vertical.second(), offset, color, angle);
	}

	Text::Layout GraphicsGL::createlayout(const std::string& text, Text::Font id, Text::Alignment alignment, int16_t maxwidth, bool formatted, int16_t line_adj)
	{
		size_t length = text.length();

		if (length == 0)
			return Text::Layout();

		LayoutBuilder builder(fonts[id], id, alignment, maxwidth, formatted, line_adj);

		const char* p_text = text.c_str();

		size_t first = 0;
		size_t offset = 0;

		while (offset < length)
		{
			size_t last = text.find_first_of(" \\#", offset + 1);

			if (last == std::string::npos)
				last = length;

			first = builder.add(p_text, first, offset, last);
			offset = last;
		}

		return builder.finish(first, offset);
	}

	GraphicsGL::LayoutBuilder::LayoutBuilder(const Font& f, Text::Font fid, Text::Alignment a, int16_t mw, bool fm, int16_t la) : font(f), base_fontid(fid), alignment(a), maxwidth(mw), formatted(fm), line_adj(la)
	{
		fontid = Text::Font::NUM_FONTS;
		color = Color::Name::NUM_COLORS;
		ax = 0;
		ay = font.linespace();
		width = 0;
		endy = 0;
		skip_lead_hash = false;

		lines.reserve(16);
		words.reserve(32);
		advances.reserve(128);

		if (maxwidth == 0)
			maxwidth = 800;
	}

	size_t GraphicsGL::LayoutBuilder::add(const char* text, size_t prev, size_t first, size_t last)
	{
		// Consume the standalone closing `#` of a macro we processed on
		// the previous call. The splitter delivers it as its own token,
		// and we need to push an advance for that byte so byte→x lookups
		// stay aligned with the source text.
		if (skip_lead_hash && first < last && text[first] == '#')
		{
			skip_lead_hash = false;
			advances.push_back(ax);
			first++;
			if (first >= last)
				return prev;
		}

		if (first == last)
			return prev;

		// Inline image macro: `#v<id>#` / `#i<id>#` (item icon),
		// `#q<id>#` (quest icon), `#s<id>#` (skill icon),
		// `#f<id>#` (face icon). Reserve a fixed 32 px slot in the
		// layout and record (pos, id, kind) so the caller can overlay
		// the actual bitmap from the right NX source. Close the
		// current word at the macro's start so its bytes are excluded
		// from glyph rendering, and skip the closing `#` on the next
		// add() call.
		auto image_kind_for = [](char c) -> Text::Layout::ImageKind
		{
			switch (c)
			{
			case 'q': return Text::Layout::ImageKind::QUEST;
			case 's': return Text::Layout::ImageKind::SKILL;
			case 'f': return Text::Layout::ImageKind::FACE;
			default:  return Text::Layout::ImageKind::ITEM;
			}
		};
		if (formatted && first + 2 <= last
			&& text[first] == '#'
			&& (text[first + 1] == 'v' || text[first + 1] == 'i'
				|| text[first + 1] == 'q' || text[first + 1] == 's'
				|| text[first + 1] == 'f'))
		{
			constexpr int16_t ICON_W = 32;

			int32_t item_id = 0;
			try
			{
				if (last > first + 2)
					item_id = std::stoi(std::string(text + first + 2, text + last));
			}
			catch (...) { item_id = 0; }

			Text::Layout::ImageKind kind = image_kind_for(text[first + 1]);

			// Wrap onto a new line if the icon would overflow the
			// current line — close the in-flight word at this position
			// before resetting ax/ay, then continue placing the icon
			// at the new line's origin.
			if (ax > 0 && ax + ICON_W > maxwidth)
			{
				add_word(prev, first, fontid, color);
				add_line();
				endy = ay;
				ax = 0;
				ay += font.linespace();
				if (lines.size() > 0)
					ay -= line_adj;
			}
			else
			{
				// Close the pending word right before the macro so its
				// bytes are excluded from text rendering.
				add_word(prev, first, fontid, color);
			}

			int16_t pre_icon_ax = ax;
			if (item_id > 0)
			{
				Text::Layout::Image img;
				// Place the icon's BOTTOM at the text baseline so the
				// sprite sits directly across from the glyphs (matches
				// QuestHelper's row layout). The previous attempt
				// centered the icon vertically and pushed its top into
				// the prior line — players saw the icon floating above
				// the item name instead of next to it.
				img.pos = Point<int16_t>(pre_icon_ax, ay - ICON_W);
				img.item_id = item_id;
				img.size = ICON_W;
				img.kind = kind;
				images.push_back(img);
			}

			ax += ICON_W;
			if (width < ax) width = ax;

			// Placeholder advances for `#v<id>` (without the closing #).
			// They aren't in any word so they won't render as glyphs;
			// the value just keeps byte→x lookups well-defined.
			for (size_t b = first; b < last; b++)
				advances.push_back(pre_icon_ax);

			skip_lead_hash = true;
			// Returning last + 1 makes the caller's next `prev` land
			// past the closing `#`, so the subsequent word starts there.
			return last + 1;
		}

		Text::Font last_font = fontid;
		Color::Name last_color = color;
		size_t skip = 0;
		bool linebreak = false;

		if (formatted)
		{
			switch (text[first])
			{
				case '\\':
				{
					if (first + 1 < last)
					{
						switch (text[first + 1])
						{
							case 'n':
								linebreak = true;
								break;
							case 'r':
								linebreak = ax > 0;
								break;
						}

						skip++;
					}

					skip++;
					break;
				}
				case '#':
				{
					if (first + 1 < last)
					{
						switch (text[first + 1])
						{
							case 'k':
								color = Color::Name::DARKGREY;
								break;
							case 'b':
								color = Color::Name::BLUE;
								break;
							case 'r':
								color = Color::Name::RED;
								break;
							case 'c':
								color = Color::Name::ORANGE;
								break;
						}

						skip++;
					}

					skip++;
					break;
				}
			}
		}

		int16_t wordwidth = 0;

		if (!linebreak)
		{
			size_t i = first;
			while (i < last)
			{
				size_t consumed = 1;
				uint32_t cp = utf8_decode(text, last, i, consumed);
				if (consumed == 0) break;
				Font::Char ch = GraphicsGL::get().ensure_glyph(base_fontid, cp);
				wordwidth += ch.ax;

				if (wordwidth > maxwidth)
				{
					if (last - first <= consumed)
						return last;
					// First codepoint alone is too wide; skip past it to avoid
					// infinite recursion when nothing has been consumed yet.
					if (i == first)
					{
						size_t next = first + consumed;
						if (next >= last) return last;
						return add(text, prev, next, last);
					}
					prev = add(text, prev, first, i);
					return add(text, prev, i, last);
				}
				i += consumed;
			}
		}

		bool newword = skip > 0;
		bool newline = linebreak || ax + wordwidth > maxwidth;

		if (newword || newline)
			add_word(prev, first, last_font, last_color);

		if (newline)
		{
			add_line();

			endy = ay;
			ax = 0;
			ay += font.linespace();

			if (lines.size() > 0)
				ay -= line_adj;
		}

		size_t pos = first;
		while (pos < last)
		{
			size_t consumed = 1;
			uint32_t cp = utf8_decode(text, last, pos, consumed);
			if (consumed == 0) break;
			Font::Char ch = GraphicsGL::get().ensure_glyph(base_fontid, cp);

			// Push the same advance for each byte of the codepoint so byte-
			// indexed lookups (word.first, etc.) keep lining up.
			for (size_t b = 0; b < consumed; ++b)
				advances.push_back(ax);

			bool skip_char = (pos < first + skip) || (newline && cp == ' ');
			if (!skip_char)
			{
				ax += ch.ax;
				if (width < ax) width = ax;
			}

			pos += consumed;
		}

		if (newword || newline)
			return first + skip;
		else
			return prev;
	}

	Text::Layout GraphicsGL::LayoutBuilder::finish(size_t first, size_t last)
	{
		add_word(first, last, fontid, color);
		add_line();

		advances.push_back(ax);

		return Text::Layout(lines, advances, images, width, ay, ax, endy);
	}

	void GraphicsGL::LayoutBuilder::add_word(size_t word_first, size_t word_last, Text::Font word_font, Color::Name word_color)
	{
		words.push_back({ word_first, word_last, word_font, word_color });
	}

	void GraphicsGL::LayoutBuilder::add_line()
	{
		int16_t line_x = 0;
		int16_t line_y = ay;

		switch (alignment)
		{
			case Text::Alignment::CENTER:
				line_x -= ax / 2;
				break;
			case Text::Alignment::RIGHT:
				line_x -= ax;
				break;
		}

		lines.push_back({ words, { line_x, line_y } });
		words.clear();
	}

	void GraphicsGL::drawtext(const DrawArgument& args, const Range<int16_t>& vertical, const std::string& text, const Text::Layout& layout, Text::Font id, Color::Name colorid, Text::Background background)
	{
		if (locked)
			return;

		const Color& color = args.get_color();

		if (text.empty() || color.invisible())
			return;

		const Font& font = fonts[id];

		GLshort x = args.getpos().x();
		GLshort y = args.getpos().y();
		GLshort w = layout.width();
		GLshort h = layout.height();
		GLshort minheight = vertical.first() > 0 ? vertical.first() : SCREEN.top();
		GLshort maxheight = vertical.second() > 0 ? vertical.second() : SCREEN.bottom();

		switch (background)
		{
			case Text::Background::NAMETAG:
			{
				// If ever changing code in here confirm placements with map 10000
				for (const Text::Layout::Line& line : layout)
				{
					GLshort left = x + line.position.x() - 1;
					GLshort right = left + w + 3;
					GLshort top = y + line.position.y() - font.linespace() + 6;
					GLshort bottom = top + h - 2;
					Color ntcolor = Color(0.0f, 0.0f, 0.0f, 0.6f);

					quads.emplace_back(left, right, top, bottom, nulloffset, ntcolor, 0.0f);
					quads.emplace_back(left - 1, left, top + 1, bottom - 1, nulloffset, ntcolor, 0.0f);
					quads.emplace_back(right, right + 1, top + 1, bottom - 1, nulloffset, ntcolor, 0.0f);
				}

				break;
			}
		}

		for (const Text::Layout::Line& line : layout)
		{
			Point<int16_t> position = line.position;

			for (const Text::Layout::Word& word : line.words)
			{
				GLshort ax = position.x() + layout.advance(word.first);
				GLshort ay = position.y();

				const GLfloat* wordcolor;

				if (word.color < Color::Name::NUM_COLORS)
					wordcolor = Color::colors[word.color];
				else
					wordcolor = Color::colors[colorid];

				Color abscolor = color * Color(wordcolor[0], wordcolor[1], wordcolor[2], 1.0f);

				size_t pos = word.first;
				while (pos < word.last)
				{
					size_t consumed = 1;
					uint32_t cp = utf8_decode(text.c_str(), word.last, pos, consumed);
					if (consumed == 0) break;
					Font::Char ch = ensure_glyph(id, cp);

					GLshort char_x = x + ax + ch.bl;
					GLshort char_y = y + ay - ch.bt;
					GLshort char_width = ch.bw;
					GLshort char_height = ch.bh;
					GLshort char_bottom = char_y + char_height;

					Offset offset = ch.offset;

					if (ch.color)
					{
						// Color emoji strikes: scale the full bitmap to ax x ax.
						char_width = ch.ax;
						char_height = ch.ax;
						char_y = y + ay - ch.ax;
						char_bottom = y + ay;
					}

					if (char_bottom > maxheight)
					{
						GLshort bottom_adjust = char_bottom - maxheight;

						if (bottom_adjust < 10)
						{
							offset.bottom -= bottom_adjust;
							char_bottom -= bottom_adjust;
						}
						else
						{
							pos += consumed;
							continue;
						}
					}

					if (char_y < minheight)
					{
						pos += consumed;
						continue;
					}

					if (ax == 0 && cp == ' ')
					{
						pos += consumed;
						continue;
					}

					ax += ch.ax;

					if (char_width <= 0 || char_height <= 0)
					{
						pos += consumed;
						continue;
					}

					// Color glyphs carry their own RGBA, so bypass the color
					// modulation that would tint monochrome alpha glyphs.
					Color draw_color = ch.color ? color : abscolor;
					quads.emplace_back(char_x, char_x + char_width, char_y, char_bottom, offset, draw_color, 0.0f);
					pos += consumed;
				}
			}
		}
	}

	void GraphicsGL::drawrectangle(int16_t x, int16_t y, int16_t width, int16_t height, float red, float green, float blue, float alpha)
	{
		if (locked)
			return;

		quads.emplace_back(x, x + width, y, y + height, nulloffset, Color(red, green, blue, alpha), 0.0f);
	}

	void GraphicsGL::drawscreenfill(float red, float green, float blue, float alpha)
	{
		drawrectangle(0, 0, VWIDTH, VHEIGHT, red, green, blue, alpha);
	}

	void GraphicsGL::lock()
	{
		locked = true;
	}

	void GraphicsGL::unlock()
	{
		locked = false;
	}

	void GraphicsGL::flush(float opacity)
	{
		bool coverscene = opacity != 1.0f;

		if (coverscene)
		{
			float complement = 1.0f - opacity;
			Color color = Color(0.0f, 0.0f, 0.0f, complement);

			quads.emplace_back(SCREEN.left(), SCREEN.right(), SCREEN.top(), SCREEN.bottom(), nulloffset, color, 0.0f);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		GLsizeiptr csize = quads.size() * sizeof(Quad);
		GLsizeiptr fsize = quads.size() * Quad::LENGTH;

#ifdef PLATFORM_IOS
		// iOS/GLKit may reset GL state between frames — re-establish it
		glUseProgram(shaderProgram);
		glUniform1i(uniform_fontregion, fontymax);
		glUniform2f(uniform_atlassize, ATLASW, ATLASH);
		glUniform2f(uniform_screensize, VWIDTH, VHEIGHT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, atlas);
#endif

		glEnableVertexAttribArray(attribute_coord);
		glEnableVertexAttribArray(attribute_color);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, csize, quads.data(), GL_STREAM_DRAW);

		glVertexAttribPointer(attribute_coord, 4, GL_SHORT, GL_FALSE, sizeof(Quad::Vertex), 0);
		glVertexAttribPointer(attribute_color, 4, GL_FLOAT, GL_FALSE, sizeof(Quad::Vertex), (const void*)8);

#ifdef PLATFORM_IOS
		// OpenGL ES 3.0 does not support GL_QUADS — draw as indexed triangles
		size_t quad_count = quads.size();
		std::vector<GLushort> indices;
		indices.reserve(quad_count * 6);

		for (size_t i = 0; i < quad_count; i++)
		{
			GLushort base = static_cast<GLushort>(i * Quad::LENGTH);
			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 0);
			indices.push_back(base + 2);
			indices.push_back(base + 3);
		}

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_SHORT, indices.data());
#else
		glDrawArrays(GL_QUADS, 0, fsize);
#endif

		glDisableVertexAttribArray(attribute_coord);
		glDisableVertexAttribArray(attribute_color);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (coverscene)
			quads.pop_back();
	}

	void GraphicsGL::clearscene()
	{
		if (!locked)
			quads.clear();
	}
}