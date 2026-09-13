# th08-multi v0.32 validation ledger

This ledger separates implemented code, automated rule checks, and observed
two-process gameplay. An item is accepted only after the relevant observable
result has been reproduced; compiling successfully is not gameplay acceptance.

## Result meanings

- **PASS**: reproduced with the listed check.
- **CODE**: implementation and focused automated checks exist, but the complete
  in-game interaction still needs an observed two-process run.
- **PENDING**: no sufficient acceptance evidence yet.

## Gameplay matrix

| Requirement | Status | Current evidence | Remaining check |
| --- | --- | --- | --- |
| Independent team selection; duplicates allowed | PASS | Local 0/0 and 1/2 sessions both entered gameplay | Repeat after release packaging |
| Independent movement | PASS | P1-only left and P2-only right were observed identically in both processes | Longer cross-machine run |
| Independent shooting and familiar/team state | CODE | Separate `Player` shot/option state and per-slot team files are active; different-team stress run stayed synchronized | Observe damage from both players and all four team combinations |
| Independent Focus and human-youkai gauge | CODE | P1-only Focus was observed without changing P2 Focus; per-slot gauge accessors are used | Observe both gauges changing independently in HUD |
| Independent bombs | PASS | P2 bomb changed the synchronized resource view from 3/3 to 3/2 while P1 remained 3 | Exercise simultaneous and deathbomb cases |
| Independent collision and graze | CODE | Bullets, lasers, and enemy contact call each physical player's collision functions; bullets carry separate P1/P2 graze latches; state tests cover combined stage reward and per-stage reset | Observe one-player-only hit/graze and verify the stage reward uses the P1+P2 sum |
| Shared score | CODE | One canonical `ZunGlobals::score` is included in the deterministic state hash, displayed by both peers, and preserved across stage setup | Raise score, cross a stage boundary, and compare both HUDs frame-for-frame |
| Shared point count and maximum point value | CODE | Point collection updates the canonical count/value; the HUD shows `POINT current/next MAX value`; reaching the threshold awards one life to each non-capped player | Collect point items through a threshold and compare both players' lives and HUDs |
| Shared Time and Last Spell threshold | CODE | Time uses one canonical current/total value; the deathbomb window reads the shared threshold | Reach threshold, hit only one player, and observe partner control plus local bomb spend |
| Nearest-player item attraction and pickup | CODE | Eligible physical collectors are resolved by squared distance with deterministic P1 tie-break; Time-rise targeting now uses the same rule | Place each item type near P1, near P2, and at an exact tie |
| Power/life/bomb item goes only to collector | CODE | Collection functions mutate the resolved collector's slot only | Observe all three item types on both players |
| Nearest living player enemy/Boss targeting | CODE | bullet/laser aim, ECL operands, movement bias, and minimum-distance gates use the nearest physical player | Observe aimed patterns after swapping which player is closer |
| Spirit mode and TH06-style revival | CODE | State tests cover Focus/no-shoot/overlap, 90-frame completion, cancel, reserve-life rejection, and revival at 128 Power; stage setup reapplies persistent spirit presence | Exhaust one player, cross a stage boundary without revival, then revive them at 128 Power; repeat with P1/P2 reversed |
| Game over only when both are unable | CODE | spirit transition tests and `BothPlayersUnableToContinue` gate exist; stage transitions no longer initialize a spirit as physical | Exhaust one player while the other remains active, then exhaust both in one two-process run |
| Final lives/bombs bonus sums both players | CODE | Stage-clear calculation and detail rows read P1+P2 independent resources | Complete Stage 6 and compare the displayed bonus with both players' remaining resources |
| Replay disabled in multiplayer | CODE | Multiplayer launch forces non-replay gameplay and hides multiplayer replay flow | Complete/end a run and confirm no replay-save prompt |
| Packet loss/jitter tolerance | PASS | Two local processes stayed alive and responsive with every 10th UDP packet dropped plus 4-18 ms jitter | Cross-machine WAN envelope |
| Desync detection | PASS | Forced Guest P2-position mutation was detected by both peers; both exited cleanly | Verify packaged diagnostic log paths |

## HUD acceptance

The multiplayer right-side HUD has these intended rows:

- Player, Spell, Power, and Graze: P1 and P2 values side by side; blue is P1,
  pink is P2, and `*` marks the local player.
- Point: shared point-item count, next shared life threshold, and shared maximum
  point-item value, displayed as `POINT current/next MAX value`.
- Time: shared current/required Time plus shared total Time.
- Score and high score: the existing canonical shared displays.
- Human-youkai gauge: numeric P1 and P2 values below the standard resource
  rows; spirit state and revival progress appear when active.

The VC7 multiplayer and normal builds pass. Visual two-window HUD acceptance is
still **PENDING** because the current automation surface cannot capture or send
input to native DirectX game windows. Do not mark the HUD complete until both
Host and Guest views have been inspected.

Use [the manual network testing guide](MULTIPLAYER_TESTING.md) and the bundled
`th08-multi-launcher.exe` only for trusted-peer two-PC tests.

## Launcher acceptance

- The multi build produces both `th08-multi.exe` and
  `th08-multi-launcher.exe`.
- Both launchers establish a separate pre-game UDP handshake and display their
  connection state before either game process starts.
- Only the connected Host can issue the synchronized start command. The Guest
  acknowledges it, both launcher sockets are released, and both games start;
  the Guest cannot start independently.
- The packaging script verifies the original Japanese 1.00d executable before
  installing and refuses to include original DAT, executable, score, or replay
  data in the patch archive.
- The launcher process remained alive during its startup smoke test.
- Fresh copies placed in the local Host/Guest fixtures connected successfully;
  both window titles reported `connected (select teams in game / delay 3)`.
- The v0.32 six-file patch archive includes the GitHub source and release URLs
  in its README and has SHA-256
  `80fa8fd5331834a5ba68e66230e77f6598c744b27af99d362638104fa14b29db`.
- Installation into the verified `th08-origin` directory preserved the original
  `th08.exe` hash, all installed payload hashes matched, and the installed
  launcher remained alive during its startup smoke test.
- Button-level visual inspection and cross-PC trusted LAN/VPN testing remain
  manual checks because native Win32 windows are unavailable to the current
  automation surface. Public-IPv4 testing is intentionally unsupported.

## Reproducible automated checks

```text
python scripts/build.py --build-type multi --fresh -j 1 multiplayer-tests
build\multiplayer-state-tests.exe
build\multiplayer-net-tests.exe
python scripts/build.py --build-type normal --fresh -j 1
python scripts/build.py --build-type multi --fresh -j 1
python scripts/package-multiplayer-patch.py --install-dir ..\th08-origin
```

Build configurations must be generated serially because they share
`build.ninja` and the `build` object directory.
