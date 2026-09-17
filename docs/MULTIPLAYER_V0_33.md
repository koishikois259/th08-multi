# th08-multi v0.33 release checkpoint

v0.33 is a synchronization update built directly on v0.32. It makes P1's
validated clear-progress snapshot authoritative for the multiplayer session
and prevents title/loading timing differences from affecting menu input or
the gameplay RNG.

## Fixes

- P1 clear progress, route availability, and unlock decisions are applied to
  both peers for the connected session. P2's raw `score.dat` is neither sent
  nor overwritten.
- Title-menu input stays locked until both game instances report ready. Inputs
  held during loading must be released before P1 can navigate the menu.
- Automatic title Demo playback is disabled in multiplayer mode.
- Every stage uses a common post-load RNG synchronization point before gameplay
  simulation resumes.
- Desync hashes use the monotonic network timeline, preventing an old run's
  hash from colliding with a later run that reuses gameplay frame numbers.

## Compatibility

Both peers must use the same v0.33 patch and Japanese TH08 1.00d data. The
gameplay build fingerprint and launcher handshake version are `0x00030003`, so
v0.33 intentionally refuses mixed-version connections.

## Validation

The VC7 multiplayer build and focused state/network tests pass. An isolated
two-process run used a progressed P1 save and a fresh P2 save: P2 logged that
its local progress was ignored, applied the same normalized progress
fingerprint as P1, entered Stage 1, and matched P1's gameplay state hashes.

The data-free patch also includes the user-provided `联机教程.txt`. It
does not contain the original executable, DAT archives, music, replay data, or
save files.
