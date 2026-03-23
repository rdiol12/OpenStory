# OpenStory

A v83 MapleStory client for Cosmic/private servers. Forked from [HeavenClient](https://github.com/HeavenClient/HeavenClient).

**Status: Playable** -- functional for gameplay, UI polish ongoing.

## Features

- Full v83 client connecting to Cosmic servers
- NX file-based assets, OpenGL rendering, GLFW windowing
- Login flow, character select, world/channel select, PIC entry
- Combat, skills, buffs, inventory, quests, NPCs, chat
- Storage, buddy list, skill macros, gamepad support
- Fullscreen, UI scaling, drag-and-drop windows

## Building

### Requirements
- Visual Studio 2022, Windows SDK, CMake 3.15+
- Dependencies: GLFW, GLEW, FreeType, Bass audio, NoLifeNx, Asio

### Build
```bash
cmake -S . -B build
cmake --build build --config Debug --target OpenStory
# Output: wz/OpenStory.exe
```

Place v83 NX files in the `wz/` directory.

## Configuration

Edit `Configuration.h` for defaults. A `Settings` file is generated after first run.

## Credits

- **Daniel Allendorf & Ryan Payton** -- Original [HeavenClient](https://github.com/HeavenClient/HeavenClient)
- **rdiol12** -- v83 Cosmic compatibility, UI systems, packet handlers

## Changelog

### Recent Fixes
- **Cygnus Knight skills**: Fixed 8-digit skill ID lookup for effects, buffs, passive stats, and afterimages
- **Skill sounds**: Fixed duplicate sample bug preventing skill sounds from playing
- **Quest system**: Completed quests now properly removed from active list; NPC quest bubbles refresh on state changes
- **Quest packets**: Full v83 quest handler coverage -- QUEST_CLEAR effect/sound, UPDATE_QUEST_INFO error sub-types (0x0A-0x0F), scripted quest action packets
- **EXP gain**: Fixed "not handled" message appearing for in-chat EXP notifications
- **SHOW_ITEM_GAIN_INCHAT**: Proper v83 mode switch for all 24 effect types
- **Notice button**: Changed from broken button to static sprite matching v83 client
- **Character select**: Updated info panel coordinates to match JourneyClient layout

## License

GNU Affero General Public License v3. See [LICENSE](LICENSE).
