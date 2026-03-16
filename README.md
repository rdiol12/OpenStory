# OpenStory

A v83 MapleStory client built for cosmic/private servers. Forked from [HeavenClient](https://github.com/HeavenClient/HeavenClient), rewritten and extended for full v83 compatibility.

## Features

- Full v83 MapleStory client connecting to cosmic v83 servers
- NX file-based asset loading (UI, sprites, maps, items)
- OpenGL rendering with GLFW windowing
- Complete login flow: account login, world/channel select, character select, PIC entry
- In-game systems: movement, combat, NPCs, quests, inventory, skills, chat

## What Was Fixed / Added

### Packet Handlers (v83 Compatibility)
- **13 new packet handlers** fully implemented with proper v83 parsing:
  - `UpdateQuestInfoHandler` (opcode 47) — quest start, progress, complete, and forfeit
  - `PartyOperationHandler` (opcode 100) — party create, invite, leave, disband
  - `BuddyListHandler` (opcode 63) — friend list updates and capacity
  - `ClockHandler` (opcode 122) — world clock and countdown timers
  - `FameResponseHandler` (opcode 58) — fame give/receive results
  - `NpcActionHandler` (opcode 263) — NPC stance/animation updates
  - `BlowWeatherHandler` (opcode 140) — map weather effects
  - `FamilyHandler`, `ForcedStatSetHandler`, `SetTractionHandler`, `YellowTipHandler`, `CatchMonsterHandler`, `RelogResponseHandler`
- Unhandled packet logging to `unhandled_packets.txt` for debugging

### Quest Log
- Detail panel showing quest name, level requirement, NPC sprite, rewards with item icons, requirements, and quest description
- Proper text formatting (color code stripping, line break handling)
- Quest forfeit support via `QuestLog::forfeit()`

### Soft Keyboard (PIC Entry)
- Clean 3-column number grid layout (1-9, 0, Del)
- Cancel/OK buttons properly positioned
- Draggable keyboard window
- Numbers-only PIC support

### Character Select Screen
- Characters centered on screen
- Proper name tag positioning
- Start button repositioned
- Create/Delete character functionality verified

### UI Improvements
- Removed key press logging (packet logging retained for debugging)
- World select draw order fixed
- UIChannel uses actual world/channel from Configuration instead of hardcoded values
- Lost items/fame status messages implemented

### Code Quality
- All TODO comments resolved across 27+ files
- Descriptive comments replacing placeholder notes
- PIC validation: repetitive character check (no 3 in a row)

## What's Left To Do

### Not Yet Implemented
- **Party system UI** — handler parses data but no Party data structure or UI display
- **Buddy list UI** — handler parses data but friend list display not connected
- **Clock display** — handler parses time but no on-screen clock rendering
- **Cash Shop purchases** — UI exists but purchase flow not implemented
- **Channel change packet** — no outgoing ChangeChannelPacket
- **Map scrolling toggle** in minimap
- **Region change** — UIWorldSelect refresh after region selection
- **Joypad combo boxes** — controller settings UI incomplete
- **Character creation color picker** — cycles through series instead of direct selection

### Known Issues
- NPC interaction fallback when dialog not found
- Reactor hit/break sounds not played
- Pet/mount button state not checked in character info
- Hurricane/piercing arrow/rapidfire attack packets may need additional 4 bytes
- Critical hit flag placeholder in attack packets

## Building

### Requirements
- Visual Studio 2022
- Windows SDK
- Dependencies: GLFW, GLEW, FreeType, Bass audio, NoLifeNx, stb_image

### Build Steps
```
1. Open build/OpenStory.sln in Visual Studio 2022
2. Set configuration to Debug / x64
3. Build solution
4. Output: wz/OpenStory.exe
```

### NX Files
Place your v83 NX files in the `wz/` directory alongside the executable.

## Configuration

The default settings can be configured by editing `Configuration.h`. These are also generated after a game session in a file called `Settings`.

Key settings:
- **ServerIP / ServerPort** — server connection details
- **Width / Height** — screen resolution (default 1024x768)
- **BGMVolume / SFXVolume** — audio levels (0-100)
- **VSync** — vertical sync toggle
- **SaveLogin** — remember last account name

## Required NX Files

*Check `NxFiles.h` for the full list*

All NX files should be v83 GMS conversions placed in the `wz/` directory.

## Dependencies

| Library | Purpose |
|---------|---------|
| [NoLifeNx](https://github.com/ryantpayton/NoLifeNx) | NX file reading |
| [GLFW3](http://www.glfw.org/) | Window/input management |
| [GLEW](http://glew.sourceforge.net/) | OpenGL extensions |
| [FreeType](http://www.freetype.org/) | Font rendering |
| [Bass](http://www.un4seen.com/) | Audio playback |
| [Asio](http://think-async.com/) | Networking (optional) |

## Credits

### Original Authors
- **Daniel Allendorf** — Original Journey/HeavenClient creator
- **Ryan Payton** — Co-developer of the continued Journey client

### Based On
- [HeavenClient](https://github.com/HeavenClient/HeavenClient) — The original open-source C++ MapleStory client

### OpenStory Contributors
- **rdiol12** — v83 cosmic server compatibility, packet handlers, UI fixes, quest system

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See [LICENSE](LICENSE) for details.
