# Connecting to Cosmic

OpenStory targets MapleStory v83. Run a compatible Cosmic server with its
required Java runtime, database, scripts and WZ XML data. Client NX files do
not replace the server's data files.

Set the server's advertised login, channel and cash-shop addresses to values
reachable from the client. The usual login port is TCP `8484`; channel ports
start at TCP `7575`. Configure those ports on your server and network as needed.

On PS5, enter the server's IPv4 address and login port on the startup screen.
On desktop, configure `ServerIP` and `ServerPort` in the local `Settings` file
beside the executable. Create accounts through the server's own account flow.

For non-ASCII chat, configure UTF-8 on both client and server. Cosmic's allowed
charset list must include UTF-8 when selecting `CHARSET: UTF-8`.

## Experimental console-hosted runtime

The [Java runtime port](cosmic-runtime/README.md) builds an embedded probe.
It is not a complete Cosmic server package. Running Cosmic on PS5 also needs
its Java modules, GraalJS host calls, JDBC database, persistent storage, server
data, and a supported process lifetime while the client runs.
