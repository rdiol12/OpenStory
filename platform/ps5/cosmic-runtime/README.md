# Experimental PS5 Java runtime

This directory cross-builds OpenJDK 21's Zero interpreter and base libraries
for a native PS5 application. It produces a Java probe, not a complete Cosmic
server or an installable game-server package.

## Build

Use Ubuntu/WSL with Clang/LLVM 18, make, autoconf, curl, patch, tar, zip/unzip
and a host JDK 21. Keep the SDK and working files on the Linux filesystem.
Run from the OpenStory repository:

```sh
python3 platform/ps5/cosmic-runtime/build.py --sdk /path/to/ps5-sdk --static
python3 platform/ps5/cosmic-runtime/embed.py --sdk /path/to/ps5-sdk --ps5library ../PS5Library
```

The default workspace is `$HOME/build/cosmic-ps5`; use `--work` on both commands
to override it. `--jdk` selects the host JDK and `--jobs` controls parallelism.
`--stage ffi`, `--stage hotspot`, and `--stage base` select individual stages.
`--static` selects `jdk-static-build`; shared builds use `jdk-build`.
Sources are downloaded only when absent and checked against pinned hashes.

| Source | Pin |
|---|---|
| FreeBSD OpenJDK 21 | `battleblow/jdk21u`, `jdk-21.0.12+8-2` |
| libffi | `3.8.0` |

To rebuild the JVM after source changes:

```sh
make -C "$HOME/build/cosmic-ps5/jdk-static-build" hotspot JOBS=8
```

Clean the affected build directory when changing JVM feature flags. Build logs
remain in the workspace; failures return a nonzero exit status.

## Output and checks

The embedded application is staged under
`build/ps5/cosmic-runtime/dist/PPSA99784`. Its native `eboot.elf` is an application
executable, not an ELF-loader payload. `status.json` records build hashes and
remaining runtime requirements. Console logs use `/download0/cosmic/java-probe.log`.

`RuntimeProbe.java` exercises persistence, threads, loopback sockets and a
selector. Host checks cover libffi calling conventions, process waiting and
the static native-symbol registry. Host checks do not establish console
compatibility.

The static registry supports embedded Java libraries and rejects unknown
libraries or symbols. This build excludes GUI, audio, printing and selected
JVM diagnostic services. Running Cosmic still requires the remaining Java
modules, GraalJS, JDBC/database support, server data and console testing.
See [server setup](../COSMIC.md) for client connection requirements.
