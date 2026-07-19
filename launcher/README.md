# GenMs Launcher

> ⚠️ **Very much WIP** — the client and server are under heavy active development; expect rough edges.

A single standalone Windows `.exe` that players run first: it shows a branded
window, syncs the game to the latest build from two manifests, then
launches the client.

## Live server (try it)

This launcher powers **GenMs**, a live MapleStory v83 test server with AI-generated
content. It's safe to point at if you want to see the client and update flow working:

- Game server: `genms.fun` (`72.60.176.12`), port `8484`
- Update server: `https://72.60.176.12/updates` (game data + client manifests)

The defaults in `config.ini` already point there — build, drop the exe next to
`config.ini`, and press PLAY.

- **Self-contained** — one `AugurMS.exe`, no installer, no .NET install, no admin.
- **Incremental** — hashes local files; downloads only what changed (most launches: nothing).
- **Safe** — downloads to a temp file, verifies SHA-256, then atomically swaps it in. A half-finished download never replaces a working file. Failed downloads retry up to 3×.
- **Self-signed TLS** — certificate validation is relaxed **only** for host `72.60.176.12`; every other host is validated normally.
- **Offline-friendly** — if the update server is unreachable it shows a friendly notice and still lets you Play.

---

## Build

Requires the **.NET 8 SDK** (https://dotnet.microsoft.com/download/dotnet/8.0).

```bat
cd AugurLauncher
dotnet publish -c Release
```

The single-file, self-contained build lands at:

```
bin\Release\net8.0-windows\win-x64\publish\AugurMS.exe
```

Everything (RID `win-x64`, single-file, self-contained, compression) is already set
in `AugurLauncher.csproj`, so `dotnet publish -c Release` is all you need.

> Want a *much smaller* exe that requires the .NET 8 Desktop Runtime to be installed
> on the player's PC? Publish framework-dependent instead:
> `dotnet publish -c Release -p:SelfContained=false -p:PublishSingleFile=true`

---

## Deploy

Put these **three files together**, in your game's root folder:

```
<game root>\
    AugurMS.exe        <- the launcher (this project)
    config.ini         <- edit paths/URLs
    logo.png           <- your branding (provided)
    ... game files ...
```

Then edit `config.ini`:

- `ClientDataDir`    — folder with the `.nx` game-data files.
- `ClientInstallDir` — folder with the client `.exe` + assets.
- `ClientExe`        — the client executable to launch (e.g. `MapleStory.exe`).

Paths are relative to the launcher's own folder (`.` = same folder). If the client
lives in a subfolder, use e.g. `ClientInstallDir = client`.

### ⚠ Critical constraint
A running program can't overwrite its own `.exe`. The launcher updates the **client**
files (including the client `.exe`) *while the client is not running*, then launches it.

- **Do NOT place `AugurMS.exe` inside the `ClientInstallDir` folder** (the folder the
  client manifest writes into). Keep the launcher in the game root and point
  `ClientInstallDir` at the client's own folder, or keep them the same folder but make
  sure the client manifest never contains an entry named `AugurMS.exe`.

---

## Manifests

Both manifests share the same shape; only the destination folder differs:

- `DataManifestUrl`   → synced into `ClientDataDir`   (the `.nx` files).
- `ClientManifestUrl` → synced into `ClientInstallDir` (client exe + assets).
  `name` here may include sub-folders (relative paths); the launcher recreates them.

```json
{
  "generatedAt": "2025-01-01T00:00:00Z",
  "baseUrl": "https://72.60.176.12/updates",
  "files": [
    { "name": "Mob.nx", "url": "https://72.60.176.12/updates/Mob.nx", "size": 480000000, "sha256": "abc…" }
  ]
}
```

- `url` may be absolute (used as-is) or relative (resolved against `baseUrl`).
- `sha256` is a lowercase hex digest. If omitted, the file is matched by size only.

---

## Notes

- A tiny `.augur_hashcache.json` is written next to the launcher to remember file
  hashes (keyed by size + modified-time) so big `.nx` files aren't re-hashed every
  launch. Safe to delete; it rebuilds itself.
- `logo.png` is loaded from next to the exe at runtime — swap it any time. If it's
  missing, the launcher shows an "AugurMS" wordmark instead.
- The window is borderless/branded: drag anywhere to move, `–` minimizes, `✕` closes.

## Font (optional, for the exact look)
The UI asks for **Fredoka** and falls back to Segoe UI. For the intended rounded look,
install Fredoka on the machine (or tell me and I'll embed `Fredoka-*.ttf` into the exe
so no install is needed).
