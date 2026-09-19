# macOS application bundle

Build inputs for `OpenStory.app`. Nothing here is used on Windows or Linux.

| File | Purpose |
| --- | --- |
| `Info.plist.in` | Template for `Contents/Info.plist`, configured by CMake (`@ONLY`). |
| `make_icns.cmake` | Derives `OpenStory.icns` from `resource/Icon.ico` with `sips` + `iconutil`. |

## Building

One target produces both layouts:

```sh
cmake -S . -B build-macos
cmake --build build-macos -j 6
```

- `build-macos/OpenStory.app` — double-clickable, ad-hoc signed. `MACOSX_BUNDLE`
  is set on the `OpenStory` target itself, so `CFBundleExecutable` *is* the game:
  one process, no launcher stub, and the process is called `OpenStory`
  everywhere.
- `wz/OpenStory` — the same binary, copied out of the bundle for terminal use,
  with `libbass.dylib` beside it. Unchanged workflow: `cd wz && ./OpenStory`.

The two copies differ only in their code signature (the one in the bundle is
sealed together with `Info.plist` and `CodeResources`).

Build only the plain executable with `-DOPENSTORY_MACOS_BUNDLE=OFF`.

Cache variables:

| Variable | Default | Meaning |
| --- | --- | --- |
| `OPENSTORY_DATA_DIR` | `<source>/wz` | Directory holding the `.nx` files and `Settings`. |
| `OPENSTORY_BUNDLE_ID` | `org.openstory.client` | `CFBundleIdentifier`. |
| `OPENSTORY_BUNDLE_VERSION` | `1.0.0` | `CFBundleShortVersionString` / `CFBundleVersion`. |

## How the assets are found

The ~3.8 GB of `.nx` files are **not** copied into the bundle. The client
resolves every runtime path against the current working directory — the assets,
`Settings`, `buddymemo.txt`, `screenshots/` and the crash log — and a
double-clicked `.app` starts in `/`, with no Info.plist key to change that.

So `chdir_to_data_directory()` in `src/MapleStory.cpp` picks a directory and
moves into it as the **first statement of `main()`**, before
`install_crash_logger()` and before anything can construct the `Configuration`
singleton (which reads `Settings` out of the working directory). The bundle just
records *where* the data lives, in `Contents/Resources/DataDirectory`.

Search order, first directory containing `Base.nx` wins:

1. `$OPENSTORY_DATA_DIR`
2. the current working directory — so every existing terminal workflow behaves
   exactly as it did before: if the cwd is already right, nothing below is even
   considered
3. the directory holding the executable — `wz/OpenStory` started from anywhere
4. the path in `Contents/Resources/DataDirectory` *(bundle only)*
5. `Contents/Resources/data`, if you put a directory or symlink there *(bundle only)*
6. `~/Library/Application Support/OpenStory`
7. the folder containing `OpenStory.app` *(bundle only)*

If none match, the working directory is left alone and the normal
"Missing a game file: Base.nx" error is raised — with every path that was tried
appended to it. Launched from Finder there is no console and no usable `stdin`,
so that message is shown in an alert instead of an interactive retry prompt that
would read EOF and exit silently.

To move the app to another machine, copy `OpenStory.app` plus the data
directory, then either export `OPENSTORY_DATA_DIR`, edit
`Contents/Resources/DataDirectory`, or drop the `.app` next to the data folder
(rule 7).

## Signing

The build ad-hoc signs `libbass.dylib` and then the bundle
(`codesign --force --sign -`). No real identity, no notarization. Verify with:

```sh
codesign --verify --deep --strict --verbose=2 build-macos/OpenStory.app
```

Editing anything inside the bundle (including `Contents/Resources/DataDirectory`)
breaks that signature. Re-sign with:

```sh
codesign --force --sign - build-macos/OpenStory.app
```

A locally built bundle carries no quarantine attribute. If you ever move the
`.app` through a browser, AirDrop or a zip from another machine, macOS will
refuse to open it; strip the flag with:

```sh
xattr -dr com.apple.quarantine /path/to/OpenStory.app
```
