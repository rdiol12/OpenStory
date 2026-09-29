# OpenStory

## PS5 port

The experimental native PS5 port provides controller navigation, native keyboard entry, server selection and streamed NX loading. See the [PS5 build and play guide](https://github.com/rdiol12/OpenStory/blob/ps5/platform/ps5/README.md).

## Features

- Full v83 client connecting to Cosmic servers
- NX file-based assets, OpenGL rendering, GLFW windowing
- Login flow, character select, world/channel select, PIC entry
- Combat, skills, buffs, inventory, quests, NPCs, chat
- Storage, buddy list, skill macros, gamepad support
- Distance-based (spatial) sound for world events
- Fullscreen, UI scaling, drag-and-drop windows

### Text & input

- Fonts are **compiled into the binary** (Roboto + Noto Sans Hebrew) — the client needs no
  font files at runtime and does not depend on `C:/Windows/Fonts`, so glyphs never
  silently vanish on another machine.
- **Right-to-left text** (Hebrew): Unicode bidi reordering, right-aligned wrapping, and a
  caret that tracks the visual position rather than the byte offset.
- **Unicode input** via the OS keyboard layout, with character-safe editing — backspace,
  delete and the arrow keys move whole characters, not bytes.
- Inline icons in chat (`#v/#i/#q/#s/#f/#e`) and an emoticon picker on the chat bar.

Non-ASCII text requires the server to use UTF-8. On Cosmic that is
`CHARSET: UTF-8` in `config.yaml`; note its `CharsetConstants` whitelist must contain
UTF-8 or it silently falls back to US-ASCII and mangles everything to `?`.

### Custom / experimental

Some features are client-side additions not supported by a stock Cosmic server:

| Feature | Status | Notes |
|---------|--------|-------|
| Event System | WIP | Custom `EVENT_INFO` / `REQUEST_EVENT_INFO` packets; needs a server-side handler |
| HP/MP Warning | Working | Client-only |
| Graphics/Effects Quality | Working | Client-only |

## Building

For Apple Silicon macOS, see the [macOS build and app bundle guide](resource/macos/README.md).

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

Fonts are compiled in via the generated `src/Graphics/EmbeddedFonts.{h,cpp}`, which are
committed — no extra build step.

## Configuration

Supply compatible v83 NX files in `wz/`.
Create a local `Settings` file beside the client executable and set `ServerIP`
and `ServerPort` to your Cosmic server's reachable address and login port.
The default address is loopback and does not select a public server.

```ini
ServerIP = 127.0.0.1
ServerPort = 8484
SaveLogin = false
```

Start the server,
open the client, then log in with an account created on that server. On PS5,
enter the address on the server selection screen. Display preferences such as


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
