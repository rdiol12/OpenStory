#include "Audio/Audio.h"
#include "Configuration.h"
#include "Gameplay/Stage.h"
#include "IO/UI.h"
#include "IO/UITypes/UIStatusBar.h"
#include "IO/Components/MapleComboBox.h"
#include "IO/Components/Slider.h"
#include <nlnx/nx.hpp>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace ms { Error init(bool offline); }

int main(int argc, char** argv) {
    try {
        ms::Configuration::get().load();
        if (auto error = ms::init(true))
            throw std::runtime_error(std::string(error.get_message()) + error.get_args());
        struct CloseAudio { ~CloseAudio() { ms::Sound::close(); } } close_audio;
        assert(nl::nx::ui["Basic.img"]["VScr"] && !nl::nx::ui["Basic.img"]["VScr9"]);
        assert(!nl::nx::ui["StatusBar2.img"] && !nl::nx::ui["UIWindow2.img"]);
        const std::string test = argc > 1 ? argv[1] : "slider";
        std::cout << "CHECK " << test << std::endl;
        if (test == "slider") {
            // These are the actual shop/inventory and chat styles; pure v83 lacks them.
            for (int type : {9, 11, int(ms::Slider::CHATBAR), 999}) {
                std::cout << "draw slider " << type << std::endl;
                ms::Slider slider(type, {0, 100}, 0, 4, 12, [](bool) {});
                slider.draw({});
                slider.setenabled(false);
                slider.draw({});
            }
        } else if (test == "combo") {
            // Later styles can be requested through the shared component API.
            for (auto type : {ms::MapleComboBox::DEFAULT, ms::MapleComboBox::BLACKL}) {
                std::cout << "draw combo " << int(type) << std::endl;
                ms::MapleComboBox combo(type, {"One", "Two"}, 0, {}, {}, 100);
                combo.draw({});
            }
        } else if (test == "game") {
            ms::UI::get().change_state(ms::UI::GAME);
            ms::UI::get().update();
            ms::UI::get().draw(1.0f);
            const auto statusbar = ms::UI::get().get_element<ms::UIStatusBar>();
            assert(statusbar && !statusbar->controller_targets().empty());
        } else if (test == "cooldown") {
            auto& player = ms::Stage::get().get_player();
            auto check = [](bool condition, const char* message) {
                if (!condition) throw std::runtime_error(message);
            };
            player.add_cooldown(1000, 1);
            check(player.has_cooldown(1000), "new cooldown starts active");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            player.add_cooldown(1001, 1);
            player.add_cooldown(1002, 10);
            player.add_cooldown(1002, 0);
            check(!player.has_cooldown(1002), "server zero cancels an existing cooldown");
            player.add_cooldown(1003, 10);
            player.add_cooldown(1003, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(850));
            // No Player::update during these waits: map/loading pauses must not extend server cooldowns.
            check(!player.has_cooldown(1000), "cooldown must expire after elapsed time without player updates");
            check(player.has_cooldown(1001), "staggered cooldown must keep its own full second");
            check(player.has_cooldown(1003), "replacement cooldown starts from the server update");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            check(!player.has_cooldown(1001), "staggered cooldown must eventually expire");
            check(!player.has_cooldown(1003), "server replacement duration must replace the old deadline");
        } else {
            throw std::runtime_error("unknown test");
        }
        std::cout << "PASS " << test << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
