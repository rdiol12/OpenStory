# OpenStory

quest system is fully implemented 

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

### Latest
- **Monster Book UI**: Card grid, detail overlay with animated mob sprite, HP/MP gauge bars, drop items, and stats display. Click cards to view details, click sprite area to cycle stand/move/die animations. Tabs for category filtering and search. *Note: Monster Book UI is broken but functional — layout and positioning still need polish.*
- **Idle HP/MP regen (HEAL_OVER_TIME)**: Client now sends regen packets every ~10 seconds while idle. Formula: HP = level/5 + 2, MP = level/5 + INT/20 + 3. Mage Improving MP Recovery skill adds Level × SkillLevel / 10 extra MP. Chair sitting gives 3x bonus. Regen pauses during attacks, hit stun, invincibility, and death.
- **Heal floating numbers**: HP regen shows blue floating numbers above the character using the BasicEff heal number sprites.
- **Chair rendering and animation**: Fixed chair sprite rendering and sit animation when using inventory chair items.

### Recent Fixes
- **Quest complete effect**: Fixed QuestClear animation path (`Quest/clear` -> `QuestClear`) to match NX data; quest complete light pillar now shows from both SHOW_STATUS_INFO and QUEST_CLEAR handlers
- **Three Snails ammo check**: Skill now blocked if player lacks required snail shell items, preventing ghost MP drain
- **Packet handler fixes (Cosmic compatibility)**: Fixed critical packet format mismatches causing stream corruption:
  - SpawnMistHandler: added mist_type/ownerId fields, fixed box coords from short to int
  - MoveMonsterResponseHandler: fixed field types (bool/short/byte/byte)
  - HitReactorHandler: fixed stance read type and added missing fields
  - MonsterBookCardHandler: added missing full-flag byte
  - UpdateSkillHandler: added trailing byte consumption
  - CharInfoHandler: changed pet parsing from count-based to sentinel-based
  - MessengerHandler: remapped mode values to match Cosmic (chat=0x06, invite=0x03)
  - FamilyChartResultHandler: reads all 12 pedigree entry fields
  - FamilyInfoResultHandler: reads trailing entitlement data
  - SpawnReactorHandler: fixed trailing read from string to short
- **Skills buff and animation**: Now showing correctly
- **Skill sounds**: Fixed duplicate sample bug preventing skill sounds from playing
- **Quest system**: Completed quests now properly removed from active list; NPC quest bubbles refresh on state changes
- **Quest packets**: Full v83 quest handler coverage -- QUEST_CLEAR effect/sound, UPDATE_QUEST_INFO error sub-types (0x0A-0x0F), scripted quest action packets
- **EXP gain**: Fixed "not handled" message appearing for in-chat EXP notifications
- **SHOW_ITEM_GAIN_INCHAT**: Proper v83 mode switch for all 24 effect types
- **Notice button**: Changed from broken button to static sprite matching v83 client
- **Snow effect**: Added snow effect (WIP — needs reference for all MapleStory effects)
- **Quest Helper**: Rewritten to support multi-quest tracking (up to 5), collapsible entries, per-quest close button (BtDelete sprite), live item/mob progress from server quest data, auto-refresh on quest updates
- **Quest drag-and-drop**: Drag quests from the Quest Log into the Quest Helper to track them; drag tracked quests to reorder
- **Chat clipping**: Chat messages no longer render above the black chat background




## Quest Helper

Track up to 5 quests at once with live progress updates.

- **Add a quest**: Open the Quest Log (Q), go to In-Progress, click and drag a quest into the Quest Helper
- **Remove a quest**: Click the X button next to the quest name
- **Reorder**: Drag a quest name up or down within the Quest Helper
- **Collapse/Expand**: Click a quest name to toggle its requirements
- **Auto-track**: Click the AUTO button to fill the helper with your active quests

## License

GNU Affero General Public License v3. See [LICENSE](LICENSE).
