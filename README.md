# OpenStory

A v83 MapleStory client built for Cosmic/private servers. Forked from [HeavenClient](https://github.com/HeavenClient/HeavenClient), rewritten and extended for full v83 compatibility.

**Status: Playable.** There are major UI fixes still needed, but the client is functional for gameplay.

## Features

- Full v83 MapleStory client connecting to Cosmic v83 servers
- NX file-based asset loading (UI, sprites, maps, items)
- OpenGL rendering with GLFW windowing
- Complete login flow: account login, world/channel select, character select, PIC entry
- In-game systems: movement, combat, NPCs, quests, inventory, skills, chat

## What's Working

### Status Bar -- Menu & System Sub-Panels
- Menu button opens a vertical panel with: Stat, Skill, Quest, Item, Equip, Community, Event, Rank
- System button opens: Channel, Game Option, Quit, Joypad, Key Setting, Option, Room Change, System Option
- Sub-panel buttons manually positioned above the bar (NX origins are invalid for this version)
- Custom dark background drawn behind each panel
- `is_in_range` extended to cover sub-panels and full screen bottom
- Post-BB buttons (MonsterBattle, MonsterLife, MSN, AfreecaTV, EpisodBook) disabled
- Each sub-panel button wired to its respective UI action

### Chat System
- UIChatBar draws NX textures at correct StatusBar anchor position
- Text input field aligned with the visual chat input box
- Chat background, messages, and text field properly positioned
- Open/close chat button works correctly (toggle bounce fixed)
- Chat target button no longer gets stuck
- Enter key correctly opens chat and focuses the text input field
- Writing indicator (blinking cursor) visible inside the chat box when typing
- Stale focusedtextfield pointer no longer traps keyboard input
- Message history with up/down arrows

### Quest Log wip
- Detail panel showing quest name, level requirement, NPC sprite, rewards with item icons, requirements, and description
- Accept, Complete, and Forfeit quest actions with proper outgoing packets
- NPC marker button to highlight quest NPCs on the minimap
- Text formatting with color code stripping and line break handling

### Buddy List
- Full buddy list UI with select, delete, whisper, and add actions
- Delete buddy sends confirmation dialog then DeleteBuddyPacket
- Whisper sends FindPlayerPacket for online buddies

### Skill Macros
- Full skill macro editor UI with 5 macro slots
- Each slot has a name textfield, shout checkbox, and 3 skill icon slots
- Populated from server-sent SKILL_MACROS data
- Save sends SkillMacroModifiedPacket (opcode 92)

### Buff Tooltip on Hover
- Hovering over buff icons in the top-right corner shows the skill/item name as a tooltip
- Tooltip clears when cursor leaves the buff area

### Player Death wip
- Player enters DIED state when HP reaches 0 (immovable, invincible)

### Expression / Face Fix
- Fixed unsigned integer underflow in `Expression::byaction()` that caused spam log output for large action values




### Soft Keyboard (PIC Entry)
- Clean 3-column number grid layout (1-9, 0, Del)
- Cancel/OK buttons, draggable window
- Numbers-only PIC support

### Character Select Screen
- Characters centered on screen with proper name tag positioning
- Start button repositioned
- Create/Delete character functionality

### UI Improvements
- Bottom row button overlap fixed (CashShop, MTS, Menu, System) with position offsets
- UIChannel uses actual world/channel from Configuration
- Lost items/fame status messages implemented
- World select draw order fixed

## Recent Changes

### GM Dark Sight Transparency
- GMs with DARKSIGHT buff render at 50% opacity instead of being fully hidden
- Local player sees themselves semi-transparent when hidden via `has_buff(DARKSIGHT)`
- Other GMs see hidden GMs as semi-transparent via `GiveForeignBuff` DARKSIGHT bit detection
- `CancelForeignBuff` properly restores full opacity when dark sight ends

### Storage System
- Full storage packet parsing for Cosmic v83 (open, take-out, store, arrange, meso)
- Fixed packet stack underflow errors (modes 0x09/0x0D don't include meso field, mode 0x0F mask is writeByte not writeShort)
- Storage UI with item grid, tab switching, meso deposit/withdraw
- Fixed item replication on drag-drop between inventory and storage panels
- Right panel item positioning aligned with left panel

### OtherChar / Bot Movement
- Smooth interpolation for other characters instead of snapping between positions
- One movement consumed per frame with lerp fill when queue is empty
- Proper hspeed/vspeed set during interpolation so walking animation plays
- Removed update_fh() calls that caused warping to wrong footholds

### Window Dragging
- Fixed UIDragElement dragging using mouse held state and dragarea values
- All draggable UI windows properly respond to click-and-drag

### Fullscreen Support
- Borderless windowed fullscreen stretches to fill any monitor resolution (including 16:10)
- Cursor mapping adjusted for viewport scaling
- Works across all screens (login, world select, character select, gameplay)

### Skill Book Fixes
- Fixed Cygnus Knights (Noblesse) job classification and skill display
- Fixed NX path lookup for 4-digit job IDs (1000+)
- SP spending works directly from "+" button (no confirmation panel)
- Beginner SP calculated client-side correctly
- Blessing of the Fairy correctly excluded from SP upgrades
- Mouse wheel scrolling support
- Filtered out event/mount skills that shouldn't be visible

### Button & Input Fixes
- Global button click detection fix -- buttons no longer require MOUSEOVER state before clicking
- Fixed double-click bug where buttons fired twice per click
- Fixed cursor position updates re-triggering click events

### Sound Effects
- Added level up sound
- Added player damage/hurt sound
- Added quest complete sound

### UI Fixes
- Hidden quickslot/numpad sprites from status bar
- Key config UI uses English NX nodes where available

## What's Left To Do

### Known Bugs
- **Mob knockback janky** -- mobs appear to move during their knockback/hit animation instead of being locked in place
- **Jump-down needs more vertical lift** -- jumping down from a platform drops immediately instead of a slight vertical lift before descent
- **Item drops off-center** -- drops land off-center from the character (server-controlled positioning, not a client bug)
- **Key config UI has Korean buttons** -- NX sprites for some buttons contain Korean text

### Not Yet Implemented
- **Cash Shop purchases** -- UI exists but purchase flow incomplete
- **Storage UI** -- functional but may need polish (item positioning, edge cases)
- **Trade UI** -- player-to-player trade stubbed
- **Guild UI** -- stub files created, not wired up
- **Messenger / Party Search** -- stub files created, not wired up
- **Monster Book** -- stub files created, not wired up

### UI Stubs Created (Not Yet Functional)
UIGuild, UIGuildBBS, UIGuildMark, UIMessenger, UIPartySearch, UIMonsterBook, UIMonsterCarnival, UIHiredMerchant, UIPersonalShop, UIMinigame, UIRPSGame, UIMapleTV, UIMapleChat, UISocialChat, UIFarmChat, UIFamily, UIWedding, UIRanking, UIQuestHelper, UISystemOption, UIChatWindow

## Building

### Requirements
- Visual Studio 2022
- Windows SDK
- CMake 3.15+
- Dependencies: GLFW, GLEW, FreeType, Bass audio, NoLifeNx, stb_image

### Build Steps
```bash
# Configure
cmake -S . -B build

# Build
cmake --build build --config Debug --target OpenStory

# Output: wz/OpenStory.exe
```

### NX Files
Place your v83 NX files in the `wz/` directory alongside the executable.

## Configuration

Edit `Configuration.h` for default settings. A `Settings` file is generated after a game session.

Key settings:
- **ServerIP / ServerPort** -- server connection details
- **Width / Height** -- screen resolution (default 1024x768)
- **BGMVolume / SFXVolume** -- audio levels (0-100)
- **VSync** -- vertical sync toggle
- **SaveLogin** -- remember last account name

## Required NX Files

*Check `NxFiles.h` for the full list.*

All NX files should be v83 GMS conversions placed in the `wz/` directory.

## Dependencies

| Library | Purpose |
|---------|---------|
| [NoLifeNx](https://github.com/ryantpayton/NoLifeNx) | NX file reading |
| [GLFW3](http://www.glfw.org/) | Window/input management |
| [GLEW](http://glew.sourceforge.net/) | OpenGL extensions |
| [FreeType](http://www.freetype.org/) | Font rendering |
| [Bass](http://www.un4seen.com/) | Audio playback |
| [Asio](http://think-async.com/) | Networking |

## Credits

### Original Authors
- **Daniel Allendorf** -- Original Journey/HeavenClient creator
- **Ryan Payton** -- Co-developer of the continued Journey client

### Based On
- [HeavenClient](https://github.com/HeavenClient/HeavenClient) -- The original open-source C++ MapleStory client

### OpenStory Contributors
- **rdiol12** -- v83 Cosmic server compatibility, opcode realignment, MTS, UI systems, packet handlers, physics fixes

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See [LICENSE](LICENSE) for details.
