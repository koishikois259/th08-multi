# Shared-keyboard local co-op checkpoint

This local development checkpoint builds on v0.41. It is not a new release:
existing release ZIPs, tags and GitHub have not been updated. The development
binaries are synchronized to `th08-origin`. `联机教程.txt` is intentionally unchanged pending the user's
requested wording.

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
- Set P2 keys opens a separate edit page. Capture is handled before dialog
  navigation; repeats, reserved keys and duplicate P2 assignments are rejected.
  Only button clicks arm capture, not focus notifications. The focused action
  button requests all keys and forwards key-down messages to capture. Foreground
  raw keyboard input also captures physical keys independently of IMEs; legacy
  `VK_PROCESSKEY` messages are resolved from their scan codes. Raw registration
  is removed when the key editor closes and does not change online input.
  Escape cancels capture. Cancel discards the draft; Defaults changes the draft;
  Save writes the validated seven-key section.
- Configurations use `[local].input_mode` and `[local_keyboard]`. Invalid
  manually edited bindings fall back to the entire default set. The online
  mode/address and Double device identifiers remain unchanged by key editing.
- Local keyboard input is foreground-only and clears on focus loss/rebind.
  Online/single-player input routes and the original engine layouts are not
  modified. Gameplay rules and the online protocol/fingerprint are unchanged.

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
default/custom actions, menu ownership and invalid-config fallback.

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
