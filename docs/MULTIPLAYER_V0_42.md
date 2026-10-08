# th08-multi v0.42 local release checkpoint

v0.42 builds on v0.41 with the shared-keyboard and local UI commits
`b4befee6`, `205ad6ff`, `3bf47758` and `d40b6a46`. This is a local patch only;
no GitHub push or Release upload is authorized.

## Changes

- Local play offers green-dot Single device / Double device radio selectors.
  Double device keeps its physical-device identification behavior and only one
  identification instruction.
- Single device supports shared-keyboard co-op without physical-device discovery.
  P1 defaults to arrows/Shift/Z/X; P2 defaults to I/K/J/L/D/F/G. Set keys edits
  both players' seven gameplay actions, with within-player and cross-player
  conflicts rejected. Shared menu/system keys remain reserved.
- Key capture handles physical keys independently of IMEs, filters repeats and
  ignores focus notifications. Generic and side Shift keys share one conflict
  domain. Remapped P1 defaults no longer remain hidden action aliases.
- Bindings are stored in the isolated local_keyboard section. Old P2-only
  configurations retain their P2 mapping and use P1 defaults. Double device,
  online and ordinary single-player input do not use these bindings.
- Local team selection omits device-number suffixes. Both team/status rows and
  two instruction rows stay inside the left playfield, with text state restored
  after queueing. Normal and Extra selection share this layout.
- The user-provided root-level Chinese tutorial is imported without editing.
  English repository README and Chinese patch README preserve credits and
  document the new mode. v0.41 mode isolation and keyboard recovery are retained.

## Compatibility and payload

The launcher version gate and game build fingerprint are now `0x00040002`.
Both peers must update the launcher and game; v0.41/older peers are rejected.
Gameplay/resource rules and game packet layouts are unchanged.

The seven-file data-free ZIP contains both executables, example INI, Chinese
README, license, the user's Chinese tutorial and SHA256SUMS.txt. Original game
files, saves, active configuration and physical device identifiers are not
packaged. Installation replaces only patch-owned files and preserves th08.exe,
DATs, score.dat, th08.cfg and th08_multi.ini.

## Validation

The VC7 game/launcher builds and six native automated suites cover keyboard
configuration, hidden launcher UI, launch mode, keyboard recovery, multiplayer
state and network behavior. Shared-keyboard tests include default/custom P1/P2
actions, fourteen-key round trips, old P2-only files, conflict/Shift handling,
save/cancel/defaults and configuration isolation. They do not emulate physical
keyboard rollover or every hardware/driver combination.

The user confirmed the radio/capture launcher fixes and left-region team
selection in the preceding checkpoints. Physical custom P1/P2 gameplay, Local
to online transitions, hotplug and long two-PC runs remain manual checks in
[the test checklist](MULTIPLAYER_TESTING.md). No new long-run crash fix is claimed.

Normal TitleScreen::OnDraw @ 0x0047087F passed its focused 493-byte exact check
before versioning; this release changes only multiplayer-conditional title
strings. Original engine layouts and accepted reconstruction ledgers are
unchanged; this is not an aggregate matching claim.

Documentation, semantic-protocol and whitespace checks pass. Full CI reaches
the shell syntax step but cannot start /bin/bash under the available WSL
environment; its preceding Python/literal/protocol checks pass.

The versioned game/launcher build and all six suites pass, including a direct
launcher packet check that accepts v0.42 and rejects v0.41. The ZIP has exactly
seven expected entries; CRC validation, source/ZIP/installed byte equality and
both checksum manifests pass. The user's tutorial matches the ZIP and installed
copy byte-for-byte. Pre/post hashes of th08.exe, th08.dat, thbgm.dat, score.dat,
th08.cfg and th08_multi.ini are unchanged. No game was launched during packaging.

ZIP SHA-256:
`2fa998700ca96bab24515ffb1990e8d592902abb0d30c281ca070a6304d4e333`

## Pending GitHub Release text

No upload is requested yet. When explicitly authorized, retain the user's
previously requested v0.41 wording along with the v0.42 additions and recheck
the archive hash before uploading:

```text
Local play 不再覆盖远程联机配置，本地双控需从启动器进入。
切屏后再切回游戏重新获取键盘，成功后立即重读，不再直接返回空输入。
新增 Single device 共用键盘本地双控，Set keys 可分别设置 P1/P2 键位，且不影响其他模式。
模式选择改为绿色圆点单选；修复改键输入法/焦点处理，调整选人文字以避免截断。
更新联机教程。

SHA-256:
2fa998700ca96bab24515ffb1990e8d592902abb0d30c281ca070a6304d4e333
```
