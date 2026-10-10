# th08-multi v0.42 local release checkpoint

v0.42 builds on v0.41 with the shared-keyboard and local UI commits
`b4befee6`, `205ad6ff`, `3bf47758` and `d40b6a46`. This is a local patch only;
no GitHub push or Release upload is authorized.
The October 10 refresh folds in host-authoritative gameplay settings from
`4b1a4a23`, retaining the v0.42 release name and the existing tutorial.

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
- The user-provided root-level Chinese tutorial was imported in the original
  v0.42 package. This refresh preserves it without editing or reimporting.
  English repository README and Chinese patch README preserve credits and
  document the new mode. v0.41 mode isolation and keyboard recovery are retained.
- Remote play uses P1 slow mode only in the temporary run configuration;
  guest saved config, Local play and ordinary single-player settings are not
  replaced. Original forced-off rules for Extra/Practice/Spell Practice remain.
- Input packets acknowledge the settings/save revision. Menu/stage readiness
  waits for bilateral confirmation, unacknowledged host snapshots are retried,
  and outdated snapshots cannot roll initial lives or slow mode backward.
  See [the settings/config audit](MULTIPLAYER_GAMEPLAY_SETTINGS.md) for other
  custom.exe settings and the boundaries of this change.

## Compatibility and payload

The launcher version gate and game build fingerprint are now `0x00040003`,
and the wire protocol is `0x00010003`. Both peers must update the launcher
and game; the original local v0.42 package (`0x00040002`) and v0.41/older
peers are rejected even though the visible version name remains v0.42.
Gameplay/resource rules are unchanged, but input headers now carry a settings
revision and welcome snapshots include host slow mode. Full redundant input
packets are 126 bytes and welcome snapshots are 317 bytes, below the 512-byte
receive buffer.

The seven-file data-free ZIP contains both executables, example INI, Chinese
README, license, the user's Chinese tutorial and SHA256SUMS.txt. Original game
files, saves, active configuration and physical device identifiers are not
packaged. Installation replaces only patch-owned files and preserves th08.exe,
DATs, score.dat, th08.cfg and th08_multi.ini.
Identical installed files are not rewritten, preserving the installed tutorial
and its modification time. The prior ZIP, checksum file and seven installed
patch files are backed up under `build/v0.42-before-settings-refresh-*` before
replacement. The previous local tag target is
`fbf778b08e5c7382c9f78f165346f6544fec34d2`; that commit remains in history.

## Validation

The VC7 game/launcher builds and seven native automated suites cover keyboard
configuration, hidden launcher UI, launch mode, keyboard recovery, multiplayer
state and network behavior. Shared-keyboard tests include default/custom P1/P2
actions, fourteen-key round trips, old P2-only files, conflict/Shift handling,
save/cancel/defaults and configuration isolation. They do not emulate physical
keyboard rollover or every hardware/driver combination.
The real UDP session suite covers both host slow-mode values, revision
confirmation, invalid settings, guest write rejection, dropped-update retries,
reordered welcomes/acknowledgements, and conflicting equal-revision snapshots.
It passed 20 consecutive runs at the implementation checkpoint.

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
The settings checkpoint's focused non-multiplayer
GameManager::GameplaySetupThread @ 0x0043ABD7 comparison is exact at
3,423/3,423 bytes. A full normal build remains blocked by the pre-existing
unguarded ValidateTrailState call at EnemyManagerUpdate.cpp:350; that source
was not changed. No aggregate exact claim or full normal-build success is made.

The refreshed game/launcher build and all seven suites pass, including a
launcher packet check that accepts the new gate and rejects older gates.
The ZIP has exactly seven expected entries; CRC validation,
source/ZIP/installed byte equality and
both checksum manifests pass. The user's tutorial matches the ZIP and installed
copy byte-for-byte. Pre/post hashes of th08.exe, th08.dat, thbgm.dat, score.dat,
th08.cfg and th08_multi.ini are unchanged. No game was launched during packaging.

ZIP SHA-256:
`0527391a7e704f3ac848b93f6e8d98d1a67c0f13956827ca6997ab8a4806ad98`

Prior package SHA-256 (retained in the local backup):
`2fa998700ca96bab24515ffb1990e8d592902abb0d30c281ca070a6304d4e333`

Unchanged tutorial SHA-256 (workspace, docs, ZIP and installed copy):
`c3ca2f2d56e922c2744aac5edad88e5a0d369bb152f52d715f08ca4b84150992`

## Pending GitHub Release text

No upload is requested yet. When explicitly authorized, retain the user's
previously requested v0.41 wording along with the v0.42 additions and recheck
the archive hash before uploading:

```text
Local play 不再覆盖远程联机配置，本地双控需从启动器进入。
切屏后再切回游戏重新获取键盘，成功后立即重读，不再直接返回空输入。
新增 Single device 共用键盘本地双控，Set keys 可分别设置 P1/P2 键位，且不影响其他模式。
模式选择改为绿色圆点单选；修复改键输入法/焦点处理，调整选人文字以避免截断。
缓慢模式统一以 P1 主机为准，仅影响本局运行配置，不改写客机设置。
增加配置版本双方确认、丢包补发和旧快照拒绝；联机教程原样保留。
双方须同时更新，不可与此前的 v0.42 包混用。

SHA-256:
0527391a7e704f3ac848b93f6e8d98d1a67c0f13956827ca6997ab8a4806ad98
```
