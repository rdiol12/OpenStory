// Runs the production renderer in a desktop Core context without NX files/server.
#include "Graphics/GraphicsGL.h"
#include "../platform/shared/ColorProbe.h"
#include <glfw3.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>

static void pixel(int x, int y, int r, int g, int b)
{
    std::array<unsigned char, 4> p{};
    glReadPixels(x, 63 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p.data());
    if (std::abs(int(p[0]) - r) > 2 || std::abs(int(p[1]) - g) > 2 || std::abs(int(p[2]) - b) > 2)
    {
        std::fprintf(stderr, "pixel(%d,%d): %u,%u,%u expected %d,%d,%d\n",
            x, y, p[0], p[1], p[2], r, g, b);
        std::abort();
    }
}

int main()
{
    assert(glfwInit());
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(64, 64, "OpenStory graphics check", nullptr, nullptr);
    assert(window);
    glfwMakeContextCurrent(window);
    auto& dimensions = ms::Constants::Constants::get();
    dimensions.set_viewwidth(64);
    dimensions.set_viewheight(64);
    dimensions.set_ui_scale(1);
    auto& graphics = ms::GraphicsGL::get();
    if (auto error = graphics.init())
    {
        std::fprintf(stderr, "%s\n", error.get_message());
        return 1;
    }
    graphics.reinit();
    glViewport(0, 0, 64, 64);
    graphics.clearscene();
    graphics.drawrectangle(0, 0, 64, 64, 0.25f, 0, 0, 1);
    graphics.setblend(true);
    graphics.drawrectangle(0, 0, 32, 64, 0, 0.5f, 0, 1);
    graphics.setblend(false);
    graphics.drawrectangle(32, 0, 32, 32, 0, 0, 1, 1);
    graphics.flush(1);
    assert(glGetError() == GL_NO_ERROR);
    pixel(10, 10, 64, 128, 0);
    pixel(40, 10, 0, 0, 255);
    pixel(40, 40, 64, 0, 0);

    // The startup diagnostic uses the same production renderer and backbuffer.
    assert(ms::check_renderer_colors(64, 64));
    pixel(8, 8, 0, 0, 0); // Diagnostic pixels must be cleared before display.

    // Prove it detects an actual texture sampler channel swap, with plain colors intact.
    std::puts("Color probe: deliberately swapping texture channels (failure expected)");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    assert(glGetError() == GL_NO_ERROR);
    assert(!ms::check_renderer_colors(64, 64));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
    assert(glGetError() == GL_NO_ERROR);

    // Exceed the 16-bit index limit. The last quad must still be drawn.
    graphics.clearscene();
    for (int i = 0; i < 17000; ++i)
        graphics.drawrectangle(0, 0, 1, 1, 1, 0, 0, 1);
    graphics.drawrectangle(32, 32, 32, 32, 0, 1, 0, 1);
    graphics.flush(1);
    assert(glGetError() == GL_NO_ERROR);
    pixel(40, 40, 0, 255, 0);
    graphics.clearscene();
    graphics.flush(1);
    assert(glGetError() == GL_NO_ERROR);
    pixel(40, 40, 0, 0, 0);
    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("Core renderer: blending, BGRA artwork/plain UI colors, large batches, empty frames passed");
}
