//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — OpenGL ES 3.0 Graphics Adapter
//	This file replaces the desktop GraphicsGL::init() shaders and GLEW calls.
//	It is compiled INSTEAD of the original GraphicsGL.cpp on iOS.
//	The atlas system, text rendering, and quad batching remain identical.
//////////////////////////////////////////////////////////////////////////////////
#include "Graphics/GraphicsGL.h"

#include "Configuration.h"

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
		// OpenGL ES 3.0 shaders — ported from GLSL 1.20
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

		const GLsizei bufSize = 512;
		GLint success;
		GLchar infoLog[bufSize];

		// No glewInit() on iOS — GL ES 3.0 is loaded by EAGLContext

		if (FT_Init_FreeType(&ftlibrary))
			return Error::Code::FREETYPE;

		FT_Int ftmajor, ftminor, ftpatch;
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
		uniform_texture = glGetUniformLocation(shaderProgram, "tex");
		uniform_atlassize = glGetUniformLocation(shaderProgram, "atlassize");
		uniform_screensize = glGetUniformLocation(shaderProgram, "screensize");
		uniform_yoffset = glGetUniformLocation(shaderProgram, "yoffset");
		uniform_fontregion = glGetUniformLocation(shaderProgram, "fontregion");

		if (attribute_coord == -1 || attribute_color == -1 || uniform_texture == -1 || uniform_atlassize == -1 || uniform_screensize == -1 || uniform_yoffset == -1)
			return Error::Code::SHADER_VARS;

		// VAO required by OpenGL ES 3.0
		GLuint VAO;
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		// Vertex Buffer Object
		glGenBuffers(1, &VBO);

		glGenTextures(1, &atlas);
		glBindTexture(GL_TEXTURE_2D, atlas);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLASW, ATLASH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		fontborder.set_y(1);

		// iOS font paths — use bundled fonts
		NSString* normalPath = [[NSBundle mainBundle] pathForResource:@"Roboto-Regular" ofType:@"ttf"];
		NSString* boldPath = [[NSBundle mainBundle] pathForResource:@"Roboto-Bold" ofType:@"ttf"];

		const char* FONT_NORMAL_STR = normalPath ? [normalPath UTF8String] : nullptr;
		const char* FONT_BOLD_STR = boldPath ? [boldPath UTF8String] : nullptr;

		if (!FONT_NORMAL_STR || !FONT_BOLD_STR)
			return Error::Code::FONT_PATH;

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

		return Error::Code::NONE;
	}

	// addfont, reinit, clear, addbitmap, getoffset, draw, createlayout,
	// drawtext, drawrectangle, drawscreenfill, lock, unlock, clearscene
	// are IDENTICAL to the desktop version and compiled from the original
	// GraphicsGL.cpp via the shared source include in CMakeLists.txt.
	//
	// Only init() differs (shaders + no GLEW + VAO + font paths).
	// The flush() method also needs a minor change for GL ES (no GL_QUADS).

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

		glEnableVertexAttribArray(attribute_coord);
		glEnableVertexAttribArray(attribute_color);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, csize, quads.data(), GL_STREAM_DRAW);

		glVertexAttribPointer(attribute_coord, 4, GL_SHORT, GL_FALSE, sizeof(Quad::Vertex), 0);
		glVertexAttribPointer(attribute_color, 4, GL_FLOAT, GL_FALSE, sizeof(Quad::Vertex), (const void*)8);

		// OpenGL ES 3.0 does not support GL_QUADS — draw as triangles
		// Each quad = 4 vertices, needs 6 indices (2 triangles)
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

		glDisableVertexAttribArray(attribute_coord);
		glDisableVertexAttribArray(attribute_color);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (coverscene)
			quads.pop_back();
	}
}
