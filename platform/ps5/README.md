# OpenStory on PS5

OpenStory is a native homebrew client for a compatible PS5 environment. It
connects to a MapleStory v83 server such as Cosmic. Supply your own game data
and server.

## Play

1. Install the OpenStory package using your homebrew package installer.
2. Start a compatible server and create an account on that server.
3. Launch OpenStory. Enter the server's IPv4 address and login port (usually
   `8484`), then choose **Connect**. Choose **Offline preview** to view the
   login screen without a server.
4. Activate the account and password fields to open the native keyboard,
   then log in and choose a world, channel and character.

The server's login, channel and cash-shop addresses must all be reachable from
PS5. Use your server's address; loopback refers to the console itself.

### Controller

| Control | Action |
|---|---|
| D-pad / left stick | Move focus within the active window |
| Cross | Activate the focused control |
| Circle | Close the focused menu; continue offline from server selection |
| R1 | Switch between open menus |
| L1 | Change tabs |
| L2 / R2 | Scroll the focused window |
| Triangle | Open the keyboard for the focused text field |
| Options | Open the game menu |
| Touchpad | Talk to a nearby NPC |

Modal dialogs retain focus until dismissed. Jump, attack, pickup and menu
bindings can be changed through the game's key configuration.

Settings, `ClientID.txt`, screenshots and `startup.log` are stored under
`/download0/openstory`. Keep `ClientID.txt` with the saved settings. Packaged assets
load from `/app0/wz`; assets in the writable game directory take precedence.

## Build the native application

Use Ubuntu or WSL with Clang/LLD/LLVM 18, CMake, Python 3 and zlib development
headers. The build also requires PS5Library's native application tooling,
which is a separate dependency. Place that checkout beside OpenStory and build
its native tooling first. OpenStory uses its startup code, allocator,
conversion tools and `ps5/build/native-app/dist/PPSA99051/sce_module/libc.prx`.

Install these dependencies under `build/deps`:

| Dependency | Version | Archive SHA-256 |
|---|---|---|
| ps5-payload-sdk | v0.43 | `a9cc9929f21b2b2c5d5b309f3bab4997067c45281c0622cf4838b1aecba66fcb` |
| PacBrew ps5-payload-dev | v0.40.2 | `a85f65de418a8e6a898c6c3e3c870d50fff7618a200e4dd59ea9692af6ecec4d` |
| blackbearreloaded/ps5-opengl SDK | v0.3.0 | `a7bd6b85f00398eaf8d87ecc58fa0bde7cab79e065acc783268070d2f14b403c` |

Merge PacBrew's `target` tree into the public SDK and install standalone Asio
headers under `target/user/homebrew/include`.

```sh
python3 platform/ps5/build.py --ps5library ../PS5Library \
  --sdk build/deps/ps5-payload-sdk \
  --opengl build/deps/ps5-opengl-sdk-0.3.0 \
  --version 01.000.001
```

`--build-dir` selects a separate output directory. The application folder is
`build/ps5/dist/PPSA99783`. The linked `build/ps5/native/eboot.elf` is a native
application executable, not an ELF-loader payload.

To include Home-menu music, add `--selection-audio /path/to/snd0.at9` with a
stereo 48-kHz ATRAC9 file.

## Prepare assets and settings

Supply compatible NX game data. Required filenames are listed in
`src/Util/NxFiles.h`. Use separate source and staging directories, and prepare
the packaging representation in a new output directory:

```sh
python3 platform/ps5/pack_assets.py /path/to/nx build/ps5/dist/PPSA99783/wz build/ps5/asset-wrapper.json
python3 tests/nx-loading.py build/ps5/dist/PPSA99783/wz /path/to/nx
```

Place a local `Settings` file beside `eboot.bin`:

```ini
ServerIP = 127.0.0.1
ServerPort = 8484
SaveLogin = false
OfflinePreview = true
```

Select the actual server in the startup UI. Packaged settings initialize the
writable settings on first launch.

## Build a full package

On Windows, install .NET 10 and configure your local packaging toolkit. Create
this title's `sce_sys/keystone` with the toolkit's `ks_create` command. Apply
[the inode-offset patch](package/inode-offset.patch) to the PS5Library
LibProsperoPkg dependency before building.

```powershell
python platform/ps5/package.py --toolkit C:/path/to/toolkit --ps5library ../PS5Library
```

The output is `build/ps5/OpenStory-PS5.pkg`. The package identity is
`PPSA99783` / `UP9000-PPSA99783_00-OPENSTORYPS50000`. Build records and verification
logs stay in the ignored build directory. Retail-format metadata checks and
homebrew installation checks are separate; inspect the generated results.

Install the resulting full package using your homebrew package installer,
then follow the [play instructions](#play).
