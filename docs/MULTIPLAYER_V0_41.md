# th08-multi v0.41 local release checkpoint

v0.41 packages the two post-v0.4 input fixes, based on v0.4 plus commits
`e1177c9` (mode isolation) and `ca98ccb` (keyboard recovery). It is a local
release only; no GitHub push or Release upload is requested.

## Changes

- Explicit per-process Local/Host/Guest mode overrides stale stored local mode.
  Local device selection no longer replaces saved online mode.
- Foreground keyboard activation/read failure receives a bounded acquisition
  attempt and immediate re-read on success. Failed/unavailable DirectInput
  uses foreground-only Win32 fallback with the original action bindings.
- Failed-read buffers are cleared and focus is checked before publishing input.
  Background keyboard input is suppressed; local physical-device separation
  and the existing gamepad merge behavior remain unchanged.
- Startup logs record resolved launch mode and keyboard input-source changes.
- English repository README and Chinese patch README/tutorial describe both
  fixes. Existing contributor acknowledgements and local-device instructions
  are preserved.

## Compatibility and payload

The launcher protocol version and game build fingerprint are `0x00040001`.
Both peers must update the launcher and game together; mixed v0.4/v0.41 builds
are rejected. Gameplay/resource rules and packet layouts are unchanged.

The seven-file data-free ZIP contains both executables, the example INI,
Chinese README, license, Chinese tutorial, and internal `SHA256SUMS.txt`.
Installation into `th08-origin` changes only these patch-owned files; the
original executable/DATs, save, game configuration, and active multiplayer
configuration must retain their pre-install hashes.

## Validation

The four native automated suites cover launch-mode isolation, keyboard recovery,
gameplay state, and network protocol behavior. Keyboard tests use a fake
backend; they do not prove compatibility with every physical device or graphics
driver. Real-device focus switching, Local-to-online transitions, and long
two-PC sessions remain manual checks in [the test checklist](MULTIPLAYER_TESTING.md).
Full repository CI remains blocked at Bash startup with `E_ACCESSDENIED`.

The v0.41 VC7 game/launcher build and all four suites passed before packaging.
The ZIP's seven entries match the installed `th08-origin` files, and both the
internal and external checksums passed independent verification. Pre/post
hashes of `th08.exe`, `th08.dat`, `thbgm.dat`, `score.dat`, `th08.cfg`, and
`th08_multi.ini` are unchanged. No game was launched as part of packaging.

ZIP SHA-256:
`77cb0cabb54146268cad32ca43d05f9670f62c21f2422494f250ec077b14cc18`

The local archive was regenerated after shortening tutorial section three to
only the Local/Host/Guest startup instruction. Executables are unchanged.
