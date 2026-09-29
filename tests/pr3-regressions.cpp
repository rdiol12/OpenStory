// Ordinary offline inputs through production handlers, assets, textfields and Stage.
#include "Audio/Audio.h"
#include "Configuration.h"
#include "Gameplay/Stage.h"
#include "Graphics/Animation.h"
#include "IO/UI.h"
#include "IO/Components/Textfield.h"
#include "Net/Handlers/Helpers/LoginParser.h"
#include "Net/Handlers/PlayerHandlers.h"
#include <nlnx/nx.hpp>
#include <SDL_mixer.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ms { Error init(bool offline); }
using namespace ms;

static std::vector<int32_t> dispatched_skills;
// Observe Stage's real routing without casting a skill or sending anything.
extern "C" void __wrap__ZN2ms6Combat8use_moveEi(void*, int32_t id)
{
    dispatched_skills.push_back(id);
}

static void append(std::vector<int8_t>& record, uint64_t value, size_t bytes)
{
    for (size_t i = 0; i < bytes; ++i)
        record.push_back(static_cast<int8_t>(value >> (8 * i)));
}

static StatsEntry login_stats(uint8_t level)
{
    // Complete ordinary v83 character-stat record; no socket or malformed input.
    std::vector<int8_t> record(13, 0);
    const std::string name = "Regression";
    std::copy(name.begin(), name.end(), record.begin());
    append(record, 0, 1); // Male.
    append(record, 0, 1); // Skin.
    append(record, 20000, 4); // Face.
    append(record, 30000, 4); // Hair.
    for (int i = 0; i < 3; ++i) append(record, 0, 8); // No pets.
    append(record, level, 1);
    append(record, 0, 2); // Beginner job.
    for (int value : {4, 4, 4, 4, 50, 50, 50, 50, 0, 0})
        append(record, value, 2); // STR, DEX, INT, LUK, HP/max, MP/max, AP, SP.
    append(record, 0, 4); // EXP.
    append(record, 0, 2); // Fame.
    append(record, 0, 4); // Gacha EXP.
    append(record, 100000000, 4); // Henesys.
    append(record, 0, 1); // Portal.
    append(record, 0, 4); // Timestamp.
    InPacket packet(record.data(), record.size());
    auto stats = LoginParser::parse_stats(packet);
    if (packet.length() != 0) throw std::runtime_error("login fixture was not fully consumed");
    return stats;
}

int main()
{
    try {
        Configuration::get().load();
        if (auto error = ms::init(true))
            throw std::runtime_error(std::string(error.get_message()) + error.get_args());
        struct CloseAudio { ~CloseAudio() { Sound::close(); } } close_audio;
        int failures = 0;
        auto check = [&](bool passed, const std::string& description) {
            std::cout << (passed ? "PASS: " : "FAIL: ") << description << '\n';
            failures += !passed;
        };

        auto& stage = Stage::get();
        LookEntry look{};
        look.faceid = 20000;
        look.hairid = 30000;
        stage.loadplayer({login_stats(127), look, 1});
        for (uint8_t level : {127, 128, 200}) {
            const auto parsed = login_stats(level).stats[MapleStat::LEVEL];
            check(parsed == level, "login level " + std::to_string(level) + " decoded " + std::to_string(parsed));
            stage.get_player().get_stats().set_stat(MapleStat::LEVEL, level - 1);
            std::vector<int8_t> record;
            append(record, 0, 1); // Item reaction.
            append(record, MapleStat::codes[MapleStat::LEVEL], 4);
            append(record, level, 1);
            InPacket packet(record.data(), record.size());
            ChangeStatsHandler{}.handle(packet);
            const auto changed = stage.get_player().get_level();
            check(changed == level && packet.length() == 0,
                  "stat-update level " + std::to_string(level) + " decoded " + std::to_string(changed));
        }

        const auto light = nl::nx::map["Back"]["shineWood.img"]["back"]["16"];
        if (!light || light["a0"].get_integer() != 235 || light["a1"])
            throw std::runtime_error("expected the v83 light-ray asset with a0=235 and no a1");
        const Frame frame(light);
        check(frame.start_opacity() == 235 && frame.opcstep(8) == 0,
              "single-ended light alpha remains 235; 8ms delta=" + std::to_string(frame.opcstep(8)));

        Mix_HaltMusic();
        Music("BGM06.img/FinalFight").play();
        check(Mix_PlayingMusic() != 0, "mis-cased map music resolves and starts playback");

        const std::vector<std::string> samples = {
            "hello", u8"\u05e9\u05dc\u05d5\u05dd", u8"\u05e9\u05dc\u05d5\u05dd abc", u8"\u05d0\u05d112"
        };
        for (size_t sample = 0; sample < samples.size(); ++sample) {
            int invalid = 0;
            for (int16_t x = 1; x < 250; ++x) {
                Textfield field(Text::A13M, Text::LEFT, Color::BLACK, {0, 300, 0, 30}, 256);
                field.update({0, 0});
                field.change_text(samples[sample]);
                field.send_cursor({x, 10}, true);
                field.send_cursor({x, 10}, false);
                field.add_codepoint('X');
                auto edited = field.get_text();
                const auto at = edited.find('X');
                const bool boundary = at <= samples[sample].size() &&
                    (at == samples[sample].size() || (static_cast<unsigned char>(samples[sample][at]) & 0xc0) != 0x80);
                if (at != std::string::npos) edited.erase(at, 1);
                invalid += !boundary || edited != samples[sample];
                UI::get().remove_textfield();
            }
            check(invalid == 0, "UTF-8 cursor insertion sample " + std::to_string(sample) +
                  " invalid byte boundaries=" + std::to_string(invalid));
        }

        stage.load(100000000, 0); // Local map assets only; no game loop or connection.
        constexpr int32_t skill = 1000; // Beginner Three Snails; the combat call is recorded.
        stage.send_key(KeyType::SKILL, skill, false);
        check(dispatched_skills.empty(), "skill release alone does not dispatch");
        dispatched_skills.clear();
        stage.send_key(KeyType::SKILL, skill, true);
        stage.send_key(KeyType::SKILL, skill, false);
        check(dispatched_skills == std::vector<int32_t>{skill}, "one skill press/release dispatches once");
        stage.clear();
        std::cout << "PR3 offline regressions: " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
