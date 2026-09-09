# th08-multi v0.1 design contract

This document freezes the agreed first-version behavior before gameplay code is
changed. It is the acceptance contract for the `multi/v0.1` branch.

## Scope

`th08-multi` is a source-level two-player cooperative fork of the Japanese
Touhou Eiyashou 1.00d executable. The implementation is based on the complete
TH08 reconstruction maintained at <https://github.com/N0zoM1z0/th08> and keeps
that repository as the `upstream` Git remote.

The network model follows the useful part of RUEEE's TH06 multiplayer design:
a host/guest session advances the game with deterministic, fixed-delay
lockstep input. TH08-specific game state remains authoritative in the game
simulation; the transport must not duplicate gameplay rules.

Replay recording and playback are out of scope for v0.1 multiplayer games.
Single-player behavior must remain available. Multiplayer v0.1 supports the
main story and Extra run paths; Practice and Spell Practice remain
single-player-only while multiplayer is configured.

## Player selection and input

- P1 and P2 independently select one of the four human-youkai teams on the
  in-game character-select screen after the connection is established.
- Both players may select the same team.
- Each player has an independent input frame.
- Human-youkai switching, options/familiars, shots, bombs, collision, and the
  human-youkai gauge are evaluated independently for each player.

## Shared state

The following state belongs to the co-op run rather than to either player:

- score;
- point-item count;
- maximum point-item value;
- Time points;
- Last Spell eligibility derived from the shared Time-point total.

When either player raises shared state, both HUD views observe the new value in
the same simulation frame.

## Per-player state

The following state is independent for P1 and P2:

- lives;
- bombs;
- power;
- human-youkai gauge;
- graze count;
- character state, including deathbomb/Last Spell state;
- familiars/options and their attacks.

If a player is hit while the shared Time-point requirement permits a Last
Spell, only that player enters the Last Spell/deathbomb window. A bomb spent in
that window is deducted from that player's bomb stock. The partner does not
enter the window and remains independently controllable.

## Items

Each collectible selects the nearest eligible player using squared distance.
An exact-distance tie is resolved in favor of P1 so that every peer makes the
same deterministic decision.

- Point/resource items update shared score, point-item count, maximum point
  value, or Time points as appropriate.
- Power, life, and bomb items update only the player that actually collected
  the item.
- Item attraction chooses the nearer eligible player and stays deterministic
  on all peers.
- A spirit-form player is not an eligible physical collector.

## Enemy targeting

Boss and enemy aimed attacks target the nearest living physical player. An
exact-distance tie is resolved in favor of P1. Spirit-form players are ignored.
Target selection is simulation state and must not depend on a peer's local
presentation or wall-clock timing.

## Death, spirit form, and revival

The run ends only after both players are out of lives and neither can continue
in physical form.

When a player would otherwise exhaust their lives, that player enters a
non-attacking, non-colliding spirit form instead of immediately ending the run.
The revival interaction follows the TH06 multiplayer patch:

1. The living player overlaps the spirit.
2. The living player is not shooting and holds Focus for 90 simulation frames
   (1.5 seconds at 60 FPS).
3. The living player spends one reserve life.
4. The spirit returns to its current physical life with normal respawn
   invulnerability; no additional reserve life is added. This matches the TH06
   patch's life-counter semantics.

The transfer is not allowed if it would leave no valid living player. Progress
is cancelled when overlap, Focus, or the no-shoot condition is broken.

## Network protocol

v0.1 uses host/guest UDP with deterministic fixed-delay lockstep:

- both peers negotiate an exact protocol/build version and initial RNG seed;
- each packet carries a sequence number plus redundant recent input frames;
- gameplay advances only when both inputs for the delayed frame are available;
- acknowledgements and resend cover packet loss;
- pause, disconnect, and timeout are explicit synchronized states;
- a periodic deterministic state hash detects divergence;
- a mismatch stops the session with diagnostics rather than silently forcing
  only the RNG back into sync.

Rollback, spectators, matchmaking, relay/NAT traversal, and hostile-client
security are not v0.1 requirements.

The minimum state hash includes both players' position/state/resources/gauges,
shared score/point/Time state, the RNG state, frame number, active enemy/bullet
counts, and boss health/phase state.

## Implementation boundaries

- Do not reinterpret TH08's existing `Player::playerType` field as the co-op
  slot index; it already represents original character/team behavior.
- P1-compatible original globals may remain the backing store for P1, while a
  multiplayer-owned state store backs P2 and exposes slot-aware accessors.
- Calls that currently reach `g_Player` or `GameManager` resource getters from
  player, item, bullet, enemy, GUI, and spell code must be audited and made
  explicitly player-aware where the contract requires it.
- Shared values keep one canonical simulation owner.
- Original game executables, DAT archives, score files, and replay files must
  never be committed or distributed.
- Modified multiplayer translation units are not claimed to remain byte-exact
  with the retail executable. Unchanged upstream provenance remains in Git
  history.

## Delivery milestones

1. Reproduce a clean upstream baseline build against the verified 1.00d target.
2. Add a multiplayer state model, two input frames, and deterministic tests.
3. Run two local players with independent team/resource/gauge/familiar state.
4. Implement nearest-player items/aiming, independent graze/Last Spell, spirit
   death/revival, dual HUD, and co-op game-over rules.
5. Add host/guest lockstep transport, handshake, redundancy, timeout, and state
   hash diagnostics.
6. Exercise a two-process local session before publishing the v0.1 release.

## v0.1 acceptance checklist

- [ ] P1/P2 can choose any teams, including identical teams.
- [ ] Both players move, shoot, focus-switch, bomb, collide, and graze
      independently.
- [ ] Lives, bombs, power, and human-youkai gauges cannot leak between slots.
- [ ] Score, point items, maximum point value, and Time points are shared.
- [ ] Items and aimed attacks select the nearest eligible player
      deterministically.
- [ ] Shared Time can enable Last Spell only for the player that was hit.
- [ ] One player can remain active while the other is a spirit.
- [ ] TH06-style life-transfer revival works and game over requires both
      players to be unable to continue.
- [ ] Two local processes stay synchronized under simulated loss/jitter within
      the supported network envelope.
- [ ] A forced divergence is detected and reported by the state hash.
- [ ] Multiplayer does not offer replay save/playback in v0.1.
- [ ] The private repository and release contain no copyrighted original game
      binaries or data archives.
