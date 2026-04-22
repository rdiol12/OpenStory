# OpenStory

A v83 MapleStory client for Cosmic/private servers. Forked from [HeavenClient](https://github.com/HeavenClient/HeavenClient).

**Status: Playable** — functional for gameplay, UI polish ongoing.

## Screenshots

### Status Bar - wip  ui need fixing
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
- **Storage UI (v83 FullBackgrnd)**: Rebuilt the two-panel storage window. Animated NPC keeper portrait (picks the state with the most frames), player CharLook portrait at 0.75x over the inventory pane, correct 10 × 6 left / 12 × 6 right cube grid aligned to the backdrop art. Free-cube placement — drag items between cubes in the same panel and per-itemid preferences persist across refreshes. Multi-select (double-click toggles a highlight using the inventory "new item" animated sprite), Retrieve button acts on the full selection and falls back to the cube under the cursor. Partial take-out: double-clicking a stackable opens a quantity prompt that takes the full stack server-side then auto-stores the excess back (v83 has no quantity field on take-out). Meso icon/labels repositioned per panel; left panel shows storage amount, right panel shows player wallet.
- **Shop UI**: Shopkeeper NPC now animates (`Animation` constructor, picks the state with most frames). Scroll bounds fixed — the buy/sell lists use 9 visible rows (matching the actual draw loops) instead of 5, so you can no longer scroll into empty space. Buy-side category tabs added — finalize_buy_tabs builds only the tabs for categories this shop actually carries (OVERALL shows everything). Uses the inventory tab sprites, positioned next to the original OVERALL tab via its sprite origin. Click-route bug fixed where DISABLED-but-active AP buttons swallowed clicks. Buy/Sell quantity prompt (`UIEnterNumber`) and single-item confirmation (`UIYesNo`) get their own `field_y_offset` / `button_x_offset` / `button_y_offset` parameters so the shop can lay them out independently of the storage / meso popups. Animated meso coin overlaid at each visible buy-item price row (loaded from `UIWindow2.img/Shop2/meso`). Char portrait uses `flipped=false` so the character faces the NPC.
- **Skill Book Macro panel (v83 inline)**: Row-selection uses the `UIWindow2.img/Skill/macro/select` sprite behind the active row. Shout checkbox state rendered with the real 6×6 `check` NX sprite (was a placeholder ColorBox). Per-row macro icons from `UIWindow.img/SkillMacro/Macroicon/0..4/icon` at the right end of each row — draggable handle. Name textfield focuses immediately when the macro panel opens and sits on the baked input rect. Fixed duplicate `HONOR` assignment that blanked weapon-avoidability every frame. Preview slot left of the name field shows the selected row's macro icon. Macro drag-to-KeyConfig wired end-to-end: new `MacroIcon` drops a `Mapping(KeyType::MACRO, idx)`, `UIKeyConfig` renders macro icons via a new `load_macro_icons` path, and drop-on-quickslot added via `Icon::IconType::MACRO`. Pressing the bound hotkey routes through `UIStateGame::send_key` → `Stage::send_key` case `MACRO` → `UISkillBook::trigger_macro(idx)` which queues the 3 skills and drains them one-per-tick when `player.can_attack()` — otherwise only the first skill fires because the animation lock blocks the others. Shout flag dispatches the macro name as a `GeneralChatPacket` on trigger.
- **UINotice drag fix**: `UIEnterNumber` and `UIEnterText` were calling `UIElement::send_cursor` instead of `UIDragElement::send_cursor`, so their popups weren't draggable. Swapped the base call. Separately, removed the re-centering transform in `UINotice` constructor that was reading the saved top-left as a center on every open and recentering — this made the popup drift up-and-left each session until it vanished off screen. Default `PosNOTICE` updated to a sensible centered top-left.
- **Scrolling server announcement**: Long notices were wrapping to a second line that spilled below the 23-px black bar. Textfield's implicit `maxwidth=0` falls back to 800 px in `GraphicsGL::createlayout` — set explicit 32767 so the notice stays on one line as the marquee scrolls.
- **Chat starts closed**: `UIChatBar::UIChatBar` initializes `chatopen = false` so the chat log panel doesn't cover the screen on login until the player explicitly opens it.
- **Status Bar**: Rebuilt/polished status bar UI (HP/MP/EXP, character menu, chat controls). See screenshot above.
- **HP/MP/EXP flash animations**: The HP and MP gauges now play their animated low-resource overlay (`ani_hp_gauge` / `ani_mp_gauge`, plus the AB-job variant `ani_hp_gauge_ab`) when the bar drops below 30%. The EXP gauge and the notice icon pulse via alpha-cycled draws so status changes are visible at a glance.
- **NPC Dialog reliability**: Fixed intermittent "NPC chat UI doesn't open" bug. Root cause was `UIStateGame::remove` calling `unique_ptr::release()` (which leaks instead of freeing), leaving a null entry that wedged the next dialog. Handler also now force-removes stale dialog before opening a new one.
- **NPC Quest Indicator**: Tightened the "available quest" bulb above NPCs so it only shows for quests the player can actually pick up (filters out auto-start, auto-pre-complete, blocked, script-started, and quests with no area / no info entry).
- **In-game Channel Switcher**: Now draws only the real channels reported by the server for the current world, instead of hard-coding 20 slots. Login server list response populates a `ChannelLoadData` cache that `UIChannel` reads at construction.
- **TestingHandlers refactor**: Split the 1200+ line catch-all TestingHandlers into proper domain files: WeatherHandlers, ClockHandlers, FieldHandlers, UIControlHandlers, QuestHandlers, MiscHandlers. Moved login handlers to LoginHandlers, fame to PlayerInteractionHandlers, cash shop handlers to CashShopHandlers, hammer/vega to InventoryHandlers.
- **Fame/Defame fix**: Fixed fame buttons sending character ID instead of map object OID (server silently dropped the packet). Also fixed FameResponseHandler reading fame value as int instead of short.
- **UIClock NX sprites**: Clock UI now uses NX-based sprites instead of programmatic drawing.
- **Event System (Custom - WIP)**: Live event list UI using NX EventList sprites. Server sends events via EVENT_INFO (0xC3) packet, client requests via REQUEST_EVENT_INFO (0xF1). Shows event name, description, status (In Progress/Ended), item rewards with tooltips, and activates UIClock countdown for the first active event. *Custom protocol -- not supported by Cosmic server out of the box.*
- **UIOptionMenu overhaul**: Full settings panel with working sliders for BGM/SFX volume, HP/MP warning thresholds, graphics/effects quality. Slider percentage labels displayed. Menu always centered on screen.
- **HP/MP Warning overlays**: Screen flashes red when HP drops below threshold, blue for MP. Configurable via UIOptionMenu sliders.
- **Graphics/Effects Quality**: Slider controls limit concurrent effects and conditionally skip mists, foregrounds, environments, and weather based on quality setting.
- **System menu cleanup**: Reduced to 6 buttons, removed duplicates (MonsterLife, SystemOption, RoomChange). Game Option opens UIGameSettings, Option opens UIOptionMenu.
- **Minimap fixes**: Fixed simple mode breaking minimap open (uninitialized mapid), added side-edge resizing.
- **Monster Book UI -i i based this system on some pictures and videos i saw if you have a better reference open a ticket and i will adjust**: Card grid, detail overlay with animated mob sprite, drop items with tooltip on hover, and stats display. Click cards to view details, click sprite area to cycle stand/move/die animations. Tabs for category filtering and search. Home/card view works. *Note: Set Effect menu UI is still broken — layout and positioning need polish.*
- **Login Remember Me**: The "Save ID" checkbox on the login screen now actually remembers your login credentials between sessions.
- **Idle HP/MP regen (HEAL_OVER_TIME)**: Client now sends regen packets every ~10 seconds while idle. Formula: HP = level/5 + 2, MP = level/5 + INT/20 + 3. Mage Improving MP Recovery skill adds Level x SkillLevel / 10 extra MP. Chair sitting gives 3x bonus. Regen pauses during attacks, hit stun, invincibility, and death.
- **Heal floating numbers**: HP regen shows blue floating numbers above the character using the BasicEff heal number sprites.
- **Chair rendering and animation**: Fixed chair sprite rendering and sit animation when using inventory chair items.

### Custom Features (Not Cosmic-supported -- WIP)
| Feature | Status | Notes |
|---------|--------|-------|
| Event System | WIP | Custom EVENT_INFO/REQUEST_EVENT_INFO packets. Needs server-side handler in Cosmic. |
| HP/MP Warning | Working | Client-only, no server changes needed |
| Graphics/Effects Quality | Working | Client-only, no server changes needed |

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

- **Add a quest**: Open the Quest Log (Q), go to In-Progress, click and drag a quest into the Quest Helper or doable click on opened quest 
- **Remove a quest**: Click the X button next to the quest name
- **Reorder**: Drag a quest name up or down within the Quest Helper
- **Collapse/Expand**: Click a quest name to toggle its requirements
- **Auto-track**: Click the AUTO button to fill the helper with your active quests

## License

GNU Affero General Public License v3. See [LICENSE](LICENSE).
