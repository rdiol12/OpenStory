#pragma once
#include "Graphics/GraphicsGL.h"
#include <array>
#include <cstdio>
#include <cstdlib>

namespace ms {
// Startup-only diagnostic: use the real renderer before the first displayed frame.
inline bool check_renderer_colors(int framebuffer_width, int framebuffer_height)
{
    auto& dimensions = Constants::Constants::get();
    const int width = dimensions.get_viewwidth(), height = dimensions.get_viewheight();
    if (width < 64 || height < 32 || framebuffer_width < 64 || framebuffer_height < 32)
        return false;
    alignas(uint32_t) static constexpr std::array<unsigned char, 16> bgra{
        0, 0, 255, 255, 0, 255, 0, 255,
        255, 0, 0, 255, 0, 255, 255, 255};
    static constexpr unsigned char expected[4][4]{
        {255, 0, 0, 255}, {0, 255, 0, 255},
        {0, 0, 255, 255}, {255, 255, 0, 255}};
    auto& graphics = GraphicsGL::get();
    const auto prior_error = glGetError();
    graphics.clearscene();
    graphics.drawraw(reinterpret_cast<size_t>(bgra.data()), 2, 2, bgra.data(),
                     {0, 32, 0, 32}, {}, Color::CWHITE, 0);
    for (int i = 0; i < 4; ++i)
        graphics.drawrectangle(32 + (i % 2) * 16, (i / 2) * 16, 16, 16,
            expected[i][0] / 255.f, expected[i][1] / 255.f, expected[i][2] / 255.f, 1);
    graphics.flush(1);
    bool passed = prior_error == GL_NO_ERROR;
    for (int sample = 0; sample < 8; ++sample) {
        const int i = sample % 4;
        const int x = 8 + (i % 2) * 16 + (sample / 4) * 32;
        const int y = 8 + (i / 2) * 16;
        std::array<unsigned char, 4> pixel{};
        glReadPixels(x * framebuffer_width / width,
            framebuffer_height - 1 - y * framebuffer_height / height,
            1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        std::printf("[ColorProbe] source=%s expected=%u,%u,%u,%u actual=%u,%u,%u,%u\n",
            sample < 4 ? "texture" : "plain", expected[i][0], expected[i][1],
            expected[i][2], expected[i][3], pixel[0], pixel[1], pixel[2], pixel[3]);
        for (int channel = 0; channel < 4; ++channel)
            passed &= std::abs(int(pixel[channel]) - expected[i][channel]) <= 2;
    }
    graphics.clearscene();
    graphics.flush(1);
    const auto error = glGetError();
    passed &= error == GL_NO_ERROR;
    std::printf("[ColorProbe] passed=%d prior-gl=%#x gl=%#x\n", passed, prior_error, error);
    return passed;
}
}
