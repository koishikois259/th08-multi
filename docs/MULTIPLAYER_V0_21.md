# th08-multi v0.21 release checkpoint

v0.21 retains the v0.2 two-player gameplay rules and changes the Windows
startup flow so that the network connection is established before either game
process starts.

## Launcher changes

- Host and Guest first select their network settings and choose **Connect**.
- Both launchers display waiting, connecting, connected, disconnect, and
  synchronized-start status without starting the game executable.
- The Host's **Start both games** button remains disabled until a Guest is
  connected. The Guest cannot start independently.
- When the Host starts the session, the Guest acknowledges the start command.
  Both launchers then release their UDP sockets and start their local
  `th08-multi.exe`; the game processes establish the normal lockstep session.
- Launcher heartbeats detect a lost peer and return the Host to waiting or the
  Guest to reconnecting state.

## Compatibility

Both peers must use the same v0.21 patch. The game and launcher handshakes use
v0.21 fingerprints, so v0.2 and v0.21 peers cannot accidentally enter a mixed
session. The supported base game remains Japanese TH08 1.00d. Replay,
matchmaking, relay, automatic port forwarding, and NAT traversal are not
included.
