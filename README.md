# OpenStory

A v83 MapleStory client for Cosmic/private servers. Forked from [HeavenClient](https://github.com/HeavenClient/HeavenClient).

**Status: Playable** — functional for gameplay, UI polish ongoing.

## Screenshots

### Status Bar
![Status Bar](docs/images/statusbar.png)

### Emoji Support & Minimap
![Emoji & Minimap](docs/images/emoji-minimap.png)

### Quest UI & NPC Quest Indicators
![Quest UI](docs/images/quest-ui.png)

## Features

- Full v83 client connecting to Cosmic servers
- NX file-based assets, OpenGL rendering, GLFW windowing
- Login flow, character select, world/channel select, PIC entry
- Combat, skills, buffs, inventory, quests, NPCs, chat
- Storage, buddy list, skill macros, gamepad support
- Distance-based (spatial) sound for world events
- Fullscreen, UI scaling, drag-and-drop windows

### Custom / experimental

Some features are client-side additions not supported by a stock Cosmic server:

| Feature | Status | Notes |
|---------|--------|-------|
| Event System | WIP | Custom `EVENT_INFO` / `REQUEST_EVENT_INFO` packets; needs a server-side handler |
| HP/MP Warning | Working | Client-only |
| Graphics/Effects Quality | Working | Client-only |

## Building

### Requirements
- Visual Studio 2022+, Windows SDK, CMake 3.15+
- Dependencies: GLFW, GLEW, FreeType, Bass audio, NoLifeNx, Asio

### Build
```bash
cmake -S . -B build
cmake --build build --config Debug --target OpenStory
# Output: wz/OpenStory.exe
```

Place v83 NX files in the `wz/` directory.

## Configuration

Edit `Configuration.h` for defaults. A `Settings` file is generated after the first run.

First run: enable **Save Login** on the login screen, close the client, then edit the
generated `Settings` file to set `VSync = false` and `Fullscreen = true`.

## Quest Helper

Track up to 5 quests at once with live progress updates.

- **Add a quest**: Open the Quest Log (Q), go to In-Progress, then drag a quest into the Quest Helper (or double-click an opened quest)
- **Remove a quest**: Click the X button next to the quest name
- **Reorder**: Drag a quest name up or down within the Quest Helper
- **Collapse/Expand**: Click a quest name to toggle its requirements
- **Auto-track**: Click the AUTO button to fill the helper with your active quests

## Credits

- **Daniel Allendorf & Ryan Payton** — Original [HeavenClient](https://github.com/HeavenClient/HeavenClient)
- **rdiol12** — v83 Cosmic compatibility, UI systems, packet handlers

## License

GNU Affero General Public License v3. See [LICENSE](LICENSE).
