# Shared-keyboard local co-op checkpoint

This feature checkpoint builds on v0.41 and is included in the local v0.42
patch. See [the v0.42 release checkpoint](MULTIPLAYER_V0_42.md) for packaging
and installation validation. The user's updated `联机教程.txt` is included
verbatim. No GitHub upload is requested.

## Implementation

- The existing Local play setup page switches between Single device and
  Double device using mutually exclusive radio options with green selected
  circles. Old configurations continue to default to Double device. The
  Double device page keeps only one device-identification instruction.
- The in-game co-op character-selection overlay starts at x=48, with horizontal
  glyph scale 0.75 and 13-pixel base spacing. P1/P2 team/status rows and two
  separate instruction rows stay within the left playfield. Both local modes
  omit device-number suffixes; online mode retains its `(YOU)` marker. Text
  rendering state is restored after queueing, and character-selection input
  and gameplay rules are unchanged.
- Single device hides device selectors and stops discovery/polling. It does
  not require saved physical keyboard/gamepad IDs to start. Windows supplies
  one shared system-keyboard snapshot; shared gamepads are not supported.
- P1 uses arrows/Shift/Z/X, with Esc/Enter/Ctrl for menus/skip. P2 uses
  I/K/J/L/D/F/G. Other retail P1 keyboard aliases are not merged into this path.
- Set keys opens a separate edit page with P1/P2 columns. Capture is handled before dialog
  navigation; repeats, reserved menu/system keys and duplicate assignments
  within or across players are rejected. Both players' gameplay keys can be
  changed; released P1 default keys can be assigned to P2. Shift aliases share
  one conflict domain and apply only to the player/action currently bound to
  Shift, rather than remaining a hidden P1 focus shortcut.
  Only button clicks arm capture, not focus notifications. The focused action
  button requests all keys and forwards key-down messages to capture. Foreground
  raw keyboard input also captures physical keys independently of IMEs; legacy
  `VK_PROCESSKEY` messages are resolved from their scan codes. Raw registration
  is removed when the key editor closes and does not change online input.
  Escape cancels capture. Cancel discards the draft; Defaults changes the draft;
  Save writes the validated fourteen-key section. Existing P2-only files retain
  their P2 mapping and use unchanged P1 defaults until explicitly edited.
- Configurations use `[local].input_mode` and `[local_keyboard]`. Invalid
  manually edited bindings fall back to the entire default set. The online
  mode/address and Double device identifiers remain unchanged by key editing.
  Both sets of custom bindings are read only inside explicit local Single
  device initialization. Double device never loads this section; online and
  ordinary single-player launches never initialize local input.
- Local keyboard input is foreground-only and clears on focus loss/rebind.
  Online/single-player input routes and the original engine layouts are not
  modified. Gameplay rules and packet layouts are unchanged. The v0.42 release
  increments both launcher and game version gates to `0x00040002`.

## Validation

The exact Japanese 1.00d input executable was verified by SHA-256 and size.
The VC7 multiplayer game and launcher compile/link. All six test programs pass:

```text
python scripts/build.py --build-type multi -j 1 build/th08-multi.exe build/th08-multi-launcher.exe multiplayer-tests
build/multiplayer-local-keyboard-tests.exe
build/multiplayer-local-launcher-tests.exe
build/multiplayer-keyboard-tests.exe
build/multiplayer-launch-mode-tests.exe
build/multiplayer-state-tests.exe
build/multiplayer-net-tests.exe
```

The new launcher integration tests create only their own hidden windows and a
temporary configuration, without showing windows, discovering devices, launching
games or altering the user's game files. They check selector visibility,
device-free startup readiness, capture/repeat/conflict/cancel/save behavior,
and configuration isolation, exclusive radio selection, focus notifications,
posted key messages through the child WndProc, IME scan-code conversion and
raw physical-key capture/repeat/release. Pure mapping tests cover simultaneous P1/P2 input,
default/custom actions for both players, menu ownership and invalid-config fallback.
The two-column follow-up also covers P1 edits/save/cancel/defaults, fourteen-key
round trips, legacy P2-only files, within-P1/cross-player collisions, generic
versus side-Shift conflicts, remapped old P1 keys ceasing to act as aliases, and
P2 reusing released P1 defaults. Remote settings, Double device IDs and the
selected local input mode remain unchanged by a key-only save.

`scripts/check-docs.py`, `scripts/check-semantic-protocols.py`, and
`git diff --check` pass. Full `scripts/ci.py` remains blocked at Bash startup
because WSL cannot start `/bin/bash` in this environment; its preceding
Python/literal/protocol checks pass.
This is not an aggregate reconstruction matching claim.

For the character-selection overlay follow-up, the normal-build
`TitleScreen::OnDraw @ 0x0047087F` still passes the focused exact comparison
(`title-screen-roi-exact-0047087f`, 493/493 bytes). All four teams, both player
labels, both READY/SELECT states and the online `(YOU)` variant were checked
against the left-region text bound: the maximum right edge is 401.25, below
416, allowing a conservative 16-pixel unscaled glyph width. Each queued row
also stays below the ASCII renderer's 64-byte text buffer. Visible placement
and readability still require in-game confirmation.

Physical shared-keyboard rollover, visible page layout, sequential team
selection, gameplay/focus switching, Double device regression and online
regression still require manual testing. No long-run crash fix is claimed by
this input feature. The desktop automation helper exited during initialization,
so the reported visible key-remapping failure could not be reproduced through
desktop automation; physical-key capture and focus/message routing are covered
by the bounded launcher regression tests, with real-device confirmation pending.
