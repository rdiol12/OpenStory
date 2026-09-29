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
`/download0/openstory`. Preserve `ClientID.txt` across updates. Packaged assets
load from `/app0/wz`; assets in the writable game directory take precedence.

## Build the native application

Use Ubuntu or WSL with Clang/LLD/LLVM 18, CMake, Python 3 and zlib development
headers. Place PS5Library beside OpenStory and build its native application
tooling first. OpenStory uses its startup code, allocator, conversion tools
and `ps5/build/native-app/dist/PPSA99051/sce_module/libc.prx`.

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

Optional build arguments:

- `--update-origin http://<UPDATE-PC-IP>:3150` embeds the native update feed.
- `--selection-audio /path/to/snd0.at9` adds stereo 48-kHz ATRAC9 Home-menu music.
- `--log-host <LOG-PC-IP>` sends application diagnostics to UDP port `9978`.

Supply your own local values for the placeholders. Keep them out of commits.

## Prepare assets and settings

Supply compatible v83 NX data. Required names are listed in
`src/Util/NxFiles.h`. Stage the v83 UI file as **`UI.nx`** and exclude the
separate `UI_83.nx` file. Use separate source and staging directories.

Prepare the packaging representation in a new output directory:

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
writable settings on first launch; updates preserve the saved configuration.

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

## Build and publish an update

Run the supported native-update CLI from PS5Library. Set its public URL to an
address reachable from the console. Increase the source `contentVersion` and
retain the exact installed full or remastered package as the reference.

```powershell
npm run native:update -- configure --title-id PPSA99783 --source C:/path/to/source --update-origin http://<UPDATE-PC-IP>:3150
npm run native:update -- prerequisites --title-id PPSA99783 --source C:/path/to/source
npm run native:update -- build --title-id PPSA99783 --source C:/path/to/source --executable C:/path/to/eboot.elf --output C:/releases/OpenStory-update.pkg --reference C:/releases/installed.pkg --base-version <INSTALLED-VERSION>
npm run native:update -- publish --title-id PPSA99783 --package C:/releases/OpenStory-update.pkg.remastered.pkg --delta C:/releases/OpenStory-update.pkg.delta.pkg --base-version <INSTALLED-VERSION> --icon C:/path/to/source/sce_sys/icon0.png --data-dir C:/path/to/active-server-data
npm run native:update -- status --title-id PPSA99783 --data-dir C:/path/to/active-server-data
```

Use the running server's data directory for publication. On PS5, select
**OpenStory → Options → Check for Update**. The updater address must remain
reachable from the console.

## Verify and collect logs

Run `python tests/ps5-containers.py` and `python tests/ps5-presentation.py` for
container and presentation checks. `tests/ps5-assets.py` extracts packaged
payloads and compares them with staged inputs; use a new scratch directory.
Verify the packaged executable against its matching native ELF as well.

To receive application logs from a diagnostic build:

```powershell
python -u platform/ps5/receive-log.py --bind <LOG-PC-IP> --ps5 <CONSOLE-IP> --output build/startup-lan.log
```

Allow UDP port `9978` from that console in the receiving PC's firewall. The
receiver is time- and size-limited. The console also writes
`/download0/openstory-startup.log`. Keep diagnostics local.

The [Cosmic runtime guide](cosmic-runtime/README.md) covers the separate,
experimental Java runtime port; it does not provide a playable console server.
