#pragma once
#include <cstddef>
#include <cstdint>

namespace ms
{
    // D-pad changes selection; every Cross press activates that selection.
    class ControllerNavigation
    {
    public:
        void enter(std::uintptr_t owner, std::size_t count)
        {
            if (owner != context || selected >= count) {
                context = owner;
                selected = 0;
            }
        }
        void step(int direction, std::size_t count)
        {
            selected = count ? (selected + count + direction) % count : 0;
        }
        bool press(std::uint32_t, std::size_t count) const
        {
            return selected < count;
        }
        std::size_t index() const { return selected; }
    private:
        std::uintptr_t context = 0;
        std::size_t selected = 0;
    };
}
