# Host-authoritative gameplay settings (v0.42 local refresh)

The implementation checkpoint `4b1a4a23` is folded into the October 10 local
v0.42 refresh, repackaged and installed into `th08-origin`. The existing
Chinese tutorial is retained byte-for-byte. GitHub publication was approved
on October 10 with the user's edited Release text and ZIP-only attachment.
See [the v0.42 checkpoint](MULTIPLAYER_V0_42.md) for packaging evidence.

## Slow-mode synchronization

Remote Host/Guest sessions use P1's validated `slowMode` value. The welcome
snapshot carries the boolean at wire byte 41, next to the existing initial-life
value. The guest's persistent `Supervisor::cfg` and `th08.cfg` are not replaced.
Only the temporary `GameManager::cfg` used by this run is overridden. Single
player and Local play keep their own original configuration.

Normal runs apply the host value when configuration is created and again at
the post-load ready barrier on every stage. The original forced-off rules for
Extra, Practice, and Spell Practice remain in effect on both peers.

Every input packet carries the current settings/save snapshot revision. Title
and stage readiness are advertised only after the session has received the
peer's matching revision. An unacknowledged host snapshot is resent every
250 ms independently of the 50 ms input retry timer. Old welcome packets may
not roll initial lives or slow mode backward; conflicting values at an equal
revision are rejected. Snapshot equality compares named fields, not native
structure padding.

The wire protocol is now `0x00010003`; the game fingerprint and launcher gate
are `0x00040003`. Both peers need this refreshed game and launcher; they must
not be mixed with the earlier local v0.42 binaries (`0x00040002`) or v0.41 and
older builds. The displayed release version remains v0.42 at the user's request.

## Configuration audit

| Setting | Observed code behavior | Synchronization policy |
| --- | --- | --- |
| `slowMode` | Skips downstream simulation updates at bullet-count thresholds 128/224/320; also affects internal replay bookkeeping | Newly host-authoritative for remote play |
| `lifeCount` | Sets starting resources and continue resources | Already host-authoritative; now protected against stale snapshots too |
| `bombCount` | Validated and serialized, but player initialization uses the selected SHT's initial bomb count | No additional config synchronization identified |
| `defaultDifficulty` | Initial offline menu preference; connected difficulty selection already resets to Normal/Extra before synchronized navigation | Existing multiplayer protection retained |
| `shotSlow`, controller mappings, axes, DirectInput selection | Convert the owner's hardware state to gameplay button bits before those bits enter lockstep | Keep individual input preferences |
| `frameskipConfig` | Controls draw/present frequency; not the bullet-count-based slow-mode update skip | Keep individual display preference; affects performance |
| `effectQuality`, minimum graphics, fog, textures, depth, blending, vertex buffers, text background, HUD redraw | Rendering choices; the reviewed effect-quality branch filters drawing, not EffectManager update/RNG | Keep individual display preferences; no direct gameplay divergence established by this audit |
| Windowed/fullscreen, color mode, VSync | Window/device/presentation choices | Keep local; may affect performance or old D3D8 compatibility |
| Music mode, volume, sound, music preload | Audio and loading choices | Keep local; stage ready barrier handles loading differences |
| `force60Fps` | Explicitly cleared by `Supervisor::LoadConfig` | Not an active setting-based divergence source in the current build |

This is a code-path audit, not proof that every rendering driver, injected
module, game-data patch, or custom.exe combination is compatible. Build
fingerprints do not attest every original DAT/ANM/ECL/SHT file. Both peers still
need matching supported original data and should avoid speed-altering patches.
The existing lockstep wait and error=7 stop are intentionally retained.

## Validation boundary

The VC7 multiplayer game and launcher compile/link. All seven native test
executables pass; the real UDP session-settings suite also passes 20
consecutive runs. Packet sizes remain bounded: 126 bytes for a full redundant
input packet and 317 bytes for a welcome snapshot, both below the 512-byte
receive buffer.

The non-multiplayer `GameManager::GameplaySetupThread @ 0x0043ABD7` focused
comparison is exact at 3,423/3,423 bytes. A full normal build is blocked by a
pre-existing unguarded `ValidateTrailState` call in
`EnemyManagerUpdate.cpp:350`, whose declaration is multiplayer-only. This
file is unchanged by this checkpoint. Full CI stops at shell syntax checks
because the installed WSL launcher cannot find `/bin/bash`; Python/literal
tests, semantic-protocol tests/checks, target-backed tracking validation, and
the documentation check pass independently. No whole-project exact claim or
full normal-build success is made.

Native UDP session tests cover both host slow-mode values, changing only slow
mode, guest write rejection, invalid values, bilateral revision confirmation,
dropped-update retransmission, reordered welcomes and input acknowledgements,
equal-revision conflicts, and identical snapshot retransmissions.

Full two-PC gameplay at the three bullet thresholds and across later-stage
transitions remains a manual validation requirement. This change does not
claim to repair heap corruption, resource-lifetime crashes, or network jitter.
