# th08-multi v0.4 local release checkpoint

v0.4 builds on the local v0.34 patch. This checkpoint is packaged and installed
locally; it has not been uploaded to GitHub.

## Changes

- Single-process, single-window local co-op with P1 and P2 on different input
  devices: two keyboards, keyboard plus gamepad, or two gamepads.
- Physical keyboard discovery through Raw Input. Keyboards are listed only
  after a Confirm press, avoiding auxiliary and virtual keyboard interfaces.
- Gamepads are listed only after a button press. DirectInput interfaces that
  report the same press together are treated as one device. The physical
  gamepad combinations and delayed duplicate reports still require testing.
- P1 confirms the team first, then P2; existing cooperative gameplay and
  resource rules are reused. Online Host/Guest play remains available.

## Compatibility and distribution

The launcher protocol version and game build fingerprint are both
`0x00040000`, so mixed v0.34/v0.4 online builds are rejected. Online peers
need the same v0.4 patch and Japanese TH08 1.00d game data. The data-free
patch contains no original executable, DAT files, save data, or replay files.
Installation into `th08-origin` updates only patch-owned files, preserving the
original game files, `score.dat`, `th08.cfg`, and `th08_multi.ini`.

## Validation

The VC7 multiplayer game and launcher build passed, as did the automated
multiplayer state and network protocol tests. The patch packaging step
verified the Japanese 1.00d original before installing patch-owned files.
The repository-wide `scripts/ci.py` run reached shell-syntax validation but
could not launch Bash in this Windows environment (`E_ACCESSDENIED`); it did
not complete. Physical two-keyboard, keyboard/gamepad, two-gamepad, hot-plug,
and long-stage tests remain manual release checks.
