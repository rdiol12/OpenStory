# GenMs Launcher

A Windows launcher that downloads client and NX-data updates, displays optional
server feeds and starts OpenStory. Supply your own server and update endpoints.

## Build

Install the .NET 8 SDK and run from the repository root:

```powershell
dotnet publish launcher/AugurLauncher.csproj -c Release
```

The self-contained executable is
`launcher/bin/Release/net8.0-windows/win-x64/publish/GenMs.exe`.
It includes its icon and logo. A .NET runtime installation is not required.

## Configure and play

Copy `config.example.ini` to `config.ini` beside `GenMs.exe`, then edit the local
copy. `.example` addresses are placeholders; no public server is preconfigured.

- `ClientDataDir`: directory containing v83 NX files.
- `ClientInstallDir`: directory containing the client and assets.
- `ClientExe`: client executable, such as `OpenStory.exe`.
- `DataManifestUrl` and `ClientManifestUrl`: your update manifests.
- `FeedUrl`, `NewsUrl` and `LauncherManifestUrl`: optional feed and self-update URLs.
- `GameHost` and `GamePort`: status-probe fallback. Client `Settings` take priority.

Paths are relative to the launcher directory. Set `ServerIP` and `ServerPort`
in the client's local `Settings`, or through the launcher's settings screen.
The update-server field rebases data, client, feed and launcher URLs; configure
the news endpoint separately. HTTPS servers must present a trusted certificate.

Start your compatible v83 server, open the launcher, apply available updates,
then select **Play** and log in with an account on that server. The launcher
can start an existing client when the update server is unavailable.

Keep the launcher outside the client manifest's file set so client updates do
not overwrite a running launcher. Self-updates use their separate manifest.
Local configuration, downloaded assets and account settings are not source files.

## Update manifests

Data and client manifests use the same structure. Data files install under
`ClientDataDir`; client files install under `ClientInstallDir`.

```json
{
  "generatedAt": "2026-01-01T00:00:00Z",
  "baseUrl": "https://updates.example/updates",
  "files": [
    { "name": "Mob.nx", "url": "Mob.nx", "size": 1234,
      "sha256": "<SHA-256-OF-FILE>" }
  ]
}
```

URLs may be absolute or relative to `baseUrl`. Provide SHA-256 hashes for file
verification. Downloads use temporary files and retain update backups according
to `BackupKeep`. The local hash cache can be deleted to force rechecking files.
