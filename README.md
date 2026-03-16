# OpenStory
support for 1920x1080 is wip


A v83 MapleStory client built for Cosmic/private servers. Forked from [HeavenClient](https://github.com/HeavenClient/HeavenClient), rewritten and extended for full v83 compatibility.

## Features

- Full v83 MapleStory client connecting to Cosmic v83 servers
- NX file-based asset loading (UI, sprites, maps, items)
- OpenGL rendering with GLFW windowing
- Complete login flow: account login, world/channel select, character select, PIC entry
- In-game systems: movement, combat, NPCs, quests, inventory, skills, chat

## What Was Fixed / Added

### Recv Opcode Realignment
All recv opcodes realigned to match Cosmic's `SendOpcodes.java`. Every packet handler now uses the correct v83 Cosmic opcode values.

### Packet Handlers
- **Login**: LOGIN_RESULT, SERVERSTATUS, SERVERLIST, CHARLIST, CHARNAME_RESPONSE, ADD_NEWCHAR_ENTRY, DELCHAR_RESPONSE, SERVER_IP, CHANGE_CHANNEL, PING, RECOMMENDED_WORLDS, CHECK_SPW_RESULT, RELOG_RESPONSE
- **Player**: CHANGE_STATS, GIVE_BUFF, CANCEL_BUFF, FORCED_STAT_SET, RECALCULATE_STATS, UPDATE_SKILL, ADD_COOLDOWN, SHOW_STATUS_INFO, FAME_RESPONSE
- **Map/Field**: SET_FIELD, FIELD_EFFECT, BLOW_WEATHER, CLOCK, SKILL_MACROS, KEYMAP, QUICKSLOT_INIT, SCRIPT_PROGRESS_MESSAGE
- **Inventory**: MODIFY_INVENTORY, GATHER_RESULT, SORT_RESULT, SCROLL_RESULT, SHOW_ITEM_GAIN_INCHAT
- **NPCs**: SPAWN_NPC, SPAWN_NPC_C, NPC_ACTION, NPC_DIALOGUE, OPEN_NPC_SHOP, CONFIRM_SHOP_TRANSACTION
- **Mobs**: SPAWN_MOB, SPAWN_MOB_C, MOB_MOVED, SHOW_MOB_HP, KILL_MOB, CATCH_MONSTER
- **Drops/Reactors**: DROP_LOOT, REMOVE_LOOT, SPAWN_REACTOR, HIT_REACTOR, REMOVE_REACTOR
- **Other Players**: SPAWN_CHAR, REMOVE_CHAR, CHAT_RECEIVED, CHAR_MOVED, UPDATE_CHARLOOK, SHOW_FOREIGN_EFFECT, GIVE_FOREIGN_BUFF, CANCEL_FOREIGN_BUFF, UPDATE_PARTYMEMBER_HP, GUILD_NAME/MARK_CHANGED, CANCEL_CHAIR, SHOW_ITEM_EFFECT, SPAWN_PET
- **Combat**: ATTACKED_CLOSE, ATTACKED_RANGED, ATTACKED_MAGIC
- **Social**: SERVER_MESSAGE, BUDDY_LIST, PARTY_OPERATION, FAMILY, PLAYER_HINT, UPDATE_QUEST_INFO, CHAR_INFO, WEEK_EVENT_MESSAGE
- **Cash Shop / MTS**: SET_CASH_SHOP, SET_ITC, CS_OPERATION, MTS_OPERATION, MTS_OPERATION2
- Unhandled packet logging to `unhandled_packets.txt` for debugging

### Maple Trading System (MTS)
- Full client-side MTS implementation (auction house)
- Outgoing packets: EnterMTS, Sell, ChangePage, Search, CancelSale, Transfer, AddCart, RemoveCart, Buy, BuyFromCart
- Incoming handlers: item listing pages, confirm sell/buy/transfer, transfer inventory, not-yet-sold inventory, fail buy, cash balance
- SET_ITC handler for MTS transition with character info parsing
- Dark panel UI with Browse/My Sales/Cart tabs, item lists, pagination, buy/sell/search

### Status Bar — Menu & System Sub-Panels
- Menu button opens a vertical panel with: Stat, Skill, Quest, Item, Equip, Community, Event, Rank
- System button opens: Channel, Game Option, Quit, Joypad, Key Setting, Option, Room Change, System Option
- Sub-panel buttons manually positioned above the bar (NX origins are invalid for this version)
- Custom dark background drawn behind each panel
- `is_in_range` extended to cover sub-panels and full screen bottom
- Post-BB buttons (MonsterBattle, MonsterLife, MSN, AfreecaTV, EpisodBook) disabled
- Each sub-panel button wired to its respective UI action

### Chat System
- UIChatBar draws NX textures at correct StatusBar anchor position (512, screen_h)
- Text input field aligned with the visual chat input box
- Chat background, messages, and text field properly positioned
- Removed duplicate chat rendering from UIStatusBar
- Enter key toggles chat input, message history with up/down arrows

### Quest Log
- Detail panel showing quest name, level requirement, NPC sprite, rewards with item icons, requirements, and description
- Accept, Complete, and Forfeit quest actions with proper outgoing packets
- NPC marker button to highlight quest NPCs on the minimap
- Text formatting with color code stripping and line break handling

### Buddy List
- Full buddy list UI with select, delete, whisper, and add actions
- Delete buddy sends confirmation dialog then DeleteBuddyPacket
- Whisper sends FindPlayerPacket for online buddies

### Physics — Traction Fix
- Foothold traction values properly applied to character movement
- Ice/slippery maps (El Nath, etc.) work correctly without freezing or getting stuck
- Traction resets on map change

### Hair — backHairOverCape Layer
- Long hair styles with `backHairOverCape` frames render correctly over capes
- No extra layer drawn for short hair or when no cape is equipped

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

## What's Left To Do

### Not Yet Implemented
- **Cash Shop purchases** — UI exists but purchase flow incomplete
- **Storage UI** — handler/UI stubbed but not fully wired
- **Trade UI** — player-to-player trade stubbed

### Known Issues
- NPC interaction fallback when dialog not found
- Hurricane/piercing arrow/rapidfire attack packets may need additional 4 bytes

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
- **ServerIP / ServerPort** — server connection details
- **Width / Height** — screen resolution (default 1024x768)
- **BGMVolume / SFXVolume** — audio levels (0-100)
- **VSync** — vertical sync toggle
- **SaveLogin** — remember last account name

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
- **Daniel Allendorf** — Original Journey/HeavenClient creator
- **Ryan Payton** — Co-developer of the continued Journey client

### Based On
- [HeavenClient](https://github.com/HeavenClient/HeavenClient) — The original open-source C++ MapleStory client

### OpenStory Contributors
- **rdiol12** — v83 Cosmic server compatibility, opcode realignment, MTS, UI systems, packet handlers, physics fixes

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See [LICENSE](LICENSE) for details.
